// DetailManager.h: interface for the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////

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

class CDetailManager
{
  public:
	struct SlotItem
	{
		float scale;
		float scale_calculated;
		Fmatrix mRotY;
		u32 vis_ID;
		float c_hemi;
		float c_sun;
	};
	DEFINE_VECTOR(SlotItem*, SlotItemVec, SlotItemVecIt);
	struct SlotPart
	{
		u32 id;
		SlotItemVec items;
		SlotItemVec r_items[3];
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
	};

	typedef xr_vector<xr_vector<SlotItemVec*>> vis_list;
	typedef svector<CDetail*, dm_max_objects> DetailVec;
	typedef DetailVec::iterator DetailIt;
	typedef poolSS<SlotItem, 4096> PSS;

	struct ObjectInstances : public xr_vector<InstanceData>
	{
		ObjectInstances()
		{
			reserve(1024);
		}
	};

  public:
	int dither[16][16];
	IReader* dtFS;
	DetailHeader dtH;
	DetailSlot* dtSlots;
	DetailSlot DS_empty;

	DetailVec objects;
	vis_list m_visibles[3];

#ifndef _EDITOR
	xrXRC xrc;
#endif
	CacheSlot1 cache_level1[dm_cache1_line][dm_cache1_line];
	Slot* cache[dm_cache_line][dm_cache_line];
	svector<Slot*, dm_cache_size> cache_task;
	Slot cache_pool[dm_cache_size];
	int cache_cx;
	int cache_cz;

	PSS poolSI;

	// Hardware processor (instanced)
	ref_geom hw_Geom;
	ref_geom hw_Geom_Instanced;
	u32 hw_BatchSize;
	IDirect3DVertexBuffer9* hw_VB;
	IDirect3DVertexBuffer9* hw_VB_Instances; // Instance data buffer
	IDirect3DIndexBuffer9* hw_IB;

	void UpdateVisibleM();

#ifdef _EDITOR
	virtual ObjectList* GetSnapList() = 0;
#endif

	IC bool UseVS()
	{
		return HW.Caps.geometry_major >= 1;
	}

	// Software processor
	ref_geom soft_Geom;
	void soft_Load();
	void soft_Unload();
	void soft_Render();

	// Hardware processor (instanced)
	void hw_Load();
	void hw_Unload();
	void hw_Render();
	void hw_Render_dump_instanced_combined();
	void hw_Render_dump_instanced();
	void hw_Render_object_instances(u32 object_index, CDetail& Object, u32 start_lod, u32 end_lod, int shader_id,
									u32 vOffset, u32 iOffset, Fmatrix& mWorld, Fmatrix& mView, Fmatrix& mProject,
									Fmatrix& mFullTransform);

	// get unpacked slot
	DetailSlot& QueryDB(int sx, int sz);

	void cache_Initialize();
	void cache_Update(int sx, int sz, Fvector& view, int limit);
	void cache_Task(int gx, int gz, Slot* D);
	Slot* cache_Query(int sx, int sz);
	void cache_Decompress(Slot* D);
	BOOL cache_Validate();
	// cache grid to world
	int cg2w_X(int x)
	{
		return cache_cx - dm_size + x;
	}
	int cg2w_Z(int z)
	{
		return cache_cz - dm_size + (dm_cache_line - 1 - z);
	}
	// world to cache grid
	int w2cg_X(int x)
	{
		return x - cache_cx + dm_size;
	}
	int w2cg_Z(int z)
	{
		return cache_cz - dm_size + (dm_cache_line - 1 - z);
	}

	void Load();
	void Unload();
	void Render();

	/// MT stuff
	xrCriticalSection MT;
	volatile u32 m_frame_calc;
	volatile u32 m_frame_rendered;

	void __stdcall MT_CALC();
	ICF void MT_SYNC()
	{
		OPTICK_EVENT("CDetailManager::MT_SYNC");

		if (m_frame_calc == Device.dwFrame)
			return;

		MT_CALC();
	}

	CDetailManager();
	virtual ~CDetailManager();
};

#endif // DetailManagerH
