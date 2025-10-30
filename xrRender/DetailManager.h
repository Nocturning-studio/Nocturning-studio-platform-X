////////////////////////////////////////////////////////////////////////////
//	Modified	: 28.10.2025
//	Author	: NSDeathMan
//  Nocturning studio for NS Platform X
//  Complete rewrite with thread-safe architecture
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

const int dm_max_decompress = 7;
const int dm_size = 24;									  //!
const int dm_cache1_count = 4;							  //
const int dm_cache1_line = dm_size * 2 / dm_cache1_count; //! dm_size*2 must be div dm_cache1_count
const int dm_max_objects = 64;
const int dm_obj_in_slot = 4;
const int dm_cache_line = dm_size + 1 + dm_size;
const int dm_cache_size = dm_cache_line * dm_cache_line;
const float dm_fade = float(2 * dm_size) - .5f;
const float dm_slot_size = DETAIL_SLOT_SIZE;

// Forward declarations
struct SlotItem;

// Multi-method occlusion culling system
enum EOcclusionMethod
{
	OCCLUSION_NONE = 0,
	OCCLUSION_HOM,			  // Hardware Occlusion Mapping (если доступен)
	OCCLUSION_HARDWARE_QUERY, // Hardware occlusion queries
	OCCLUSION_GEOMETRY,		  // Geometry-based occlusion (статика)
	OCCLUSION_PORTALS,		  // Portal-based occlusion
	OCCLUSION_RASTER,		  // Software rasterizer
	OCCLUSION_HYBRID,		  // Комбинированный подход
	OCCLUSION_METHOD_COUNT
};

class CDetailManager
{
  private:
	// Types declarations FIRST
	typedef xr_vector<SlotItem*> SlotItemVec;
	typedef SlotItemVec::iterator SlotItemVecIt;

	struct SlotPart
	{
		u32 id;
		SlotItemVec items;
		SlotItemVec r_items[3];

		SlotPart() : id(DetailSlot::ID_Empty)
		{
		}
	};

	enum SlotType
	{
		stReady = 0,
		stPending,
		stFORCEDWORD = 0xffffffff
	};

	struct Slot
	{
		struct
		{
			u32 empty : 1;
			u32 type : 1;
			u32 frame : 30;
		};
		int sx, sz;
		vis_data vis;
		SlotPart G[dm_obj_in_slot];

		// Multi-method occlusion data
		IDirect3DQuery9* occlusion_query;
		u32 last_occlusion_test_frame;
		u32 occlusion_visible_frames;
		bool occlusion_visible;
		float occlusion_confidence; // Уверенность в результате occlusion test

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

	struct CacheSlot1
	{
		u32 empty;
		vis_data vis;
		Slot** slots[dm_cache1_count * dm_cache1_count];

		// Multi-method occlusion data
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

	// Instance data structure for vertex stream instancing
	struct InstanceData
	{
		Fvector4 mat0;
		Fvector4 mat1;
		Fvector4 mat2;
		Fvector4 color;
		float lod_id;

		InstanceData()
		{
			mat0.set(0, 0, 0, 0);
			mat1.set(0, 0, 0, 0);
			mat2.set(0, 0, 0, 0);
			color.set(1, 1, 1, 1);
			lod_id = 0;
		}
	};

	// Frustum planes for culling
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

	// Multi-method occlusion culling system
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

	// Portal-based occlusion data
	struct OcclusionPortal
	{
		Fvector center;
		float radius;
		xr_vector<Fvector> polygon; // Portal polygon
		bool active;

		OcclusionPortal() : active(false)
		{
		}
	};

	// Types
	typedef xr_vector<xr_vector<SlotItemVec*>> vis_list;
	typedef svector<CDetail*, dm_max_objects> DetailVec;
	typedef DetailVec::iterator DetailIt;
	typedef poolSS<SlotItem, 4096> PSS;

  private:
	int dither[16][16];

