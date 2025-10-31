////////////////////////////////////////////////////////////////////////////
//	Modified	: 28.10.2025
//	Author		: NSDeathMan
//  Nocturning studio for NS Platform X
//  Complete rewrite with thread-safe architecture
//
//	Major Updates:
//	- Separated computation (Update) and rendering (Render) phases
//	- Support for multiple render passes (GBuffer, shadows, lights)
//	- Thread-safe matrix management for different light sources
//	- Optimized instancing with pre-computed batches
////////////////////////////////////////////////////////////////////////////
#ifndef DetailManagerH
#define DetailManagerH
#pragma once

#include "xrpool.h"
#include "detailformat.h"
#include "detailmodel.h"

#ifdef _EDITOR
#include "ESceneClassList.h"
#endif

// Константы системы деталей
const int dm_max_decompress = 7; // Максимальное количество декомпрессий за кадр
const int dm_size = 64;			 // Размер кэша в слотах
const int dm_cache1_count = 4;	 // Количество слотов в кэше первого уровня
const int dm_cache1_line = dm_size * 2 / dm_cache1_count; // Линия кэша первого уровня
const int dm_max_objects = 64; // Максимальное количество моделей травы
const int dm_obj_in_slot = 4;  // Количество объектов в одном слоте
const int dm_cache_line = dm_size + 1 + dm_size;		 // Полный размер линии кэша
const int dm_cache_size = dm_cache_line * dm_cache_line; // Общий размер кэша
const float dm_fade = float(2 * dm_size) - .5f;			 // Дистанция затухания
const float dm_slot_size = DETAIL_SLOT_SIZE;			 // Размер слота в мире

// Предварительные объявления
struct SlotItem;

// Система occlusion culling с множественными методами
enum EOcclusionMethod
{
	OCCLUSION_NONE = 0,		  // Без occlusion culling
	OCCLUSION_HOM,			  // Аппаратный occlusion mapping
	OCCLUSION_HARDWARE_QUERY, // Аппаратные occlusion queries
	OCCLUSION_GEOMETRY,		  // Геометрический occlusion
	OCCLUSION_PORTALS,		  // Portal-based occlusion
	OCCLUSION_RASTER,		  // Программный растеризатор
	OCCLUSION_HYBRID,		  // Гибридный подход
	OCCLUSION_METHOD_COUNT
};

/**
 * @class CDetailManager
 * @brief Менеджер рендеринга детализированной геометрии (трава, мелкие объекты)
 *
 * Основные особенности:
 * - Многопоточная архитектура с разделением вычислений и рендеринга
 * - Поддержка инстансинга для эффективного рендеринга
 * - Многоуровневая система occlusion culling
 * - Кэширование видимости и преобразований
 * - Поддержка нескольких проходов рендеринга (GBuffer, тени, свет)
 */
class CDetailManager
{
  private:
	// Типы данных должны быть объявлены первыми
	typedef xr_vector<SlotItem*> SlotItemVec;
	typedef SlotItemVec::iterator SlotItemVecIt;

	/**
	 * @struct SlotPart
	 * @brief Часть слота, содержащая объекты определенного типа
	 */
	struct SlotPart
	{
		u32 id;					// ID типа объекта
		SlotItemVec items;		// Все объекты в этой части
		SlotItemVec r_items[3]; // Объекты по LOD уровням

		SlotPart() : id(DetailSlot::ID_Empty)
		{
		}
	};

	// Тип состояния слота
	enum SlotType
	{
		stReady = 0, // Слот готов к рендерингу
		stPending,	 // Слот в процессе подготовки
		stFORCEDWORD = 0xffffffff
	};

	/**
	 * @struct Slot
	 * @brief Слот детализации - основная единица кэширования
	 */
	struct Slot
	{
		struct
		{
			u32 empty : 1;	// Флаг пустого слота
			u32 type : 1;	// Тип состояния слота
			u32 frame : 30; // Кадр последнего обновления
		};
		int sx, sz;	  // Координаты слота в мире
		vis_data vis; // Данные видимости

		// Данные для occlusion culling
		IDirect3DQuery9* occlusion_query;
		u32 last_occlusion_test_frame;
		u32 occlusion_visible_frames;
		bool occlusion_visible;
		float occlusion_confidence;

