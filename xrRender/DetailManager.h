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

const int dm_size = 36;
const int dm_max_decompress = 16;
const int dm_cache1_count = 4;
const int dm_cache1_line = dm_size * 2 / dm_cache1_count;
const int dm_max_objects = 64;
const int dm_obj_in_slot = 4;
const int dm_cache_line = dm_size + 1 + dm_size;
const int dm_cache_size = dm_cache_line * dm_cache_line;
const float dm_fade = float(2 * dm_size) - .5f;
const float dm_slot_size = DETAIL_SLOT_SIZE;

// Forward declarations
struct SlotItem;

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

		Slot()
		{
			frame = 0;
			empty = 1;
			type = stReady;
			sx = sz = 0;
			vis.clear();
		}
	};

	struct CacheSlot1
	{
		u32 empty;
		vis_data vis;
		Slot** slots[dm_cache1_count * dm_cache1_count];

		CacheSlot1()
		{
			empty = 1;
			vis.clear();
			ZeroMemory(slots, sizeof(slots));
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

	// Memory pool
	PSS m_poolSI;

	// Hardware resources
	ref_geom m_hw_Geom;
	ref_geom m_hw_Geom_Instanced;
	u32 m_hw_BatchSize;
	IDirect3DVertexBuffer9* m_hw_VB;
	IDirect3DVertexBuffer9* m_hw_VB_Instances;
	IDirect3DIndexBuffer9* m_hw_IB;

	// Threading and state
	xrCriticalSection m_MT;
	volatile u32 m_frame_calc;
	volatile u32 m_frame_rendered;
	bool m_initialized;
	bool m_loaded;
	volatile BOOL m_bInLoad;
	volatile BOOL m_bShuttingDown;
	bool m_bRegisteredInParallel;

  public:
	// Static configuration
	static BOOL s_disable_hom;
	static BOOL s_use_hom_occlusion;
	static float s_occlusion_padding_scale;
	static float s_max_occlusion_distance;

  private:
	// Метод для удаления из параллельных задач
	void UnregisterFromParallel();

	// Internal methods
	void Internal_Load();
	void Internal_Unload();
	void Internal_Render();

	// Visibility and cache management
	void UpdateVisibleM();
	void ClearVisibleLists();
	void CopyVisibleListsForRender();

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
