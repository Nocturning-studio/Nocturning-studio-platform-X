////////////////////////////////////////////////////////////////////////////
//	Modified	: 28.10.2025
//	Author		: NSDeathMan
//  Nocturning studio for NS Platform X
//  Complete rewrite with thread-safe architecture
//
//	Major Improvements:
//	1. Separated computation and rendering phases
//	2. Multi-pass rendering support (GBuffer, shadows, lights)
//	3. Thread-safe matrix management
//	4. Optimized instancing with pre-computed batches
//	5. Comprehensive error handling and logging
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include <cmath>
#include "DetailManager.h"
#include "cl_intersect.h"
#include "..\xrEngine\xrXRC.h"

#ifdef _EDITOR
#include "ESceneClassList.h"
#include "Scene.h"
#include "SceneObject.h"
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif

// ===========================================================================
// Инициализация статических членов
// ===========================================================================

BOOL CDetailManager::s_disable_hom = TRUE;
BOOL CDetailManager::s_use_hom_occlusion = FALSE;
float CDetailManager::s_occlusion_padding_scale = 1.0f;
float CDetailManager::s_max_occlusion_distance = 150.0f;
BOOL CDetailManager::s_use_occlusion_culling = TRUE;
u32 CDetailManager::s_occlusion_frame_interval = 4;
u32 CDetailManager::s_occlusion_min_visible_frames = 2;
EOcclusionMethod CDetailManager::s_preferred_occlusion_method = OCCLUSION_HYBRID;
float CDetailManager::s_occlusion_confidence_threshold = 0.7f;

// Матрица дизеринга 4x4 для распределения объектов
const int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};

// ===========================================================================
// Форматы вершин
// ===========================================================================

// Формат вершин для обычного рендеринга
static D3DVERTEXELEMENT9 dwDecl_Details[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()};

// Формат вершин для инстансинга
static D3DVERTEXELEMENT9 dwDecl_Details_Instanced[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{1, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	{1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},
	{1, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3},
	{1, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4},
	D3DDECL_END()};

// Формат вершин для occlusion тестов
static D3DVERTEXELEMENT9 dwDecl_Occlusion[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};

D3DVERTEXELEMENT9* CDetailManager::GetDetailsVertexDecl()
{
	return dwDecl_Details;
}

D3DVERTEXELEMENT9* CDetailManager::GetDetailsInstancedVertexDecl()
{
	return dwDecl_Details_Instanced;
}

D3DVERTEXELEMENT9* CDetailManager::GetOcclusionVertexDecl()
{
	return dwDecl_Occlusion;
}

// ===========================================================================
// Вспомогательные функции
// ===========================================================================

/**
 * @brief Интерполяция значений в базе 4x4
 * @param base - базовые значения для интерполяции
 * @param x, y - координаты для интерполяции
 * @param size - размер области интерполяции
 * @return интерполированное значение
 */
IC float Interpolate(float* base, u32 x, u32 y, u32 size)
{
	float f = float(size);
	float fx = float(x) / f;
	float ifx = 1.f - fx;
	float fy = float(y) / f;
	float ify = 1.f - fy;

	float c01 = base[0] * ifx + base[1] * fx;
	float c23 = base[2] * ifx + base[3] * fx;
	float c02 = base[0] * ify + base[2] * fy;
	float c13 = base[1] * ify + base[3] * fy;

	float cx = ify * c01 + fy * c23;
	float cy = ifx * c02 + fx * c13;
	return (cx + cy) / 2;
}

// ===========================================================================
// Реализация CDetailManager
// ===========================================================================

/**
 * @brief Конструктор менеджера деталей
 * @note Инициализирует все системы в безопасном состоянии
 */
CDetailManager::CDetailManager()
	: m_dtFS(nullptr), m_dtSlots(nullptr), m_hw_Geom(nullptr), m_hw_Geom_Instanced(nullptr), m_hw_BatchSize(0),
	  m_hw_VB(nullptr), m_hw_VB_Instances(nullptr), m_hw_IB(nullptr), m_cache_cx(0), m_cache_cz(0), m_frame_calc(0),
	  m_frame_rendered(0), m_initialized(false), m_loaded(false), m_bInLoad(FALSE), m_bShuttingDown(FALSE),
	  m_bRegisteredInParallel(false), m_occlusion_frame(0), m_occlusion_enabled(false),
	  m_primary_occlusion_method(OCCLUSION_NONE), m_fallback_occlusion_method(OCCLUSION_NONE),
	  m_software_depth_buffer(nullptr), m_software_depth_width(0), m_software_depth_height(0),
	  m_software_depth_scale(1.0f), m_occlusion_geom(nullptr), m_occlusion_vb(nullptr), m_occlusion_ib(nullptr),
	  m_bUseCustomMatrices(false), m_bUpdateRequired(false)
{
	Msg("CDetailManager::CDetailManager() - Initializing detail manager");

	// Инициализация пустого слота
	m_DS_empty.w_id(0, DetailSlot::ID_Empty);
	m_DS_empty.w_id(1, DetailSlot::ID_Empty);
	m_DS_empty.w_id(2, DetailSlot::ID_Empty);
	m_DS_empty.w_id(3, DetailSlot::ID_Empty);

	// Инициализация кэша
	ZeroMemory(m_cache, sizeof(m_cache));
	ZeroMemory(m_cache_level1, sizeof(m_cache_level1));
	ZeroMemory(m_cache_pool, sizeof(m_cache_pool));

	// Инициализация плоскостей frustum'а
	ZeroMemory(m_frustum_planes, sizeof(m_frustum_planes));

	// Инициализация матрицы дизеринга
	bwdithermap(2);

	// Инициализация матриц
	m_currentView.identity();
	m_currentProject.identity();
	m_currentViewProject.identity();

	Msg("CDetailManager::CDetailManager() - Initialization completed");
}

/**
 * @brief Деструктор менеджера деталей
 * @note Гарантирует безопасное освобождение всех ресурсов
 */
CDetailManager::~CDetailManager()
{
	Shutdown();
}

/**
 * @brief Завершение работы менеджера деталей
 * @note Останавливает все операции и освобождает ресурсы
 */
void CDetailManager::Shutdown()
{
	if (m_bShuttingDown)
		return;

	Msg("CDetailManager::Shutdown() - Starting shutdown sequence");

	// Устанавливаем флаг завершения работы
	m_bShuttingDown = TRUE;

	// Удаляем из параллельных задач
	UnregisterFromParallel();

	// Выгружаем ресурсы
	Unload();

	Msg("CDetailManager::Shutdown() - Shutdown completed");
}

// ===========================================================================
// НОВАЯ СИСТЕМА: Управление матрицами и многопроходный рендеринг
// ===========================================================================

/**
 * @brief Обновление текущих матриц
 * @note Вычисляет результирующую матрицу на основе текущих установок
 */
void CDetailManager::UpdateMatrices()
{
	if (m_bUseCustomMatrices)
	{
		// Используем кастомные матрицы (для источников света)
		m_currentViewProject.mul(m_currentProject, m_currentView);
	}
	else
	{
		// Используем матрицы основной камеры
		m_currentView = Device.mView;
		m_currentProject = Device.mProject;
		m_currentViewProject = Device.mFullTransform;
	}
}

/**
 * @brief Установка кастомных матриц для рендеринга
 * @param view - матрица вида
 * @param projection - матрица проекции
 * @note Используется для рендеринга с позиции источников света
 */
void CDetailManager::SetCustomMatrices(const Fmatrix& view, const Fmatrix& projection)
{
	m_MT.Enter();

	m_currentView = view;
	m_currentProject = projection;
	m_bUseCustomMatrices = true;
	UpdateMatrices();

	m_MT.Leave();
}

/**
 * @brief Сброс к матрицам основной камеры
 * @note Возвращает систему к рендерингу с основной камеры
 */
void CDetailManager::ResetMatrices()
{
	m_MT.Enter();

	m_bUseCustomMatrices = false;
	UpdateMatrices();

	m_MT.Leave();
}

/**
 * @brief Построение пакетов для рендеринга
 * @note Предварительно вычисляет все данные для инстансинга
 */
void CDetailManager::BuildRenderBatches()
{
	m_renderBatches.clear();

	// Проходим по всем объектам и LOD уровням
	for (u32 obj_index = 0; obj_index < m_objects.size(); obj_index++)
	{
		CDetail& Object = *m_objects[obj_index];

		for (u32 lod_id = 0; lod_id < 3; lod_id++)
		{
			xr_vector<SlotItemVec*>& vis = m_render_visibles[lod_id][obj_index];
			if (vis.empty())
				continue;

			// Создаем новый пакет
			RenderBatch batch;
			batch.object_index = obj_index;
			batch.lod_id = lod_id;
			batch.instances.reserve(512); // Предварительное резервирование

			// Собираем все инстансы для этого объекта и LOD
			for (auto& slot_items : vis)
			{
				if (!slot_items)
					continue;

				for (auto& item_ptr : *slot_items)
				{
					if (!item_ptr)
						continue;

					SlotItem& item = *item_ptr;
					InstanceData inst;

					// Вычисляем матрицу преобразования
					Fmatrix& M = item.mRotY;
					float scale = item.scale_calculated;

					inst.mat0.set(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
					inst.mat1.set(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
					inst.mat2.set(M._13 * scale, M._23 * scale, M._33 * scale, M._43);

					// Устанавливаем цвет и освещение
					inst.color.set(item.c_sun, item.c_sun, item.c_sun, item.c_hemi);
					inst.lod_id = (float)lod_id;

					batch.instances.push_back(inst);
				}
			}

			// Добавляем пакет только если в нем есть инстансы
			if (!batch.instances.empty())
			{
				m_renderBatches.push_back(batch);
			}
		}
	}

	Msg("CDetailManager::BuildRenderBatches() - Built %d render batches", m_renderBatches.size());
}

// ===========================================================================
// Основной цикл обновления и рендеринга
// ===========================================================================

/**
 * @brief Основной метод обновления (вызывается асинхронно)
 * @note Выполняет все вычисления: кэширование, видимость, occlusion culling
 */
void __stdcall CDetailManager::Update()
{
	static volatile LONG updateGuard = 0;

	// Проверка условий выполнения
	if (m_bShuttingDown || !psDeviceFlags.is(rsDetails))
		return;

	// Захват мьютекса обновления
	if (InterlockedExchange(&updateGuard, 1) == 1)
		return;

	__try
	{
		// Дополнительные проверки
		if (m_frame_calc == Device.dwFrame || !Device.Statistic || !g_pGameLevel || !g_pGamePersistent)
		{
			InterlockedExchange(&updateGuard, 0);
			return;
		}

		m_MT.Enter();

		// Проверка состояния системы
		if (m_bShuttingDown || !m_loaded || !m_initialized || m_bInLoad)
		{
			m_MT.Leave();
			InterlockedExchange(&updateGuard, 0);
			return;
		}

		// Основной цикл вычислений
		if (m_frame_calc != Device.dwFrame)
		{
			__try
			{
				// Сбрасываем матрицы к основной камере для вычислений
				ResetMatrices();

				// Выполняем все вычисления
				UpdateCache();
				UpdateVisibleM();
				CopyVisibleListsForRender();
				BuildRenderBatches(); // Предварительное построение пакетов

				m_frame_calc = Device.dwFrame;
				m_bUpdateRequired = true;

				Msg("CDetailManager::Update() - Frame %d updated successfully", Device.dwFrame);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				Msg("![ERROR] CDetailManager::Update() - Exception during update");
			}
		}

		m_MT.Leave();
	}
	__finally
	{
		InterlockedExchange(&updateGuard, 0);
	}
}

/**
 * @brief Рендеринг для основной камеры
 * @note Использует матрицы основной камеры устройства
 */
void CDetailManager::Render()
{
	ResetMatrices();
	Internal_Render();
}

/**
 * @brief Рендеринг с кастомными матрицами
 * @param view - матрица вида
 * @param projection - матрица проекции
 * @note Используется для рендеринга с позиции источников света
 */
void CDetailManager::Render(const Fmatrix& view, const Fmatrix& projection)
{
	SetCustomMatrices(view, projection);
	Internal_Render();
}

/**
 * @brief Внутренний метод рендеринга
 * @note Выполняет фактический рендеринг с текущими матрицами
 */
void CDetailManager::Internal_Render()
{
#ifndef _EDITOR
	if (!m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	if (!psDeviceFlags.is(rsDetails))
		return;
#endif

	m_MT.Enter();

	__try
	{
		// Синхронное обновление если необходимо
		if (m_frame_calc != Device.dwFrame)
		{
			Device.Statistic->RenderDUMP_DT_Update.Begin();
			Update();
			Device.Statistic->RenderDUMP_DT_Update.End();
		}

		// Начало рендеринга
		Device.Statistic->RenderDUMP_DT_Render.Begin();

		// Настройка состояний рендеринга
		RenderBackend.set_CullMode(CULL_NONE);
		RenderBackend.set_xform_world(Fidentity);

		// Выполнение аппаратного рендеринга
		hw_Render();

		// Восстановление состояний
		RenderBackend.set_CullMode(CULL_CCW);
		Device.Statistic->RenderDUMP_DT_Render.End();

		// Обработка occlusion queries
		if (m_occlusion_enabled && s_use_occlusion_culling)
		{
			Device.Statistic->RenderDUMP_DT_Occq.Begin();
			RenderOcclusionQueries();
			Device.Statistic->RenderDUMP_DT_Occq.End();
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Msg("![ERROR] CDetailManager::Internal_Render() - Exception during rendering");
	}

	m_MT.Leave();
}

// ===========================================================================
// Аппаратный рендеринг с инстансингом
// ===========================================================================

/**
 * @brief Квантование константы для вершинного формата
 * @param v - исходное значение
 * @return квантованное значение
 */
short CDetailManager::QuantConstant(float v)
{
	int t = iFloor(v * 16384.0f);
	clamp(t, -32768, 32767);
	return short(t & 0xffff);
}

/**
 * @brief Загрузка аппаратных ресурсов
 * @note Создает vertex buffers, index buffers и шейдеры
 */
void CDetailManager::hw_Load()
{
	if (m_bShuttingDown)
		return;

	Msg("CDetailManager::hw_Load() - Loading hardware resources");

	if (m_objects.empty())
	{
		Msg("CDetailManager::hw_Load() - No objects to load");
		return;
	}

	// Подсчет общего количества вершин и индексов
	u32 totalVertices = 0;
	u32 totalIndices = 0;

	for (u32 i = 0; i < m_objects.size(); i++)
	{
		CDetail& D = *m_objects[i];
		totalVertices += D.number_vertices;
		totalIndices += D.number_indices;
	}

	Msg("CDetailManager::hw_Load() - Total vertices: %d, indices: %d", totalVertices, totalIndices);

	if (totalVertices == 0 || totalIndices == 0)
	{
		Msg("CDetailManager::hw_Load() - No geometry data");
		return;
	}

	// Создание vertex buffer'а
	HRESULT hr = HW.pDevice->CreateVertexBuffer(totalVertices * sizeof(vertHW), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED,
												&m_hw_VB, 0);
	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create vertex buffer (HR: 0x%08X)", hr);
		return;
	}

	// Создание index buffer'а
	hr = HW.pDevice->CreateIndexBuffer(totalIndices * sizeof(u16), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED,
									   &m_hw_IB, 0);
	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create index buffer (HR: 0x%08X)", hr);
		_RELEASE(m_hw_VB);
		return;
	}

	// Создание buffer'а для инстансов
	m_hw_BatchSize = 256; // Оптимальный размер батча для инстансинга
	hr = HW.pDevice->CreateVertexBuffer(m_hw_BatchSize * sizeof(InstanceData), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
										D3DPOOL_DEFAULT, &m_hw_VB_Instances, 0);
	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create instance buffer (HR: 0x%08X)", hr);
		_RELEASE(m_hw_VB);
		_RELEASE(m_hw_IB);
		return;
	}

	// Заполнение vertex buffer'а
	vertHW* pVerts = nullptr;
	hr = m_hw_VB->Lock(0, 0, (void**)&pVerts, 0);
	if (SUCCEEDED(hr) && pVerts)
	{
		u32 vertexOffset = 0;
		for (u32 o = 0; o < m_objects.size(); o++)
		{
			CDetail& D = *m_objects[o];

			// Вычисление масштаба для текстурных координат
			float height_range = D.bv_bb.max.y - D.bv_bb.min.y;
			float t_scale = (fis_zero(height_range)) ? 0.0f : 1.0f / height_range;

			for (u32 v = 0; v < D.number_vertices; v++)
			{
				Fvector& vP = D.vertices[v].P;

				pVerts->x = vP.x;
				pVerts->y = vP.y;
				pVerts->z = vP.z;

				pVerts->u = QuantConstant(D.vertices[v].u);
				pVerts->v = QuantConstant(D.vertices[v].v);
				pVerts->t = QuantConstant((vP.y - D.bv_bb.min.y) * t_scale);
				pVerts->mid = 0;

				pVerts++;
			}
			vertexOffset += D.number_vertices;
		}
		m_hw_VB->Unlock();
	}
	else
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to lock vertex buffer");
	}

	// Заполнение index buffer'а
	u16* pIndices = nullptr;
	hr = m_hw_IB->Lock(0, 0, (void**)&pIndices, 0);
	if (SUCCEEDED(hr) && pIndices)
	{
		for (u32 o = 0; o < m_objects.size(); o++)
		{
			CDetail& D = *m_objects[o];
			for (u32 i = 0; i < D.number_indices; i++)
			{
				*pIndices++ = u16(D.indices[i]);
			}
		}
		m_hw_IB->Unlock();
	}
	else
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to lock index buffer");
	}

	// Создание геометрии
	m_hw_Geom.create(GetDetailsVertexDecl(), m_hw_VB, m_hw_IB);
	m_hw_Geom_Instanced.create(GetDetailsInstancedVertexDecl(), m_hw_VB, m_hw_IB);

	Msg("CDetailManager::hw_Load() - Hardware resources loaded successfully");
}