		SlotPart G[dm_obj_in_slot]; // Части слота по типам объектов

		Slot()
		{
			frame = 0;
			empty = 1;
			type = stReady;
			sx = sz = 0;
			vis.clear();
			occlusion_query = nullptr;
			last_occlusion_test_frame = 0;
			occlusion_visible_frames = 0;
			occlusion_visible = true;
			occlusion_confidence = 1.0f;
		}
	};

	/**
	 * @struct CacheSlot1
	 * @brief Кэш первого уровня для группировки слотов
	 */
	struct CacheSlot1
	{
		u32 empty;
		vis_data vis;
		Slot** slots[dm_cache1_count * dm_cache1_count]; // Указатели на слоты

		// Данные для occlusion culling
		IDirect3DQuery9* occlusion_query;
		u32 last_occlusion_test_frame;
		u32 occlusion_visible_frames;
		bool occlusion_visible;
		float occlusion_confidence;

		CacheSlot1()
		{
			empty = 1;
			vis.clear();
			ZeroMemory(slots, sizeof(slots));
			occlusion_query = nullptr;
			last_occlusion_test_frame = 0;
			occlusion_visible_frames = 0;
			occlusion_visible = true;
			occlusion_confidence = 1.0f;
		}
	};

	/**
	 * @struct InstanceData
	 * @brief Данные для инстансинга - передаются в шейдер
	 */
	struct InstanceData
	{
		Fvector4 mat0;	// Первая строка матрицы преобразования
		Fvector4 mat1;	// Вторая строка матрицы преобразования
		Fvector4 mat2;	// Третья строка матрицы преобразования
		Fvector4 color; // Цвет и освещение
		float lod_id;	// ID LOD уровня

		InstanceData()
		{
			mat0.set(0, 0, 0, 0);
			mat1.set(0, 0, 0, 0);
			mat2.set(0, 0, 0, 0);
			color.set(1, 1, 1, 1);
			lod_id = 0;
		}
	};

	/**
	 * @struct RenderBatch
	 * @brief Пакет для рендеринга - предварительно вычисленные данные
	 */
	struct RenderBatch
	{
		u32 object_index;				   // Индекс объекта
		u32 lod_id;						   // Уровень детализации
		xr_vector<InstanceData> instances; // Данные инстансов

		RenderBatch() : object_index(0), lod_id(0)
		{
		}
	};

	// Плоскости frustum'а для отсечения
	enum EFrustumPlanes
	{
		FRUSTUM_LEFT = 0,
		FRUSTUM_RIGHT,
		FRUSTUM_TOP,
		FRUSTUM_BOTTOM,
		FRUSTUM_NEAR,
		FRUSTUM_FAR,
		FRUSTUM_PLANES_COUNT
	};
	Fplane m_frustum_planes[FRUSTUM_PLANES_COUNT];

	// Система occlusion culling
	struct OcclusionGeometry
	{
		Fvector* vertices;
		u32 vertex_count;
		u16* indices;
		u32 index_count;
		Fbox bounds;

		OcclusionGeometry() : vertices(nullptr), vertex_count(0), indices(nullptr), index_count(0)
		{
			bounds.invalidate();
		}
	};

	struct OcclusionPortal
	{
		Fvector center;
		float radius;
		xr_vector<Fvector> polygon;
		bool active;

		OcclusionPortal() : active(false)
		{
		}
	};

	// Основные типы данных
	typedef xr_vector<xr_vector<SlotItemVec*>> vis_list;
	typedef svector<CDetail*, dm_max_objects> DetailVec;
	typedef DetailVec::iterator DetailIt;
	typedef poolSS<SlotItem, 4096> PSS;

	// Двойная буферизация для данных рендеринга
	struct RenderFrameData
	{
		xr_vector<RenderBatch> renderBatches;
		vis_list renderVisibles[3]; // vis_list = xr_vector<xr_vector<SlotItemVec*>>
		u32 frameNumber;
		bool valid;

		RenderFrameData() : frameNumber(0), valid(false)
		{
		}

		void Clear()
		{
			// Очищаем batches
			renderBatches.clear();
			renderBatches.shrink_to_fit();

			// Очищаем видимые списки
			for (int i = 0; i < 3; i++)
			{
				renderVisibles[i].clear();
				renderVisibles[i].shrink_to_fit();
			}

			valid = false;
			frameNumber = 0;
		}
	};

