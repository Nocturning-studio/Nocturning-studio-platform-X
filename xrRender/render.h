#pragma once

#include "r_dsgraph_structure.h"
#include "r_occlusion.h"

#include "PSLibrary.h"

#include "r_types.h"
#include "r_rendertarget.h"

#include "hom.h"
#include "detailmanager.h"
#include "modelpool.h"
#include "wallmarksengine.h"

#include "smap_allocator.h"
#include "light_db.h"
#include "light_render_direct.h"
#include "LightTrack.h"
#include "r_sun_cascades.h"

#include "../xrEngine\irenderable.h"
#include "../xrEngine\fmesh.h"
#include "xrRender_console.h"

// definition
class CRender : public R_dsgraph_structure
{
  public:
	enum
	{
		PHASE_NORMAL = 0,		// E[0]
		PHASE_SHADOW_DEPTH = 1, // E[1]
		PHASE_DEPTH_PREPASS = 2,
		PHASE_HUD = 3
	};

  public:
	struct _options
	{
		u32 smapsize : 16;
		u32 intz : 1;
		u32 nvstencil : 1;
		u32 nvdbt : 1;
		u32 distortion : 1;
		u32 forceskinw : 1;
		u32 noshadows : 1;

		float sun_depth_near_scale;
		float sun_depth_near_bias;
		float sun_depth_far_scale;
		float sun_depth_far_bias;

		bool use_ssao;
		bool use_atest_aa;
	} o;

	// string
	string32 c_build_id;
	string32 c_smapsize;
	string32 c_debugview;
	string32 c_vignette;
	string32 c_ao_quality;
	string32 c_aa_type;
	string32 c_fxaa_quality;
	string32 c_bloom_quality;
	string32 c_material_quality;
	string32 c_shadow_filter;
	string32 c_sun_shafts_quality;

	void CheckHWRenderSupporting();
	void update_options();

	struct _stats
	{
		u32 l_total, l_visible;
		u32 l_shadowed, l_unshadowed;
		s32 s_used, s_merged, s_finalclip;
		u32 o_queries, o_culled;
		u32 ic_total, ic_culled;
	} stats;

  public:
	// Sector detection and visibility
	CSector* pLastSector;
	Fvector vLastCameraPos;
	u32 uLastLTRACK;
	xr_vector<IRender_Portal*> Portals;
	xr_vector<IRender_Sector*> Sectors;
	xrXRC Sectors_xrc;
	CDB::MODEL* rmPortals;
	CHOM HOM;
	R_occlusion HWOCC;

	// Global vertex-buffer container
	xr_vector<FSlideWindowItem> SWIs;
	xr_vector<ref_shader> Shaders;
	typedef svector<D3DVERTEXELEMENT9, MAXD3DDECLLENGTH + 1> VertexDeclarator;
	xr_vector<VertexDeclarator> nDC, xDC;
	xr_vector<IDirect3DVertexBuffer9*> nVB, xVB;
	xr_vector<IDirect3DIndexBuffer9*> nIB, xIB;
	xr_vector<IRender_Visual*> Visuals;
	CPSLibrary PSLibrary;

	CDetailManager* Details;
	CModelPool* Models;
	CWallmarksEngine* Wallmarks;

	CRenderTarget* Target; // Render-target

	CLight_DB Lights;
	CLight_Compute_XFORM_and_VIS LR;
	xr_vector<light*> Lights_LastFrame;
	SMAP_Allocator LP_smap_pool;
	light_Package LP_normal;
	light_Package LP_pending;

	xr_vector<Fbox3, render_alloc<Fbox3>> main_coarse_structure;

	shared_str c_sbase;
	shared_str c_lmaterial;
	float o_hemi;
	float o_sun;
	IDirect3DQuery9* q_sync_point[2];
	u32 q_sync_count;

	bool m_bFirstFrameAfterReset; // Determines weather the frame is the first after resetting device.

	bool m_need_render_sun;
	xr_vector<sun::cascade> m_sun_cascades;

	//Motion blur
	Fmatrix m_saved_viewproj;

  private:
	// Loading / Unloading
	void LoadBuffers(CStreamReader* fs, BOOL _alternative);
	void LoadVisuals(IReader* fs);
	void LoadLights(IReader* fs);
	void LoadSectors(IReader* fs);
	void LoadSWIs(CStreamReader* fs);

	BOOL add_Dynamic(IRender_Visual* pVisual, u32 planes); // normal processing
	void add_Static(IRender_Visual* pVisual, u32 planes);
	void add_leafs_Dynamic(IRender_Visual* pVisual); // if detected node's full visibility
	void add_leafs_Static(IRender_Visual* pVisual);	 // if detected node's full visibility

  public:
	bool need_render_sun();
	void check_distort();

	void render_main(Fmatrix& mCombined, bool _fportals);
	void query_wait();
	void render_lights(light_Package& LP);
	void render_sun_cascade(u32 cascade_ind);
	void init_cacades();
	void render_sun_cascades();

	void render_stage_hom();
	void render_stage_ao();
	void render_stage_depth_prepass();
	void render_stage_gbuffer_main();
	void render_stage_gbuffer_secondary();
	void render_stage_occlusion_culling();
	void render_stage_forward();
	void update_shadow_map_visibility();
	void render_stage_sun();
	void render_stage_lights();
	void render_stage_postprocess();

	void RenderScene();
	void RenderMenu();