/**
 * @brief Выгрузка аппаратных ресурсов
 */
void CDetailManager::hw_Unload()
{
	Msg("CDetailManager::hw_Unload() - Unloading hardware resources");

	m_hw_Geom.destroy();
	m_hw_Geom_Instanced.destroy();

	_RELEASE(m_hw_VB_Instances);
	_RELEASE(m_hw_IB);
	_RELEASE(m_hw_VB);

	m_hw_BatchSize = 0;

	Msg("CDetailManager::hw_Unload() - Hardware resources unloaded");
}

/**
 * @brief Получение параметров ветра
 * @return параметры ветра в формате Fvector4
 */
Fvector4 GetWindParams()
{
	CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
	float WindGusting = desc->wind_gusting;
	float WindStrength = desc->wind_strength;
	float WindDirectionX = cos(desc->wind_direction), WindDirectionY = sin(desc->wind_direction);
	return Fvector4().set(WindDirectionX, WindDirectionY, WindGusting, WindStrength);
}

/**
 * @brief Получение параметров турбулентности ветра
 * @return параметры турбулентности в формате Fvector4
 */
Fvector4 GetWindTurbulence()
{
	CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
	float Turbulence = desc->wind_turbulence;
	float TurbulencePacked = std::max(std::min(Turbulence, 0.0f), 1.0f);
	float WindSpeed = Device.fTimeGlobal * 2.5f * (1.0f + TurbulencePacked);
	return Fvector4().set(Turbulence, TurbulencePacked, WindSpeed, 0);
}

/**
 * @brief Основной метод аппаратного рендеринга
 */