	RenderFrameData m_renderFrames[2];
	std::atomic<int> m_currentRenderFrame;
	std::atomic<int> m_readyRenderFrame;

	u32 m_consecutiveSkipFrames;
	u32 m_maxConsecutiveSkips;

  private:
	// Матрица дизеринга для распределения объектов
	int dither[16][16];

	// Основные данные
	IReader* m_dtFS;
	DetailHeader m_dtH;
	DetailSlot* m_dtSlots;
	DetailSlot m_DS_empty;

	// Объекты и видимость
	DetailVec m_objects;
	vis_list m_visibles[3];
	vis_list m_render_visibles[3]; // Thread-safe копия для рендеринга

	// Система кэширования
#ifndef _EDITOR
	xrXRC m_xrc;
#endif
	CacheSlot1 m_cache_level1[dm_cache1_line][dm_cache1_line];
	Slot* m_cache[dm_cache_line][dm_cache_line];
	xr_vector<Slot*> m_cache_task;
	Slot m_cache_pool[dm_cache_size];
	int m_cache_cx;
	int m_cache_cz;

	// Система occlusion culling
	xr_vector<IDirect3DQuery9*> m_occlusion_queries;
	xr_vector<Slot*> m_occlusion_test_slots;
	xr_vector<CacheSlot1*> m_occlusion_test_cache1;
	u32 m_occlusion_frame;
	bool m_occlusion_enabled;
	EOcclusionMethod m_primary_occlusion_method;
	EOcclusionMethod m_fallback_occlusion_method;

	// Геометрический occlusion
	xr_vector<OcclusionGeometry> m_occlusion_geometries;
	xr_vector<Fvector> m_simple_occluders;

	// Portal-based occlusion
	xr_vector<OcclusionPortal> m_occlusion_portals;

	// Программный растеризатор
	u32* m_software_depth_buffer;
	u32 m_software_depth_width;
	u32 m_software_depth_height;
	float m_software_depth_scale;

	// Пул памяти для SlotItem
	PSS m_poolSI;

	// Аппаратные ресурсы
	ref_geom m_hw_Geom;
	ref_geom m_hw_Geom_Instanced;
	u32 m_hw_BatchSize;
	IDirect3DVertexBuffer9* m_hw_VB;
	IDirect3DVertexBuffer9* m_hw_VB_Instances;
	IDirect3DIndexBuffer9* m_hw_IB;

	// Геометрия для occlusion тестов
	ref_geom m_occlusion_geom;
	IDirect3DVertexBuffer9* m_occlusion_vb;
	IDirect3DIndexBuffer9* m_occlusion_ib;

	// Многопоточность и состояние
	xrCriticalSection m_MT;
	xrCriticalSection m_VisibleMT;
	volatile u32 m_frame_calc;
	volatile u32 m_frame_rendered;
	bool m_initialized;
	bool m_loaded;
	volatile BOOL m_bInLoad;
	volatile BOOL m_bShuttingDown;
	volatile LONG m_bUpdating;
	bool m_bRegisteredInParallel;

	// НОВОЕ: Система матриц для многопроходного рендеринга
	Fmatrix m_currentView;
	Fmatrix m_currentProject;
	Fmatrix m_currentViewProject;
	bool m_bUseCustomMatrices;
	bool m_bUpdateRequired;
	xr_vector<RenderBatch> m_renderBatches;

	// Асинхронная система
	std::atomic<bool> m_bAsyncUpdateInProgress;
	std::atomic<bool> m_bAsyncUpdateRequired;
	std::thread m_UpdateThread;
	std::mutex m_UpdateMutex;
	std::condition_variable m_UpdateCondition;
	std::atomic<bool> m_bUpdateThreadShouldExit;

	// Данные для асинхронного обновления
	Fmatrix m_AsyncView;
	Fmatrix m_AsyncProject;
	u32 m_AsyncFrame;

	// Результаты асинхронного обновления
	xr_vector<RenderBatch> m_AsyncRenderBatches;
	vis_list m_AsyncVisibles[3];
	std::mutex m_AsyncDataMutex;