  public:
	ShaderElement* rimp_select_sh_static(IRender_Visual* pVisual, float cdist_sq);
	ShaderElement* rimp_select_sh_dynamic(IRender_Visual* pVisual, float cdist_sq);
	D3DVERTEXELEMENT9* getVB_Format(int id, BOOL _alt = FALSE);
	IDirect3DVertexBuffer9* getVB(int id, BOOL _alt = FALSE);
	IDirect3DIndexBuffer9* getIB(int id, BOOL _alt = FALSE);
	FSlideWindowItem* getSWI(int id);
	IRender_Portal* getPortal(int id);
	IRender_Sector* getSectorActive();
	IRender_Visual* model_CreatePE(LPCSTR name);
	IRender_Sector* detectSector(const Fvector& P, Fvector& D);
	int translateSector(IRender_Sector* pSector);

	// HW-occlusion culling
	IC u32 occq_begin(u32& ID)
	{
		return HWOCC.occq_begin(ID);
	}
	IC void occq_end(u32& ID)
	{
		HWOCC.occq_end(ID);
	}
	IC u32 occq_get(u32& ID)
	{
		return HWOCC.occq_get(ID);
	}

	ICF void apply_object(IRenderable* O)
	{
		if (0 == O)
			return;
		if (0 == O->renderable_ROS())
			return;
		CROS_impl& LT = *((CROS_impl*)O->renderable_ROS());
		LT.update_smooth(O);
		o_hemi = 0.75f * LT.get_hemi();
		o_sun = 0.75f * LT.get_sun();
	}
	IC void apply_lmaterial()
	{
		R_constant* C = &*RCache.get_c(c_sbase); // get sampler
		if (0 == C)
			return;
		VERIFY(RC_dest_sampler == C->destination);
		VERIFY(RC_sampler == C->type);
		CTexture* T = RCache.get_ActiveTexture(u32(C->samp.index));
		VERIFY(T);
		float mtl = T->m_material;
		RCache.set_c(c_lmaterial, o_hemi, o_sun, 0, (mtl + .5f) / 4.f);
	}

  public:
	// Loading / Unloading
	virtual void create();
	virtual void destroy();
	virtual void reset_begin();
	virtual void reset_end();

	virtual void level_Load(IReader*);
	virtual void level_Unload();

	virtual IDirect3DBaseTexture9* texture_load(LPCSTR fname, u32& msize);

	// Information
	virtual void Statistics(CGameFont* F);
	virtual LPCSTR getShaderPath()
	{
		return "";
	}
	virtual ref_shader getShader(int id);
	virtual IRender_Sector* getSector(int id);
	virtual IRender_Visual* getVisual(int id);
	virtual IRender_Sector* detectSector(const Fvector& P);
	virtual IRender_Target* getTarget();

	// Main
	virtual void flush();
	virtual void set_Object(IRenderable* O);
	virtual void add_Occluder(Fbox2& bb_screenspace); // mask screen region as oclluded
	virtual void add_Visual(IRender_Visual* V);		  // add visual leaf	(no culling performed at all)
	virtual void add_Geometry(IRender_Visual* V);	  // add visual(s)	(all culling performed)

	// wallmarks
	virtual void add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void clear_static_wallmarks();
	virtual void add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm);
	virtual void add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
									  const Fvector& dir, float size);

	//
	virtual IBlender* blender_create(CLASS_ID cls);
	virtual void blender_destroy(IBlender*&);

	//
	virtual IRender_ObjectSpecific* ros_create(IRenderable* parent);
	virtual void ros_destroy(IRender_ObjectSpecific*&);

	// Lighting
	virtual IRender_Light* light_create();
	virtual IRender_Glow* glow_create();

	// Models
	virtual IRender_Visual* model_CreateParticles(LPCSTR name);
	virtual IRender_DetailModel* model_CreateDM(IReader* F);
	virtual IRender_Visual* model_Create(LPCSTR name, IReader* data = 0);
	virtual IRender_Visual* model_CreateChild(LPCSTR name, IReader* data);
	virtual IRender_Visual* model_Duplicate(IRender_Visual* V);
	virtual void model_Delete(IRender_Visual*& V, BOOL bDiscard);
	virtual void model_Delete(IRender_DetailModel*& F);
	virtual void model_Logging(BOOL bEnable)
	{
		Models->Logging(bEnable);
	}
	virtual void models_Prefetch();
	virtual void models_Clear(BOOL b_complete);

	// Occlusion culling
	virtual BOOL occ_visible(vis_data& V);
	virtual BOOL occ_visible(Fbox& B);
	virtual BOOL occ_visible(sPoly& P);

	// Main
	virtual void Calculate();
	virtual void Render();
	virtual void Screenshot(ScreenshotMode mode = SM_NORMAL, LPCSTR name = 0);
	virtual void OnFrame();

	// Render mode
	virtual void rmNear();
	virtual void rmFar();
	virtual void rmNormal();

	// KD: need to know, what R2 phase is active now
	virtual u32 active_phase()
	{
		return render_phase;
	};

	virtual void set_active_phase(int active_phase)
	{
		render_phase = active_phase;
	};

	BOOL is_sun();

	// Constructor/destructor/loader
	CRender();
	virtual ~CRender();

	CShaderMacros FetchShaderMacros();

	HMODULE hCompiler;
  private:
	FS_FileSet m_file_set;
};

extern CRender RImplementation;