void CDetailManager::hw_Render()
{
	if (!psDeviceFlags.is(rsDetails))
		return;

	if (!m_hw_Geom_Instanced || !m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	try
	{
		RenderBackend.set_Geometry(m_hw_Geom_Instanced);
		hw_Render_dump();
	}
	catch (...)
	{
		Msg("![ERROR] CDetailManager::hw_Render() - Exception during rendering");
	}
}

/**
 * @brief Внутренний метод аппаратного рендеринга с инстансингом
 * @note Рендерит предварительно собранные пакеты с текущими матрицами
 */
void CDetailManager::hw_Render_dump()
{
	Device.Statistic->RenderDUMP_DT_Count = 0;

	// Используем текущие матрицы (основной камеры или источника света)
	Fmatrix mWorld = Fidentity;
	Fmatrix mView = m_currentView;
	Fmatrix mProject = m_currentProject;
	Fmatrix mWorldViewProject = m_currentViewProject;
	Fmatrix mWorldView;
	mWorldView.mul(mView, mWorld);

	// Получение параметров ветра
	Fvector4 wind_params = GetWindParams();
	Fvector4 wind_turbulence = GetWindTurbulence();

	// Сброс кэша констант
	RenderBackend.get_ConstantCache_Vertex().force_dirty();
	RenderBackend.get_ConstantCache_Pixel().force_dirty();

	// Рендеринг предварительно собранных пакетов
	for (const auto& batch : m_renderBatches)
	{
		u32 obj_index = batch.object_index;
		u32 lod_id = batch.lod_id;

		CDetail& Object = *m_objects[obj_index];

		// Вычисление смещений в буферах
		u32 vOffset = 0;
		u32 iOffset = 0;
		for (u32 i = 0; i < obj_index; i++)
		{
			vOffset += m_objects[i]->number_vertices;
			iOffset += m_objects[i]->number_indices;
		}

		// Выбор шейдера в зависимости от типа объекта
		int shader_id = Object.m_Flags.is(DO_NO_WAVING) ? SE_DETAIL_NORMAL_STATIC : SE_DETAIL_NORMAL_ANIMATED;
		if (!Object.shader || !Object.shader->E[shader_id])
			continue;

		const xr_vector<InstanceData>& instances = batch.instances;
		if (instances.empty())
			continue;

		// Настройка шейдера и констант
		RenderBackend.set_Element(Object.shader->E[shader_id]);
		RenderBackend.get_ConstantCache_Vertex().force_dirty();
		RenderBackend.get_ConstantCache_Pixel().force_dirty();

		RenderBackend.set_Constant("m_WorldViewProject", mWorldViewProject);
		RenderBackend.set_Constant("m_WorldView", mWorldView);

		// Установка параметров ветра для анимированных объектов
		if (!Object.m_Flags.is(DO_NO_WAVING))
		{
			RenderBackend.set_Constant("m_wind_turbulence", wind_turbulence);
			RenderBackend.set_Constant("m_wind_params", wind_params);
		}

		RenderBackend.flush();

		// Разбиение на батчи и рендеринг
		u32 total_instances = instances.size();
		u32 batch_count = (total_instances + m_hw_BatchSize - 1) / m_hw_BatchSize;

		for (u32 batch_idx = 0; batch_idx < batch_count; batch_idx++)
		{
			u32 start_instance = batch_idx * m_hw_BatchSize;
			u32 end_instance = std::min(start_instance + m_hw_BatchSize, total_instances);
			u32 batch_instance_count = end_instance - start_instance;

			// Блокировка и заполнение buffer'а инстансов
			InstanceData* pInstances = nullptr;
			HRESULT hr = m_hw_VB_Instances->Lock(0, batch_instance_count * sizeof(InstanceData), (void**)&pInstances,
												 D3DLOCK_DISCARD);

			if (SUCCEEDED(hr) && pInstances)
			{
				CopyMemory(pInstances, &instances[start_instance], batch_instance_count * sizeof(InstanceData));
				m_hw_VB_Instances->Unlock();

				// Настройка потоков данных
				HW.pDevice->SetStreamSource(0, m_hw_VB, 0, sizeof(vertHW));
				HW.pDevice->SetStreamSource(1, m_hw_VB_Instances, 0, sizeof(InstanceData));

				// Настройка частоты потоков для инстансинга
				HW.pDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | batch_instance_count);
				HW.pDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul);

				// Рендеринг инстансов
				hr = HW.pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices, iOffset,
													  Object.number_indices / 3);

				if (SUCCEEDED(hr))
				{
					Device.Statistic->RenderDUMP_DT_Count += batch_instance_count;
				}

				// Восстановление частоты потоков
				HW.pDevice->SetStreamSourceFreq(0, 1);
				HW.pDevice->SetStreamSourceFreq(1, 1);
			}
		}
	}

	// Очистка пакетов после рендеринга
	m_renderBatches.clear();
}

// ===========================================================================
// Управление параллельными задачами
// ===========================================================================

/**
 * @brief Удаление из списка параллельных задач
 */
void CDetailManager::UnregisterFromParallel()
{
	if (m_bRegisteredInParallel)
	{
		Device.seqParallel.remove([this](const fastdelegate::FastDelegate0<>& delegate) { return true; });
		m_bRegisteredInParallel = false;
		Msg("CDetailManager: Unregistered from parallel tasks");
	}
}

// ===========================================================================
// Система загрузки и выгрузки ресурсов
// ===========================================================================

/**
 * @brief Загрузка системы деталей
 * @note Загружает модели, слоты, инициализирует системы кэширования и рендеринга
 */
void CDetailManager::Load()
{
	if (m_bShuttingDown)
		return;

	Msg("CDetailManager::Load() - Starting detail system loading");

	if (m_loaded)
	{
		Msg("CDetailManager::Load() - Already loaded, skipping");
		return;
	}

	m_MT.Enter();
	try
	{
		Internal_Load();
		m_loaded = true;

		// Регистрация в параллельных задачах если поддерживается многопоточность
		if (!m_bRegisteredInParallel && ps_render_flags.test(RFLAG_EXP_MT_CALC))
		{
			Device.seqParallel.insert(Device.seqParallel.begin(),
									  fastdelegate::FastDelegate0<>(this, &CDetailManager::Update));
			m_bRegisteredInParallel = true;
			Msg("CDetailManager::Load() - Registered in parallel tasks");
		}
	}
	catch (...)
	{
		Msg("![ERROR] CDetailManager::Load() - Exception during loading");
		m_loaded = false;
	}
	m_MT.Leave();

	Msg("CDetailManager::Load() - Loading completed successfully");
}

/**
 * @brief Внутренняя загрузка ресурсов
 * @note Выполняет фактическую загрузку данных из файлов
 */
void CDetailManager::Internal_Load()
{
#ifndef _EDITOR
	if (m_bShuttingDown)
		return;

	m_bInLoad = TRUE;

	// Проверка существования файла деталей уровня
	if (!FS.exist("$level$", "level.details"))
	{
		Msg("CDetailManager::Internal_Load() - level.details not found, details disabled");
		m_bInLoad = FALSE;
		return;
	}

	// Загрузка файла деталей
	string_path fn;
	FS.update_path(fn, "$level$", "level.details");
	Msg("CDetailManager::Internal_Load() - Loading details from: %s", fn);

	m_dtFS = FS.r_open(fn);
	if (!m_dtFS)
	{
		Msg("![ERROR] CDetailManager::Internal_Load() - Failed to open details file");
		m_bInLoad = FALSE;
		return;
	}

	// Безопасное чтение заголовка
	__try
	{
		m_dtFS->r_chunk_safe(0, &m_dtH, sizeof(m_dtH));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Msg("![ERROR] CDetailManager::Internal_Load() - Failed to read detail header");
		FS.r_close(m_dtFS);
		m_dtFS = nullptr;
		m_bInLoad = FALSE;
		return;
	}

	// Проверка версии формата
	if (m_dtH.version != DETAIL_VERSION)
	{
		Msg("![ERROR] CDetailManager::Internal_Load() - Invalid detail version: %d, expected: %d", m_dtH.version,
			DETAIL_VERSION);
		FS.r_close(m_dtFS);
		m_dtFS = nullptr;
		m_bInLoad = FALSE;
		return;
	}

	Msg("CDetailManager::Internal_Load() - Detail header: objects=%d, size_x=%d, size_z=%d", m_dtH.object_count,
		m_dtH.size_x, m_dtH.size_z);

	// Ограничение количества объектов
	if (m_dtH.object_count > dm_max_objects)
	{
		Msg("CDetailManager::Internal_Load() - Too many objects: %d, clamping to %d", m_dtH.object_count,
			dm_max_objects);
		m_dtH.object_count = dm_max_objects;
	}

	// Загрузка моделей объектов
	u32 m_count = m_dtH.object_count;
	IReader* m_fs = m_dtFS->open_chunk(1);
	if (m_fs)
	{
		for (u32 m_id = 0; m_id < m_count; m_id++)
		{
			CDetail* dt = xr_new<CDetail>();
			IReader* S = m_fs->open_chunk(m_id);
			if (S)
			{
				dt->Load(S);
				m_objects.push_back(dt);
				Msg("CDetailManager::Internal_Load() - Loaded detail object %d", m_id);
				S->close();
			}
		}
		m_fs->close();
	}
	else
	{
		Msg("![ERROR] CDetailManager::Internal_Load() - No model chunk found in details file");
	}

	// Загрузка слотов деталей
	IReader* m_slots = m_dtFS->open_chunk(2);
	if (m_slots)
	{
		m_dtSlots = (DetailSlot*)m_slots->pointer();
		m_slots->close();
		Msg("CDetailManager::Internal_Load() - Loaded %d detail slots", m_dtH.size_x * m_dtH.size_z);
	}
	else
	{
		Msg("![ERROR] CDetailManager::Internal_Load() - No slot chunk found in details file");
		m_dtSlots = nullptr;
	}

	// Инициализация систем видимости
	for (u32 i = 0; i < 3; ++i)
	{
		m_visibles[i].resize(m_objects.size());
		m_render_visibles[i].resize(m_objects.size());
	}

	// Инициализация кэширования
	cache_Initialize();

	// Загрузка аппаратных ресурсов
	hw_Load();

	// Инициализация системы occlusion culling
	if (s_use_occlusion_culling)
	{
		m_occlusion_enabled = true;
		InitializeOcclusionSystem();
	}
	else
	{
		m_occlusion_enabled = false;
	}

	m_initialized = true;
	m_bInLoad = FALSE;

	Msg("CDetailManager::Internal_Load() - Internal loading completed, loaded %d objects", m_objects.size());
#endif
}