	// Добавляем счетчики для диагностики утечек
	std::atomic<u32> m_slotItemsCreated;
	std::atomic<u32> m_slotItemsDestroyed;
	std::atomic<u32> m_currentSlotItems;

	// Метод для диагностики
	void DumpSlotItemStats(const char* location);
	void DumpMemoryStats();

  public:
	// Статическая конфигурация
	static BOOL s_disable_hom;
	static BOOL s_use_hom_occlusion;
	static float s_occlusion_padding_scale;
	static float s_max_occlusion_distance;
	static BOOL s_use_occlusion_culling;
	static u32 s_occlusion_frame_interval;
	static u32 s_occlusion_min_visible_frames;
	static EOcclusionMethod s_preferred_occlusion_method;
	static float s_occlusion_confidence_threshold;

  private:
	// Внутренние методы

	// Управление параллельными задачами
	void UnregisterFromParallel();

	// Основные операции
	void Internal_Load();
	void Internal_Unload();
	void Internal_Render();

	// Управление видимостью
	void UpdateVisibleM();
	void ClearVisibleLists();
	void CopyVisibleListsForRender();

	// Отсечение по frustum'у
	void UpdateFrustumPlanes();
	bool IsSphereInsideFrustum(const Fvector& center, float radius) const;
	bool IsAABBInsideFrustum(const Fvector& min, const Fvector& max) const;
	bool IsSlotVisible(const Slot* slot) const;

	// Система occlusion culling
	void InitializeOcclusionSystem();
	void CleanupOcclusionSystem();
	void DetectAvailableOcclusionMethods();
	EOcclusionMethod SelectOcclusionMethod() const;

	// Геометрические методы occlusion
	void LoadOcclusionGeometry();
	void BuildSimpleOccluders();
	bool TestGeometryOcclusion(const Fvector& test_point, float test_radius) const;
	bool TestSimpleOccluders(const Fvector& test_point, float test_radius) const;

	// Portal-based occlusion
	void LoadOcclusionPortals();
	bool TestPortalOcclusion(const Fvector& test_point, float test_radius) const;
	bool IsPointBehindPortal(const Fvector& point, const OcclusionPortal& portal) const;

	// Аппаратные occlusion queries
	void UpdateHardwareOcclusionTests();
	void ProcessHardwareOcclusionResults();
	bool IsSlotHardwareOccluded(Slot* slot);
	bool IsCache1HardwareOccluded(CacheSlot1* cache1);
	void RenderHardwareOcclusionTest(const Fvector& center, float radius);
	void RenderHardwareOcclusionTest(const Fvector& min, const Fvector& max);

	// Программный растеризатор
	void InitializeSoftwareRasterizer();
	void CleanupSoftwareRasterizer();
	void UpdateSoftwareDepthBuffer();
	bool TestSoftwareOcclusion(const Fvector& test_point, float test_radius) const;
	void RasterizeOcclusionGeometry();

	// Гибридное тестирование occlusion
	bool PerformOcclusionTest(Slot* slot);
	bool PerformOcclusionTest(CacheSlot1* cache1);
	float CalculateOcclusionConfidence(const Fvector& test_point, EOcclusionMethod method) const;

	// Унифицированный интерфейс occlusion
	bool IsSlotOccluded(Slot* slot);
	bool IsCache1Occluded(CacheSlot1* cache1);
	void UpdateOcclusionTests();
	void ProcessOcclusionResults();
	void RenderOcclusionQueries();

	// Система кэширования
	void UpdateCacheLevel1Bounds();
	void cache_Initialize();
	void cache_Update(int sx, int sz, Fvector& view, int limit);
	void cache_Task(int gx, int gz, Slot* D);
	Slot* cache_Query(int sx, int sz);
	void cache_Decompress(Slot* D);
	BOOL cache_Validate();
	DetailSlot& QueryDB(int sx, int sz);
	void bwdithermap(int levels);