	// Core data
	IReader* m_dtFS;
	DetailHeader m_dtH;
	DetailSlot* m_dtSlots;
	DetailSlot m_DS_empty;

	// Objects and visibility
	DetailVec m_objects;
	vis_list m_visibles[3];
	vis_list m_render_visibles[3]; // Thread-safe copy for rendering

	// Cache system
#ifndef _EDITOR
	xrXRC m_xrc;
#endif
	CacheSlot1 m_cache_level1[dm_cache1_line][dm_cache1_line];
	Slot* m_cache[dm_cache_line][dm_cache_line];
	xr_vector<Slot*> m_cache_task;
	Slot m_cache_pool[dm_cache_size];
	int m_cache_cx;
	int m_cache_cz;

	// Multi-method occlusion system
	xr_vector<IDirect3DQuery9*> m_occlusion_queries;
	xr_vector<Slot*> m_occlusion_test_slots;
	xr_vector<CacheSlot1*> m_occlusion_test_cache1;
	u32 m_occlusion_frame;
	bool m_occlusion_enabled;
	EOcclusionMethod m_primary_occlusion_method;
	EOcclusionMethod m_fallback_occlusion_method;

	// Geometry-based occlusion
	xr_vector<OcclusionGeometry> m_occlusion_geometries;
	xr_vector<Fvector> m_simple_occluders; // Упрощенные боксы для быстрой проверки

	// Portal-based occlusion
	xr_vector<OcclusionPortal> m_occlusion_portals;

	// Software rasterizer data
	u32* m_software_depth_buffer;
	u32 m_software_depth_width;
	u32 m_software_depth_height;
	float m_software_depth_scale;

	// Memory pool
	PSS m_poolSI;

	// Hardware resources
	ref_geom m_hw_Geom;
	ref_geom m_hw_Geom_Instanced;
	u32 m_hw_BatchSize;
	IDirect3DVertexBuffer9* m_hw_VB;
	IDirect3DVertexBuffer9* m_hw_VB_Instances;
	IDirect3DIndexBuffer9* m_hw_IB;

	// Occlusion test geometry
	ref_geom m_occlusion_geom;
	IDirect3DVertexBuffer9* m_occlusion_vb;
	IDirect3DIndexBuffer9* m_occlusion_ib;

	// Threading and state
	xrCriticalSection m_MT;
	xrCriticalSection m_VisibleMT; // For visibility synchronization
	volatile u32 m_frame_calc;
	volatile u32 m_frame_rendered;
	bool m_initialized;
	bool m_loaded;
	volatile BOOL m_bInLoad;
	volatile BOOL m_bShuttingDown;
	volatile LONG m_bUpdating; // Prevent concurrent updates
	bool m_bRegisteredInParallel;

	Fmatrix m_currentView;
	Fmatrix m_currentProject;
	Fmatrix m_currentWorldViewProject;
	void UpdateMatrices();

  public:
	// Static configuration
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
	// Unregister from parallel tasks
	void UnregisterFromParallel();

	// Internal methods
	void Internal_Load();
	void Internal_Unload();
	void Internal_Render();

	// Visibility and cache management
	void UpdateVisibleM();
	void ClearVisibleLists();
	void CopyVisibleListsForRender();

	// Frustum culling methods
	void UpdateFrustumPlanes();
	bool IsSphereInsideFrustum(const Fvector& center, float radius) const;
	bool IsAABBInsideFrustum(const Fvector& min, const Fvector& max) const;
	bool IsSlotVisible(const Slot* slot) const;

	// Multi-method occlusion culling system
	void InitializeOcclusionSystem();
	void CleanupOcclusionSystem();
	void DetectAvailableOcclusionMethods();
	EOcclusionMethod SelectOcclusionMethod() const;