/**
 * @brief Выгрузка системы деталей
 * @note Освобождает все ресурсы и останавливает системы
 */
void CDetailManager::Unload()
{
	Msg("CDetailManager::Unload() - Starting detail system unloading");

	// Удаление из параллельных задач
	UnregisterFromParallel();

	if (!m_loaded)
	{
		Msg("CDetailManager::Unload() - Not loaded, skipping");
		return;
	}

	m_MT.Enter();
	try
	{
		Internal_Unload();
		m_loaded = false;
		m_initialized = false;
	}
	catch (...)
	{
		Msg("![ERROR] CDetailManager::Unload() - Exception during unloading");
	}
	m_MT.Leave();

	Msg("CDetailManager::Unload() - Unloading completed");
}

/**
 * @brief Внутренняя выгрузка ресурсов
 * @note Освобождает все внутренние ресурсы
 */
void CDetailManager::Internal_Unload()
{
	Msg("CDetailManager::Internal_Unload() - Starting internal unloading");

	// Очистка системы occlusion culling
	CleanupOcclusionSystem();

	// Выгрузка аппаратных ресурсов
	hw_Unload();

	// Очистка кэша слотов
	for (u32 i = 0; i < dm_cache_size; i++)
	{
		Slot& slot = m_cache_pool[i];
		for (u32 j = 0; j < dm_obj_in_slot; j++)
		{
			SlotPart& sp = slot.G[j];

			// Освобождение элементов слота
			for (auto item : sp.items)
			{
				if (item)
				{
					m_poolSI.destroy(item);
				}
			}
			sp.items.clear();

			// Очистка LOD массивов
			for (int lod = 0; lod < 3; lod++)
				sp.r_items[lod].clear();
		}
	}

	// Освобождение объектов деталей
	for (DetailIt it = m_objects.begin(); it != m_objects.end(); it++)
	{
		if (*it)
		{
			(*it)->Unload();
			xr_delete(*it);
		}
	}
	m_objects.clear();

	// Очистка систем видимости
	for (u32 i = 0; i < 3; ++i)
	{
		m_visibles[i].clear();
		m_render_visibles[i].clear();
	}

	// Очистка задач кэширования
	m_cache_task.clear();

	// Закрытие файлового потока
	if (m_dtFS)
	{
		FS.r_close(m_dtFS);
		m_dtFS = nullptr;
	}

	// Очистка пакетов рендеринга
	m_renderBatches.clear();

	Msg("CDetailManager::Internal_Unload() - Internal unloading completed");
}

// ===========================================================================
// Система видимости и кэширования
// ===========================================================================

/**
 * @brief Обновление видимых объектов
 * @note Определяет какие объекты видны в текущем кадре с учетом frustum и occlusion culling
 */
void CDetailManager::UpdateVisibleM()
{
	if (!g_pGameLevel || !Device.Statistic || !m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	Device.Statistic->RenderDUMP_DT_VIS.Begin();

	// Очистка предыдущих списков видимости
	ClearVisibleLists();

	// Обновление плоскостей frustum'а для отсечения
	UpdateFrustumPlanes();

	// Обновление occlusion тестов если включено
	if (m_occlusion_enabled && s_use_occlusion_culling)
	{
		UpdateOcclusionTests();
	}

	// Обход всех слотов в кэше
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* currentSlot = m_cache[z][x];
			if (!currentSlot || currentSlot->empty)
				continue;

			// Проверка видимости слота через frustum culling
			if (!IsSlotVisible(currentSlot))
				continue;

			// Проверка occlusion culling
			if (IsSlotOccluded(currentSlot))
				continue;

			// Обновление слота если он устарел
			if (Device.dwFrame > currentSlot->frame)
			{
				currentSlot->frame = Device.dwFrame;

				// Обработка всех частей слота
				for (int slot_part_id = 0; slot_part_id < dm_obj_in_slot; slot_part_id++)
				{
					SlotPart& slotPart = currentSlot->G[slot_part_id];
					if (slotPart.id == DetailSlot::ID_Empty)
						continue;

					// Очистка LOD массивов
					for (int lod = 0; lod < 3; lod++)
						slotPart.r_items[lod].clear();

					// Распределение объектов по LOD уровням
					for (auto& item_ptr : slotPart.items)
					{
						if (item_ptr)
						{
							SlotItem& item = *item_ptr;
							item.scale_calculated = item.scale;
							// Все объекты идут в LOD 0 (базовый уровень)
							slotPart.r_items[0].push_back(item_ptr);
						}
					}
				}
			}

			// Добавление видимых объектов в списки рендеринга
			for (int slot_part_id = 0; slot_part_id < dm_obj_in_slot; slot_part_id++)
			{
				SlotPart& slotPart = currentSlot->G[slot_part_id];
				if (slotPart.id == DetailSlot::ID_Empty)
					continue;

				for (int lod = 0; lod < 3; lod++)
				{
					if (!slotPart.r_items[lod].empty())
					{
						m_visibles[lod][slotPart.id].push_back(&slotPart.r_items[lod]);
					}
				}
			}
		}
	}

	Device.Statistic->RenderDUMP_DT_VIS.End();
}

/**
 * @brief Очистка списков видимости
 * @note Подготавливает списки для нового кадра
 */
void CDetailManager::ClearVisibleLists()
{
	for (u32 lod = 0; lod < 3; lod++)
	{
		for (u32 obj = 0; obj < m_objects.size(); obj++)
		{
			m_visibles[lod][obj].clear();
		}
	}
}

/**
 * @brief Копирование списков видимости для рендеринга
 * @note Создает thread-safe копию для использования в рендер потоке
 */
void CDetailManager::CopyVisibleListsForRender()
{
	m_MT.Enter();

	for (u32 lod = 0; lod < 3; lod++)
	{
		for (u32 obj = 0; obj < m_objects.size(); obj++)
		{
			m_render_visibles[lod][obj] = m_visibles[lod][obj];
		}
	}

	m_MT.Leave();
}

// ===========================================================================
// Система кэширования слотов
// ===========================================================================

/**
 * @brief Инициализация системы кэширования
 * @note Создает первоначальную структуру кэша и распределяет слоты
 */
void CDetailManager::cache_Initialize()
{
	Msg("CDetailManager::cache_Initialize() - Initializing cache system");

	m_cache_cx = 0;
	m_cache_cz = 0;

	// Распределение слотов в кэше
	Slot* slot_ptr = m_cache_pool;
	for (u32 i = 0; i < dm_cache_line; i++)
	{
		for (u32 j = 0; j < dm_cache_line; j++, slot_ptr++)
		{
			m_cache[i][j] = slot_ptr;
			cache_Task(j, i, slot_ptr);
		}
	}

	// Инициализация кэша первого уровня
	for (int mz1 = 0; mz1 < dm_cache1_line; mz1++)
	{
		for (int mx1 = 0; mx1 < dm_cache1_line; mx1++)
		{
			CacheSlot1& cache_slot = m_cache_level1[mz1][mx1];
			for (int z = 0; z < dm_cache1_count; z++)
			{
				for (int x = 0; x < dm_cache1_count; x++)
				{
					cache_slot.slots[z * dm_cache1_count + x] =
						&m_cache[mz1 * dm_cache1_count + z][mx1 * dm_cache1_count + x];
				}
			}
		}
	}

	// Обновление границ кэша первого уровня
	UpdateCacheLevel1Bounds();

	Msg("CDetailManager::cache_Initialize() - Cache system initialized: %dx%d slots", dm_cache_line, dm_cache_line);
}

/**
 * @brief Обновление границ кэша первого уровня
 * @note Вычисляет bounding box'ы для групп слотов
 */
void CDetailManager::UpdateCacheLevel1Bounds()
{
	for (int mz1 = 0; mz1 < dm_cache1_line; mz1++)
	{
		for (int mx1 = 0; mx1 < dm_cache1_line; mx1++)
		{
			CacheSlot1& cache_slot = m_cache_level1[mz1][mx1];
			cache_slot.empty = true;
			cache_slot.vis.clear();
			cache_slot.vis.box.invalidate();

			int null_count = 0;
			int valid_count = 0;

			// Обход всех слотов в группе
			for (int i = 0; i < dm_cache1_count * dm_cache1_count; i++)
			{
				if (cache_slot.slots[i] == nullptr)
				{
					null_count++;
					continue;
				}

				Slot* slot_ptr = *cache_slot.slots[i];
				if (!slot_ptr)
				{
					null_count++;
					continue;
				}

				valid_count++;
				Slot& slot = *slot_ptr;

				// Объединение bounding box'ов
				if (cache_slot.vis.box.is_valid())
				{
					cache_slot.vis.box.merge(slot.vis.box);
				}
				else
				{
					cache_slot.vis.box.set(slot.vis.box);
				}

				if (!slot.empty)
					cache_slot.empty = false;
			}

			// Логирование проблемных случаев
			if (null_count > 0)
			{
				Msg("CDetailManager::UpdateCacheLevel1Bounds() - CacheSlot1[%d][%d]: %d null slots, %d valid slots",
					mz1, mx1, null_count, valid_count);
			}

			// Создание fallback bounding box если необходимо
			if (!cache_slot.vis.box.is_valid())
			{
				int world_x = cg2w_X(mx1 * dm_cache1_count);
				int world_z = cg2w_Z(mz1 * dm_cache1_count);

				cache_slot.vis.box.min.set(world_x, 0, world_z);
				cache_slot.vis.box.max.set(world_x + dm_slot_size * dm_cache1_count, 1.0f,
										   world_z + dm_slot_size * dm_cache1_count);
				cache_slot.vis.box.grow(0.1f);
			}

			// Вычисление bounding sphere
			cache_slot.vis.box.getsphere(cache_slot.vis.sphere.P, cache_slot.vis.sphere.R);
		}
	}
}