	// Вспомогательные методы для работы со слотами
	bool InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size);
	float GetSlotYBase(const DetailSlot& DS) const
	{
		return DS.r_ybase();
	}
	float GetSlotYHeight(const DetailSlot& DS) const
	{
		return DS.r_yheight();
	}
	float GetSlotHemi(const DetailSlot& DS) const
	{
		return DS.r_qclr(DS.c_hemi, 15);
	}
	float GetSlotSun(const DetailSlot& DS) const
	{
		return DS.r_qclr(DS.c_dir, 15);
	}
	u32 GetSlotID(const DetailSlot& DS, int i) const
	{
		return DS.r_id(i);
	}

	// Преобразования координат
	int cg2w_X(int x)
	{
		return m_cache_cx - dm_size + x;
	}
	int cg2w_Z(int z)
	{
		return m_cache_cz - dm_size + (dm_cache_line - 1 - z);
	}
	int w2cg_X(int x)
	{
		return x - m_cache_cx + dm_size;
	}
	int w2cg_Z(int z)
	{
		return (dm_cache_line - 1) - (z - m_cache_cz + dm_size);
	}

	// Аппаратный рендеринг
	void hw_Load();
	void hw_Unload();
	void hw_Render(const xr_vector<RenderBatch>& batches);
	void hw_Render_dump(const xr_vector<RenderBatch>& batches);

	// НОВОЕ: Управление матрицами и батчами
	void UpdateMatrices();
	void BuildRenderBatches();

	// Форматы вершин
	static D3DVERTEXELEMENT9* GetDetailsVertexDecl();
	static D3DVERTEXELEMENT9* GetDetailsInstancedVertexDecl();
	static D3DVERTEXELEMENT9* GetOcclusionVertexDecl();

	// Структура вершины для аппаратного рендеринга
#pragma pack(push, 1)
	struct vertHW
	{
		float x, y, z;
		short u, v, t, mid;
	};
#pragma pack(pop)

	// Вспомогательные методы
	short QuantConstant(float v);

	void AsyncUpdateThreadProc();
	void Internal_AsyncUpdate();

	void ClearRenderData();

  public:
	// Публичный интерфейс

	CDetailManager();
	virtual ~CDetailManager();

	// Основные методы
	void Load();
	void Unload();

	// НОВОЕ: Многопроходный рендеринг
	void Render();												 // Рендер для основной камеры
	void Render(const Fmatrix& view, const Fmatrix& projection); // Рендер с кастомными матрицами

	void __stdcall Update();
	void UpdateCache();
	void SafeUpdate();

	// НОВОЕ: Управление матрицами
	void SetCustomMatrices(const Fmatrix& view, const Fmatrix& projection);
	void ResetMatrices();

	// Управление устройством
	void OnDeviceResetBegin();
	void OnDeviceResetEnd();

	// Завершение работы
	void Shutdown();
	IC bool IsShuttingDown() const
	{
		return m_bShuttingDown;
	}

	// Асинхронное управление
	void StartAsyncUpdate();
	void WaitForAsyncUpdate();
	bool IsAsyncUpdateComplete() const
	{
		return !m_bAsyncUpdateInProgress;
	}

	bool NeedsPriorityUpdate() const;

	void SwapRenderBuffers();
	RenderFrameData* GetCurrentRenderData();

	// Управление occlusion culling
	void EnableOcclusionCulling(bool enable)
	{
		m_occlusion_enabled = enable;
	}
	bool IsOcclusionCullingEnabled() const
	{
		return m_occlusion_enabled;
	}
	void SetOcclusionMethod(EOcclusionMethod method);
	EOcclusionMethod GetCurrentOcclusionMethod() const
	{
		return m_primary_occlusion_method;
	}

	// Отладка и статистика
	u32 GetOccludedSlotCount() const;
	u32 GetVisibleSlotCount() const;
	void DumpOcclusionStats();

#ifdef _EDITOR
	virtual ObjectList* GetSnapList() = 0;
#endif

  private:
	// Запрет копирования
	CDetailManager(const CDetailManager&);
	CDetailManager& operator=(const CDetailManager&);
};

// Определение SlotItem должно быть после CDetailManager из-за зависимости от PSS
struct SlotItem
{
	Fmatrix mRotY;
	float scale;
	float scale_calculated;
	u32 vis_ID;
	float c_hemi;
	float c_sun;

	SlotItem() : scale(1.0f), scale_calculated(1.0f), vis_ID(0), c_hemi(1.0f), c_sun(1.0f)
	{
		mRotY.identity();
	}
};

#endif // DetailManagerH