	// Geometry-based occlusion methods
	void LoadOcclusionGeometry();
	void BuildSimpleOccluders();
	bool TestGeometryOcclusion(const Fvector& test_point, float test_radius) const;
	bool TestSimpleOccluders(const Fvector& test_point, float test_radius) const;

	// Portal-based occlusion methods
	void LoadOcclusionPortals();
	bool TestPortalOcclusion(const Fvector& test_point, float test_radius) const;
	bool IsPointBehindPortal(const Fvector& point, const OcclusionPortal& portal) const;

	// Hardware query methods
	void UpdateHardwareOcclusionTests();
	void ProcessHardwareOcclusionResults();
	bool IsSlotHardwareOccluded(Slot* slot);
	bool IsCache1HardwareOccluded(CacheSlot1* cache1);
	void RenderHardwareOcclusionTest(const Fvector& center, float radius);
	void RenderHardwareOcclusionTest(const Fvector& min, const Fvector& max);

	// Software rasterizer methods
	void InitializeSoftwareRasterizer();
	void CleanupSoftwareRasterizer();
	void UpdateSoftwareDepthBuffer();
	bool TestSoftwareOcclusion(const Fvector& test_point, float test_radius) const;
	void RasterizeOcclusionGeometry();

	// Hybrid occlusion testing
	bool PerformOcclusionTest(Slot* slot);
	bool PerformOcclusionTest(CacheSlot1* cache1);
	float CalculateOcclusionConfidence(const Fvector& test_point, EOcclusionMethod method) const;

	// Unified occlusion interface
	bool IsSlotOccluded(Slot* slot);
	bool IsCache1Occluded(CacheSlot1* cache1);
	void UpdateOcclusionTests();
	void ProcessOcclusionResults();
	void RenderOcclusionQueries();

	// Cache system
	void UpdateCacheLevel1Bounds();
	void cache_Initialize();
	void cache_Update(int sx, int sz, Fvector& view, int limit);
	void cache_Task(int gx, int gz, Slot* D);
	Slot* cache_Query(int sx, int sz);
	void cache_Decompress(Slot* D);
	BOOL cache_Validate();
	DetailSlot& QueryDB(int sx, int sz);
	void bwdithermap(int levels);

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

	// Coordinate transformations
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

	// Hardware rendering methods
	void hw_Load();
	void hw_Unload();
	void hw_Render();
	void hw_Render_dump();

	// Vertex formats for instancing
	static D3DVERTEXELEMENT9* GetDetailsVertexDecl();
	static D3DVERTEXELEMENT9* GetDetailsInstancedVertexDecl();

	// Occlusion vertex format
	static D3DVERTEXELEMENT9* GetOcclusionVertexDecl();

	// Vertex structure
#pragma pack(push, 1)
	struct vertHW
	{
		float x, y, z;
		short u, v, t, mid;
	};
#pragma pack(pop)

	// Helper methods
	short QuantConstant(float v);

  public:
	// Public interface
	CDetailManager();
	virtual ~CDetailManager();

	// Main public methods
	void Load();
	void Unload();
	void Render();
	void __stdcall Update();
	void UpdateCache();
	void SafeUpdate();

	// Device management
	void OnDeviceResetBegin();
	void OnDeviceResetEnd();

	// Shutdown method
	void Shutdown();

	IC bool IsShuttingDown() const
	{
		return m_bShuttingDown;
	}

	// Utility
	IC bool UseVS()
	{
		return HW.Caps.geometry_major >= 1;
	}

	// Multi-method occlusion culling control
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

	// Debug and statistics
	u32 GetOccludedSlotCount() const;
	u32 GetVisibleSlotCount() const;
	void DumpOcclusionStats();

#ifdef _EDITOR
	virtual ObjectList* GetSnapList() = 0;
#endif

  private:
	// Prevent copying
	CDetailManager(const CDetailManager&);
	CDetailManager& operator=(const CDetailManager&);
};

// SlotItem definition must be after CDetailManager due to PSS dependency
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