/**
 * @brief Запрос слота по мировым координатам
 * @param sx - координата X в мире
 * @param sz - координата Z в мире
 * @return указатель на слот или nullptr если не найден
 */
CDetailManager::Slot* CDetailManager::cache_Query(int sx, int sz)
{
	int grid_x = w2cg_X(sx);
	if (grid_x < 0 || grid_x >= dm_cache_line)
		return nullptr;

	int grid_z = w2cg_Z(sz);
	if (grid_z < 0 || grid_z >= dm_cache_line)
		return nullptr;

	return m_cache[grid_z][grid_x];
}

/**
 * @brief Подготовка задачи для слота
 * @param grid_x - координата X в сетке кэша
 * @param grid_z - координата Z в сетке кэша
 * @param slot - указатель на слот для подготовки
 */
void CDetailManager::cache_Task(int grid_x, int grid_z, Slot* slot)
{
	if (!slot)
		return;

	int world_x = cg2w_X(grid_x);
	int world_z = cg2w_Z(grid_z);

	// Запрос данных слота из базы данных
	DetailSlot& detail_slot = QueryDB(world_x, world_z);

	// Инициализация состояния слота
	slot->empty = (detail_slot.id0 == DetailSlot::ID_Empty) && (detail_slot.id1 == DetailSlot::ID_Empty) &&
				  (detail_slot.id2 == DetailSlot::ID_Empty) && (detail_slot.id3 == DetailSlot::ID_Empty);

	slot->type = stReady;
	slot->sx = world_x;
	slot->sz = world_z;

	// Установка bounding volume
	slot->vis.box.min.set(world_x * dm_slot_size, detail_slot.r_ybase(), world_z * dm_slot_size);
	slot->vis.box.max.set(slot->vis.box.min.x + dm_slot_size, detail_slot.r_ybase() + detail_slot.r_yheight(),
						  slot->vis.box.min.z + dm_slot_size);
	slot->vis.box.grow(0.05f);
	slot->vis.box.getsphere(slot->vis.sphere.P, slot->vis.sphere.R);

	// Очистка предыдущих данных
	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		slot->G[i].id = detail_slot.r_id(i);
		for (auto& item : slot->G[i].items)
		{
			if (item)
			{
				m_poolSI.destroy(item);
			}
		}
		slot->G[i].items.clear();
		for (int lod = 0; lod < 3; lod++)
			slot->G[i].r_items[lod].clear();
	}

	// Добавление в задачи если слот не пустой
	if (!slot->empty)
	{
		m_cache_task.push_back(slot);
	}
}

/**
 * @brief Запрос данных слота из базы данных
 * @param sx - координата X в мире
 * @param sz - координата Z в мире
 * @return ссылка на данные слота
 */
DetailSlot& CDetailManager::QueryDB(int sx, int sz)
{
	int db_x = sx + m_dtH.offs_x;
	int db_z = sz + m_dtH.offs_z;

	if ((db_x >= 0) && (db_x < int(m_dtH.size_x)) && (db_z >= 0) && (db_z < int(m_dtH.size_z)))
	{
		u32 linear_id = db_z * m_dtH.size_x + db_x;
		return m_dtSlots[linear_id];
	}
	else
	{
		return m_DS_empty;
	}
}

/**
 * @brief Декомпрессия слота - создание объектов в слоте
 * @param slot - слот для декомпрессии
 */
void CDetailManager::cache_Decompress(Slot* slot)
{
	if (!slot || !m_loaded || !m_initialized || m_bShuttingDown)
		return;

	Slot& decompressing_slot = *slot;
	decompressing_slot.type = stReady;

	// Пропуск если слот уже декомпрессирован
	if (!decompressing_slot.empty && !decompressing_slot.G[0].items.empty())
		return;

	if (decompressing_slot.empty)
		return;

	// Получение данных слота
	DetailSlot& detail_slot = QueryDB(decompressing_slot.sx, decompressing_slot.sz);

	// Подготовка bounding volume для collision detection
	Fvector box_center, box_dimensions;
	decompressing_slot.vis.box.get_CD(box_center, box_dimensions);

	// Запрос статической геометрии для размещения объектов
	m_xrc.box_options(CDB::OPT_FULL_TEST);
	m_xrc.box_query(g_pGameLevel->ObjectSpace.GetStaticModel(), box_center, box_dimensions);
	u32 triangle_count = m_xrc.r_count();

	if (0 == triangle_count)
		return;

	CDB::TRI* triangles = g_pGameLevel->ObjectSpace.GetStaticTris();
	Fvector* vertices = g_pGameLevel->ObjectSpace.GetStaticVerts();

	// Подготовка альфа-каналов для дизеринга
	float alpha_values[dm_obj_in_slot][4];
	for (int i = 0; i < dm_obj_in_slot; i++)
	{
		alpha_values[i][0] = 255.f * float(detail_slot.palette[i].a0) / 15.f;
		alpha_values[i][1] = 255.f * float(detail_slot.palette[i].a1) / 15.f;
		alpha_values[i][2] = 255.f * float(detail_slot.palette[i].a2) / 15.f;
		alpha_values[i][3] = 255.f * float(detail_slot.palette[i].a3) / 15.f;
	}

	// Параметры распределения объектов
	float density = ps_r_Detail_density;
	float jitter = density / 1.7f;
	u32 distribution_size = iCeil(dm_slot_size / density);

	svector<int, dm_obj_in_slot> selected_objects;
	u32 random_seed = decompressing_slot.sx * decompressing_slot.sz;

	// Генераторы случайных чисел для различных параметров
	CRandom selection_rng(0x12071980 ^ random_seed);
	CRandom jitter_rng(0x12071980 ^ random_seed);
	CRandom yaw_rng(0x12071980 ^ random_seed);
	CRandom scale_rng(0x12071980 ^ random_seed);

	Fbox objects_bounds;
	objects_bounds.invalidate();

	// Распределение объектов по сетке
	for (u32 z = 0; z <= distribution_size; z++)
	{
		for (u32 x = 0; x <= distribution_size; x++)
		{
			u32 shift_x = jitter_rng.randI(16);
			u32 shift_z = jitter_rng.randI(16);

			selected_objects.clear();

			// Выбор объектов для размещения на основе альфа-каналов
			for (u32 i = 0; i < dm_obj_in_slot; i++)
			{
				u32 detail_id = GetSlotID(detail_slot, i);
				if (detail_id != DetailSlot::ID_Empty &&
					InterpolateAndDither(alpha_values[i], x, z, shift_x, shift_z, distribution_size))
				{
					selected_objects.push_back(i);
				}
			}

			if (selected_objects.empty())
				continue;

			// Выбор конкретного объекта для размещения
			u32 selected_index;
			if (selected_objects.size() == 1)
				selected_index = selected_objects[0];
			else
				selected_index = selected_objects[selection_rng.randI(selected_objects.size())];

			u32 detail_id = GetSlotID(detail_slot, selected_index);
			if (detail_id >= m_objects.size())
				continue;

			CDetail* detail_object = m_objects[detail_id];
			SlotItem* new_item = m_poolSI.create();
			SlotItem& item = *new_item;

			// Вычисление позиции с дизерингом
			float pos_x = (float(x) / float(distribution_size)) * dm_slot_size + decompressing_slot.vis.box.min.x;
			float pos_z = (float(z) / float(distribution_size)) * dm_slot_size + decompressing_slot.vis.box.min.z;

			Fvector item_position;
			item_position.set(pos_x + jitter_rng.randFs(jitter), decompressing_slot.vis.box.max.y,
							  pos_z + jitter_rng.randFs(jitter));

			// Raycast для размещения на поверхности
			float surface_y = decompressing_slot.vis.box.min.y - 5;
			Fvector ray_direction;
			ray_direction.set(0, -1, 0);

			float hit_u, hit_v, hit_range;
			for (u32 tid = 0; tid < triangle_count; tid++)
			{
				CDB::TRI& triangle = triangles[m_xrc.r_begin()[tid].id];
				Fvector triangle_vertices[3] = {vertices[triangle.verts[0]], vertices[triangle.verts[1]],
												vertices[triangle.verts[2]]};

				if (CDB::TestRayTri(item_position, ray_direction, triangle_vertices, hit_u, hit_v, hit_range, TRUE))
				{
					if (hit_range >= 0)
					{
						float test_y = item_position.y - hit_range;
						if (test_y > surface_y)
							surface_y = test_y;
					}
				}
			}

			// Проверка валидности позиции
			if (surface_y < decompressing_slot.vis.box.min.y)
			{
				m_poolSI.destroy(new_item);
				continue;
			}

			item_position.y = surface_y;

			// Установка параметров объекта
			item.scale = scale_rng.randF(detail_object->m_fMinScale, detail_object->m_fMaxScale);
			item.scale_calculated = item.scale;

			item.mRotY.identity();
			item.mRotY.rotateY(yaw_rng.randF(0, PI_MUL_2));
			item.mRotY.translate_over(item_position);

			// Вычисление bounding volume объекта
			Fmatrix scale_matrix, transform_matrix;
			Fbox item_bounds;
			scale_matrix.scale(item.scale, item.scale, item.scale);
			transform_matrix.mul_43(item.mRotY, scale_matrix);
			item_bounds.xform(detail_object->bv_bb, transform_matrix);
			objects_bounds.merge(item_bounds);

			// Установка освещения
			item.c_hemi = GetSlotHemi(detail_slot);
			item.c_sun = GetSlotSun(detail_slot);
			item.vis_ID = 0;

			// Добавление объекта в слот
			decompressing_slot.G[selected_index].items.push_back(new_item);
		}
	}

	// Обновление bounding volume слота если были добавлены объекты
	if (objects_bounds.is_valid())
	{
		decompressing_slot.vis.box.set(objects_bounds);
		decompressing_slot.vis.box.grow(0.05f);
		decompressing_slot.vis.box.getsphere(decompressing_slot.vis.sphere.P, decompressing_slot.vis.sphere.R);
	}

	// Проверка пустоты слота
	decompressing_slot.empty = true;
	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		if (!decompressing_slot.G[i].items.empty())
		{
			decompressing_slot.empty = false;
			break;
		}
	}
}

/**
 * @brief Обновление системы кэширования
 * @note Перемещает кэш вслед за камерой и обновляет необходимые слоты
 */
void CDetailManager::UpdateCache()
{
	if (m_bShuttingDown || !m_loaded || !m_initialized || m_bInLoad)
		return;

	Fvector camera_position;

	if (g_pGamePersistent)
		camera_position = Device.vCameraPosition;
	else
		return;

	// Проверка валидности позиции камеры
	if (!std::isfinite(camera_position.x) || !std::isfinite(camera_position.y) || !std::isfinite(camera_position.z) ||
		camera_position.square_magnitude() < 0.01f)
		return;

	// Вычисление координат в системе слотов
	int slot_x, slot_z;
	__try
	{
		slot_x = iFloor(camera_position.x / dm_slot_size + .5f);
		slot_z = iFloor(camera_position.z / dm_slot_size + .5f);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return;
	}

	// Ограничение координат
	slot_x = std::max(-1024, std::min(1024, slot_x));
	slot_z = std::max(-1024, std::min(1024, slot_z));

	if (Device.Statistic)
		Device.Statistic->RenderDUMP_DT_Cache.Begin();

	__try
	{
		cache_Update(slot_x, slot_z, camera_position, dm_max_decompress);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Msg("![ERROR] CDetailManager::UpdateCache() - Exception in cache_Update");
	}

	if (Device.Statistic)
		Device.Statistic->RenderDUMP_DT_Cache.End();
}

/**
 * @brief Внутреннее обновление кэша
 * @param target_x, target_z - целевые координаты камеры в системе слотов
 * @param view - позиция камеры
 * @param limit - ограничение на количество декомпрессий
 */
void CDetailManager::cache_Update(int target_x, int target_z, Fvector& view, int limit)
{
	if (m_bShuttingDown || !m_loaded || !m_initialized)
		return;

	// Ограничение координат
	target_x = std::max(-1024, std::min(1024, target_x));
	target_z = std::max(-1024, std::min(1024, target_z));

	// Вычисление смещения камеры
	int delta_x = target_x - m_cache_cx;
	int delta_z = target_z - m_cache_cz;

	// Полная перестройка кэша при большом смещении
	if (abs(delta_x) >= dm_cache_line || abs(delta_z) >= dm_cache_line)
	{
		m_cache_cx = target_x;
		m_cache_cz = target_z;

		Slot* slot_ptr = m_cache_pool;
		for (u32 i = 0; i < dm_cache_line; i++)
		{
			for (u32 j = 0; j < dm_cache_line; j++, slot_ptr++)
			{
				if (m_cache[i][j])
				{
					cache_Task(j, i, m_cache[i][j]);
				}
			}
		}
		m_cache_task.clear();

		UpdateCacheLevel1Bounds();
		return;
	}

	// Постепенное обновление кэша по осям
	while (m_cache_cx != target_x)
	{
		if (target_x > m_cache_cx)
		{
			m_cache_cx++;
			// Сдвиг кэша вправо
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* first_slot = m_cache[z][0];
				if (!first_slot)
					continue;

				for (int x = 1; x < dm_cache_line; x++)
				{
					m_cache[z][x - 1] = m_cache[z][x];
				}
				m_cache[z][dm_cache_line - 1] = first_slot;
				cache_Task(dm_cache_line - 1, z, first_slot);
			}
		}
		else
		{
			m_cache_cx--;
			// Сдвиг кэша влево
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* last_slot = m_cache[z][dm_cache_line - 1];
				if (!last_slot)
					continue;

				for (int x = dm_cache_line - 1; x > 0; x--)
				{
					m_cache[z][x] = m_cache[z][x - 1];
				}
				m_cache[z][0] = last_slot;
				cache_Task(0, z, last_slot);
			}
		}
	}

	while (m_cache_cz != target_z)
	{
		if (target_z > m_cache_cz)
		{
			m_cache_cz++;
			// Сдвиг кэша вниз
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* bottom_slot = m_cache[dm_cache_line - 1][x];
				if (!bottom_slot)
					continue;

				for (int z = dm_cache_line - 1; z > 0; z--)
				{
					m_cache[z][x] = m_cache[z - 1][x];
				}
				m_cache[0][x] = bottom_slot;
				cache_Task(x, 0, bottom_slot);
			}
		}
		else
		{
			m_cache_cz--;
			// Сдвиг кэша вверх
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* top_slot = m_cache[0][x];
				if (!top_slot)
					continue;

				for (int z = 1; z < dm_cache_line; z++)
				{
					m_cache[z - 1][x] = m_cache[z][x];
				}
				m_cache[dm_cache_line - 1][x] = top_slot;
				cache_Task(x, dm_cache_line - 1, top_slot);
			}
		}
	}

	// Обновление границ кэша первого уровня
	if (m_initialized)
	{
		UpdateCacheLevel1Bounds();
	}

	// Обработка задач декомпрессии
	u32 tasks_to_process = m_cache_task.size();
	for (u32 i = 0; i < tasks_to_process; i++)
	{
		Slot* task_slot = m_cache_task[i];
		if (task_slot)
		{
			__try
			{
				cache_Decompress(task_slot);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				Msg("![ERROR] CDetailManager::cache_Update() - cache_Decompress failed for slot");
			}
		}
	}

	m_cache_task.clear();
}

/**
 * @brief Валидация состояния кэша
 * @return TRUE если кэш в валидном состоянии
 */
BOOL CDetailManager::cache_Validate()
{
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			int world_x = cg2w_X(x);
			int world_z = cg2w_Z(z);
			Slot* slot = m_cache[z][x];

			if (!slot || slot->sx != world_x)
				return FALSE;
			if (slot->sz != world_z)
				return FALSE;
		}
	}
	return TRUE;
}

// ===========================================================================
// Frustum Culling System
// ===========================================================================

/**
 * @brief Обновление плоскостей frustum'а
 * @note Вычисляет плоскости из матрицы полного преобразования камеры
 */
void CDetailManager::UpdateFrustumPlanes()
{
	Fmatrix& full_transform = Device.mFullTransform;

	// Левая плоскость
	m_frustum_planes[FRUSTUM_LEFT].n.x = full_transform._14 + full_transform._11;
	m_frustum_planes[FRUSTUM_LEFT].n.y = full_transform._24 + full_transform._21;
	m_frustum_planes[FRUSTUM_LEFT].n.z = full_transform._34 + full_transform._31;
	m_frustum_planes[FRUSTUM_LEFT].d = full_transform._44 + full_transform._41;

	// Правая плоскость
	m_frustum_planes[FRUSTUM_RIGHT].n.x = full_transform._14 - full_transform._11;
	m_frustum_planes[FRUSTUM_RIGHT].n.y = full_transform._24 - full_transform._21;
	m_frustum_planes[FRUSTUM_RIGHT].n.z = full_transform._34 - full_transform._31;
	m_frustum_planes[FRUSTUM_RIGHT].d = full_transform._44 - full_transform._41;

	// Верхняя плоскость
	m_frustum_planes[FRUSTUM_TOP].n.x = full_transform._14 - full_transform._12;
	m_frustum_planes[FRUSTUM_TOP].n.y = full_transform._24 - full_transform._22;
	m_frustum_planes[FRUSTUM_TOP].n.z = full_transform._34 - full_transform._32;
	m_frustum_planes[FRUSTUM_TOP].d = full_transform._44 - full_transform._42;

	// Нижняя плоскость
	m_frustum_planes[FRUSTUM_BOTTOM].n.x = full_transform._14 + full_transform._12;
	m_frustum_planes[FRUSTUM_BOTTOM].n.y = full_transform._24 + full_transform._22;
	m_frustum_planes[FRUSTUM_BOTTOM].n.z = full_transform._34 + full_transform._32;
	m_frustum_planes[FRUSTUM_BOTTOM].d = full_transform._44 + full_transform._42;

	// Ближняя плоскость
	m_frustum_planes[FRUSTUM_NEAR].n.x = full_transform._13;
	m_frustum_planes[FRUSTUM_NEAR].n.y = full_transform._23;
	m_frustum_planes[FRUSTUM_NEAR].n.z = full_transform._33;
	m_frustum_planes[FRUSTUM_NEAR].d = full_transform._43;

	// Дальняя плоскость
	m_frustum_planes[FRUSTUM_FAR].n.x = full_transform._14 - full_transform._13;
	m_frustum_planes[FRUSTUM_FAR].n.y = full_transform._24 - full_transform._23;
	m_frustum_planes[FRUSTUM_FAR].n.z = full_transform._34 - full_transform._33;
	m_frustum_planes[FRUSTUM_FAR].d = full_transform._44 - full_transform._43;

	// Нормализация всех плоскостей
	for (int i = 0; i < FRUSTUM_PLANES_COUNT; i++)
	{
		float length = 1.0f / m_frustum_planes[i].n.magnitude();
		m_frustum_planes[i].n.x *= length;
		m_frustum_planes[i].n.y *= length;
		m_frustum_planes[i].n.z *= length;
		m_frustum_planes[i].d *= length;
	}
}

/**
 * @brief Проверка сферы на видимость во frustum'е
 * @param center - центр сферы
 * @param radius - радиус сферы
 * @return true если сфера видима
 */
bool CDetailManager::IsSphereInsideFrustum(const Fvector& center, float radius) const
{
	for (int i = 0; i < FRUSTUM_PLANES_COUNT; i++)
	{
		float distance = m_frustum_planes[i].classify(center);
		if (distance < -radius)
			return false;
	}
	return true;
}

/**
 * @brief Проверка AABB на видимость во frustum'е
 * @param min - минимальная точка AABB
 * @param max - максимальная точка AABB
 * @return true если AABB видим
 */
bool CDetailManager::IsAABBInsideFrustum(const Fvector& min, const Fvector& max) const
{
	for (int i = 0; i < FRUSTUM_PLANES_COUNT; i++)
	{
		// Поиск наиболее удаленной точки в направлении нормали плоскости
		Fvector positive_point;
		positive_point.x = (m_frustum_planes[i].n.x > 0) ? max.x : min.x;
		positive_point.y = (m_frustum_planes[i].n.y > 0) ? max.y : min.y;
		positive_point.z = (m_frustum_planes[i].n.z > 0) ? max.z : min.z;

		if (m_frustum_planes[i].classify(positive_point) < 0)
			return false;
	}
	return true;
}

/**
 * @brief Проверка видимости слота
 * @param slot - слот для проверки
 * @return true если слот видим
 */
bool CDetailManager::IsSlotVisible(const Slot* slot) const
{
	if (!slot || slot->empty)
		return false;

	// Быстрая проверка со сферой
	if (!IsSphereInsideFrustum(slot->vis.sphere.P, slot->vis.sphere.R))
		return false;

	// Точная проверка с AABB
	return IsAABBInsideFrustum(slot->vis.box.min, slot->vis.box.max);
}

// ===========================================================================
// Вспомогательные методы
// ===========================================================================

/**
 * @brief Интерполяция и дизеринг для распределения объектов
 */
bool CDetailManager::InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size)
{
	clamp(x, (u32)0, size - 1);
	clamp(y, (u32)0, size - 1);
	int c = iFloor(Interpolate(alpha255, x, y, size) + .5f);
	clamp(c, 0, 255);

	u32 row = (y + sy) % 16;
	u32 col = (x + sx) % 16;
	return c > dither[col][row];
}

/**
 * @brief Создание матрицы дизеринга
 */
void CDetailManager::bwdithermap(int levels)
{
	float N = 255.0f / (levels - 1);
	float magicfact = (N - 1) / 16;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				for (int l = 0; l < 4; l++)
				{
					dither[4 * k + i][4 * l + j] =
						(int)(0.5 + magic4x4[i][j] * magicfact + (magic4x4[k][l] / 16.) * magicfact);
				}
			}
		}
	}
}

// ===========================================================================
// Безопасные методы обновления
// ===========================================================================

void CDetailManager::SafeUpdate()
{
	Update();
}

void CDetailManager::OnDeviceResetBegin()
{
	Msg("CDetailManager::OnDeviceResetBegin()");
	m_MT.Enter();
	hw_Unload();
	CleanupOcclusionSystem();
}

void CDetailManager::OnDeviceResetEnd()
{
	Msg("CDetailManager::OnDeviceResetEnd()");
	if (m_loaded && !m_bShuttingDown)
	{
		hw_Load();
		if (s_use_occlusion_culling)
		{
			InitializeOcclusionSystem();
		}
	}
	m_MT.Leave();
}

// ===========================================================================
// Система Occlusion Culling - недостающие реализации
// ===========================================================================

/**
 * @brief Инициализация системы occlusion culling
 * @note Настраивает методы occlusion culling в зависимости от возможностей hardware
 */
void CDetailManager::InitializeOcclusionSystem()
{
	if (!s_use_occlusion_culling)
	{
		m_occlusion_enabled = false;
		return;
	}

	Msg("CDetailManager::InitializeOcclusionSystem() - Initializing occlusion system");

	// Определяем доступные методы occlusion culling
	DetectAvailableOcclusionMethods();

	// Загружаем геометрию для occlusion тестов
	LoadOcclusionGeometry();

	// Загружаем порталы для occlusion тестов
	LoadOcclusionPortals();

	// Инициализируем программный растеризатор
	InitializeSoftwareRasterizer();

	m_occlusion_enabled = true;
	m_occlusion_frame = 0;

	Msg("CDetailManager::InitializeOcclusionSystem() - Occlusion system initialized with method: %d",
		m_primary_occlusion_method);
}

/**
 * @brief Очистка системы occlusion culling
 * @note Освобождает все ресурсы, связанные с occlusion culling
 */
void CDetailManager::CleanupOcclusionSystem()
{
	Msg("CDetailManager::CleanupOcclusionSystem() - Cleaning up occlusion system");

	// Освобождаем аппаратные occlusion queries
	for (int z = 0; z < dm_cache1_line; z++)
	{
		for (int x = 0; x < dm_cache1_line; x++)
		{
			CacheSlot1& cache1 = m_cache_level1[z][x];
			_RELEASE(cache1.occlusion_query);
		}
	}

	for (u32 i = 0; i < dm_cache_size; i++)
	{
		Slot& slot = m_cache_pool[i];
		_RELEASE(slot.occlusion_query);
	}

	// Очищаем программный растеризатор
	CleanupSoftwareRasterizer();

	// Освобождаем геометрию occlusion тестов
	m_occlusion_geom.destroy();
	_RELEASE(m_occlusion_ib);
	_RELEASE(m_occlusion_vb);

	// Очищаем данные occlusion системы
	m_occlusion_geometries.clear();
	m_simple_occluders.clear();
	m_occlusion_portals.clear();
	m_occlusion_queries.clear();
	m_occlusion_test_slots.clear();
	m_occlusion_test_cache1.clear();

	m_occlusion_enabled = false;

	Msg("CDetailManager::CleanupOcclusionSystem() - Occlusion system cleaned up");
}

/**
 * @brief Обновление occlusion тестов
 * @note Выполняет occlusion тесты для слотов и кэшей первого уровня
 */
void CDetailManager::UpdateOcclusionTests()
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return;

	m_occlusion_frame++;

	// Обновляем тесты для кэша первого уровня
	for (int z = 0; z < dm_cache1_line; z++)
	{
		for (int x = 0; x < dm_cache1_line; x++)
		{
			CacheSlot1& cache1 = m_cache_level1[z][x];

			if (cache1.empty)
				continue;

			// Проверяем необходимость тестирования
			if ((m_occlusion_frame - cache1.last_occlusion_test_frame) < s_occlusion_frame_interval)
				continue;

			cache1.last_occlusion_test_frame = m_occlusion_frame;

			// Выполняем occlusion тест
			if (IsAABBInsideFrustum(cache1.vis.box.min, cache1.vis.box.max))
			{
				cache1.occlusion_visible = !PerformOcclusionTest(&cache1);
				if (cache1.occlusion_visible)
				{
					cache1.occlusion_visible_frames++;
				}
				else
				{
					cache1.occlusion_visible_frames = 0;
				}
			}
			else
			{
				cache1.occlusion_visible = false;
				cache1.occlusion_visible_frames = 0;
			}
		}
	}

	// Обновляем тесты для отдельных слотов
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (!slot || slot->empty)
				continue;

			// Проверяем расстояние до камеры
			Fvector camera_pos = Device.vCameraPosition;
			float distance = camera_pos.distance_to(slot->vis.sphere.P);
			if (distance > s_max_occlusion_distance)
				continue;

			// Проверяем необходимость тестирования
			if ((m_occlusion_frame - slot->last_occlusion_test_frame) < s_occlusion_frame_interval)
				continue;

			slot->last_occlusion_test_frame = m_occlusion_frame;

			// Выполняем occlusion тест
			if (IsSlotVisible(slot))
			{
				slot->occlusion_visible = !PerformOcclusionTest(slot);
				if (slot->occlusion_visible)
				{
					slot->occlusion_visible_frames++;
				}
				else
				{
					slot->occlusion_visible_frames = 0;
				}
			}
			else
			{
				slot->occlusion_visible = false;
				slot->occlusion_visible_frames = 0;
			}
		}
	}
}

/**
 * @brief Рендеринг occlusion queries
 * @note Обрабатывает результаты аппаратных occlusion queries
 */
void CDetailManager::RenderOcclusionQueries()
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return;

	// В данной реализации используем software методы,
	// поэтому аппаратные queries не обрабатываем
	ProcessOcclusionResults();
}

/**
 * @brief Проверка occlusion для слота
 * @param slot - слот для проверки
 * @return true если слот occlusion'нут
 */
bool CDetailManager::IsSlotOccluded(Slot* slot)
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return false;

	if (!slot || slot->empty)
		return true;

	// Используем выбранный метод occlusion culling
	switch (m_primary_occlusion_method)
	{
	case OCCLUSION_HARDWARE_QUERY:
		return IsSlotHardwareOccluded(slot);

	case OCCLUSION_GEOMETRY:
	case OCCLUSION_PORTALS:
	case OCCLUSION_RASTER:
	case OCCLUSION_HYBRID:
	default:
		return PerformOcclusionTest(slot);
	}
}

// ===========================================================================
// Вспомогательные методы Occlusion System
// ===========================================================================

/**
 * @brief Определение доступных методов occlusion culling
 */
void CDetailManager::DetectAvailableOcclusionMethods()
{
	m_primary_occlusion_method = OCCLUSION_NONE;
	m_fallback_occlusion_method = OCCLUSION_NONE;

	// Проверяем поддержку аппаратных occlusion queries
	bool hardware_queries_available = false;
	IDirect3DQuery9* test_query = nullptr;
	if (SUCCEEDED(HW.pDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &test_query)))
	{
		hardware_queries_available = true;
		_RELEASE(test_query);
	}

	// Выбираем методы на основе доступности и предпочтений
	if (hardware_queries_available && s_preferred_occlusion_method == OCCLUSION_HARDWARE_QUERY)
	{
		m_primary_occlusion_method = OCCLUSION_HARDWARE_QUERY;
	}
	else
	{
		// Используем гибридный подход как fallback
		m_primary_occlusion_method = OCCLUSION_HYBRID;
	}

	m_fallback_occlusion_method = OCCLUSION_RASTER;

	Msg("CDetailManager::DetectAvailableOcclusionMethods() - Primary: %d, Fallback: %d", m_primary_occlusion_method,
		m_fallback_occlusion_method);
}

/**
 * @brief Загрузка геометрии для occlusion тестов
 */
void CDetailManager::LoadOcclusionGeometry()
{
	// В данной реализации создаем упрощенные occluders
	BuildSimpleOccluders();
}

/**
 * @brief Создание упрощенных occluders
 */
void CDetailManager::BuildSimpleOccluders()
{
	m_simple_occluders.clear();

	// Создаем несколько тестовых occluders
	Fvector occluder1_min = {-50, 0, -50};
	Fvector occluder1_max = {50, 50, 50};
	m_simple_occluders.push_back(occluder1_min);
	m_simple_occluders.push_back(occluder1_max);

	Fvector occluder2_min = {100, 0, -25};
	Fvector occluder2_max = {150, 30, 25};
	m_simple_occluders.push_back(occluder2_min);
	m_simple_occluders.push_back(occluder2_max);

	Msg("CDetailManager::BuildSimpleOccluders() - Created %d simple occluders", m_simple_occluders.size() / 2);
}

/**
 * @brief Загрузка порталов для occlusion тестов
 */
void CDetailManager::LoadOcclusionPortals()
{
	// В данной реализации порталы не используются
	m_occlusion_portals.clear();
}

/**
 * @brief Инициализация программного растеризатора
 */
void CDetailManager::InitializeSoftwareRasterizer()
{
	// В данной реализации программный растеризатор не используется
	m_software_depth_buffer = nullptr;
	m_software_depth_width = 0;
	m_software_depth_height = 0;
	m_software_depth_scale = 1.0f;
}

/**
 * @brief Обновление буфера глубины программного растеризатора
 */
void CDetailManager::UpdateSoftwareDepthBuffer()
{
	// В данной реализации не используется
}

/**
 * @brief Очистка программного растеризатора
 */
void CDetailManager::CleanupSoftwareRasterizer()
{
	if (m_software_depth_buffer)
	{
		xr_free(m_software_depth_buffer);
		m_software_depth_buffer = nullptr;
	}
}

/**
 * @brief Тестирование геометрического occlusion
 */
bool CDetailManager::TestGeometryOcclusion(const Fvector& test_point, float test_radius) const
{
	// Простая реализация - проверяем пересечение с упрощенными occluders
	return TestSimpleOccluders(test_point, test_radius);
}

/**
 * @brief Тестирование с упрощенными occluders
 */
bool CDetailManager::TestSimpleOccluders(const Fvector& test_point, float test_radius) const
{
	Fvector camera_pos = Device.vCameraPosition;

	for (size_t i = 0; i < m_simple_occluders.size(); i += 2)
	{
		const Fvector& min = m_simple_occluders[i];
		const Fvector& max = m_simple_occluders[i + 1];

		// Проверяем, находится ли тестовая точка за occluder'ом относительно камеры
		Fvector dir = Fvector().sub(test_point, camera_pos);
		float distance = dir.magnitude();
		dir.normalize();

		Fvector intersection;
		Fbox test_box;
		test_box.set(min, max);

		// Простая проверка пересечения луча с bounding box
		if (Fbox::rpOriginOutside == test_box.Pick2(camera_pos, dir, intersection))
		{
			float dist_to_intersection = camera_pos.distance_to(intersection);
			if (dist_to_intersection < distance - test_radius)
			{
				return true;
			}
		}
	}

	return false;
}

/**
 * @brief Тестирование portal-based occlusion
 */
bool CDetailManager::TestPortalOcclusion(const Fvector& test_point, float test_radius) const
{
	// В данной реализации portal-based occlusion не используется
	return false;
}

/**
 * @brief Проверка точки относительно портала
 */
bool CDetailManager::IsPointBehindPortal(const Fvector& point, const OcclusionPortal& portal) const
{
	// В данной реализации не используется
	return false;
}

/**
 * @brief Тестирование программного occlusion
 */
bool CDetailManager::TestSoftwareOcclusion(const Fvector& test_point, float test_radius) const
{
	// Простая реализация на основе raycast
	Fvector camera_pos = Device.vCameraPosition;
	float test_distance = camera_pos.distance_to(test_point);

	// Raycast для проверки видимости
	collide::rq_result RQ;
	Fvector dir_to_test = Fvector().sub(test_point, camera_pos);
	float dir_length = dir_to_test.magnitude();
	dir_to_test.normalize();

	if (g_pGameLevel->ObjectSpace.RayPick(camera_pos, dir_to_test, dir_length, collide::rqtStatic, RQ, nullptr))
	{
		// Если есть пересечение ближе тестовой точки
		if (RQ.range < test_distance - test_radius)
		{
			return true;
		}
	}

	return false;
}

/**
 * @brief Растеризация occlusion геометрии
 */
void CDetailManager::RasterizeOcclusionGeometry()
{
	// В данной реализации не используется
}

/**
 * @brief Выполнение occlusion теста для слота
 */
bool CDetailManager::PerformOcclusionTest(Slot* slot)
{
	if (!slot || slot->empty)
		return true;

	Fvector test_point = slot->vis.sphere.P;
	float test_radius = slot->vis.sphere.R * s_occlusion_padding_scale;

	// Используем комбинированный подход
	bool occluded = TestSimpleOccluders(test_point, test_radius) || TestSoftwareOcclusion(test_point, test_radius);

	slot->occlusion_confidence = CalculateOcclusionConfidence(test_point, m_primary_occlusion_method);
	return occluded;
}

/**
 * @brief Выполнение occlusion теста для кэша первого уровня
 */
bool CDetailManager::PerformOcclusionTest(CacheSlot1* cache1)
{
	if (!cache1 || cache1->empty)
		return true;

	Fvector test_point = cache1->vis.sphere.P;
	float test_radius = cache1->vis.sphere.R * s_occlusion_padding_scale;

	bool occluded = TestSimpleOccluders(test_point, test_radius) || TestSoftwareOcclusion(test_point, test_radius);

	cache1->occlusion_confidence = CalculateOcclusionConfidence(test_point, m_primary_occlusion_method);
	return occluded;
}

/**
 * @brief Расчет уверенности в результате occlusion теста
 */
float CDetailManager::CalculateOcclusionConfidence(const Fvector& test_point, EOcclusionMethod method) const
{
	Fvector camera_pos = Device.vCameraPosition;
	float distance = camera_pos.distance_to(test_point);

	float base_confidence = 0.0f;

	switch (method)
	{
	case OCCLUSION_HARDWARE_QUERY:
		base_confidence = 0.9f;
		break;
	case OCCLUSION_GEOMETRY:
		base_confidence = 0.8f;
		break;
	case OCCLUSION_RASTER:
		base_confidence = 0.7f;
		break;
	case OCCLUSION_HYBRID:
		base_confidence = 0.85f;
		break;
	default:
		base_confidence = 0.5f;
	}

	// Уверенность уменьшается с расстоянием
	float distance_factor = 1.0f - (distance / s_max_occlusion_distance);
	clamp(distance_factor, 0.1f, 1.0f);

	return base_confidence * distance_factor;
}

/**
 * @brief Проверка аппаратного occlusion для слота
 */
bool CDetailManager::IsSlotHardwareOccluded(Slot* slot)
{
	// В данной реализации аппаратные queries не используются
	// Всегда возвращаем false, чтобы не occlusion'ить слоты
	return false;
}

/**
 * @brief Проверка аппаратного occlusion для кэша первого уровня
 */
bool CDetailManager::IsCache1HardwareOccluded(CacheSlot1* cache1)
{
	// В данной реализации аппаратные queries не используются
	return false;
}

/**
 * @brief Обработка результатов occlusion тестов
 */
void CDetailManager::ProcessOcclusionResults()
{
	// В данной реализации не требуется дополнительной обработки
}

/**
 * @brief Обновление аппаратных occlusion тестов
 */
void CDetailManager::UpdateHardwareOcclusionTests()
{
	// В данной реализации аппаратные occlusion queries не используются
}

/**
 * @brief Обработка результатов аппаратных occlusion queries
 */
void CDetailManager::ProcessHardwareOcclusionResults()
{
	// В данной реализации аппаратные occlusion queries не используются
}

// ===========================================================================
// Методы для отладки и статистики
// ===========================================================================

/**
 * @brief Получение количества occlusion'нутых слотов
 */
u32 CDetailManager::GetOccludedSlotCount() const
{
	u32 count = 0;
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (slot && !slot->empty && !slot->occlusion_visible)
				count++;
		}
	}
	return count;
}

/**
 * @brief Получение количества видимых слотов
 */
u32 CDetailManager::GetVisibleSlotCount() const
{
	u32 count = 0;
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (slot && !slot->empty && slot->occlusion_visible)
				count++;
		}
	}
	return count;
}

/**
 * @brief Вывод статистики occlusion system
 */
void CDetailManager::DumpOcclusionStats()
{
	u32 total_slots = 0;
	u32 visible_slots = 0;
	u32 occluded_slots = 0;

	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (slot && !slot->empty)
			{
				total_slots++;
				if (slot->occlusion_visible)
					visible_slots++;
				else
					occluded_slots++;
			}
		}
	}

	Msg("CDetailManager Occlusion Stats:");
	Msg("  Total slots: %d", total_slots);
	Msg("  Visible slots: %d (%.1f%%)", visible_slots, total_slots > 0 ? (visible_slots * 100.0f / total_slots) : 0.0f);
	Msg("  Occluded slots: %d (%.1f%%)", occluded_slots,
		total_slots > 0 ? (occluded_slots * 100.0f / total_slots) : 0.0f);
	Msg("  Occlusion method: %d", m_primary_occlusion_method);
}

/**
 * @brief Установка метода occlusion culling
 */
void CDetailManager::SetOcclusionMethod(EOcclusionMethod method)
{
	if (method >= OCCLUSION_METHOD_COUNT)
		return;

	s_preferred_occlusion_method = method;

	// Переинициализируем систему если она уже была инициализирована
	if (m_occlusion_enabled)
	{
		CleanupOcclusionSystem();
		InitializeOcclusionSystem();
	}
}