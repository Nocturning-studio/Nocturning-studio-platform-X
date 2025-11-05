#pragma once

#include "render_dsgraph_structure.h"
#include "render_occlusion.h"

#include "PSLibrary.h"

#include "render_types.h"

#include "hom.h"
#include "detailmanager.h"
#include "modelpool.h"
#include "wallmarksengine.h"

#include "smap_allocator.h"
#include "light_db.h"
#include "light_render_direct.h"
#include "LightTrack.h"
#include "render_sun_cascades.h"

#include "../xrEngine/irenderable.h"
#include "../xrEngine/fmesh.h"

#include "render_taa.h"

#include "rendertarget.h"

class dxRender_Visual;

#pragma todo ("Implement static spot/point lights")

class cl_object_id : public R_constant_setup { virtual void setup(R_constant* C); };
class cl_planar_env			: public R_constant_setup { virtual void setup(R_constant* C); };
class cl_planar_amb			: public R_constant_setup { virtual void setup(R_constant* C); };
class cl_planar_bias		: public R_constant_setup { virtual void setup(R_constant* C); };
class cl_planar_shadow		: public R_constant_setup { virtual void setup(R_constant* C); };
class cl_planar_vp_camera	: public R_constant_setup { virtual void setup(R_constant* C); };

// definition
class CRender	:	public R_dsgraph_structure
{
public:
	enum PHASE
	{
		PHASE_NORMAL	= 0,	// E[0], E[1]	base
		PHASE_SMAP		= 1,	// E[2]			hw depth prepass
		PHASE_ZPREPASS	= 3,	// E[5]			sw depth prepass
		PHASE_RSMAP		= 4,	// E[6]	light-w-G-buffer & hw depth prepass
		PHASE_PLANAR	= 5,	// SE_NORMAL_LQ without bump
	};

	enum MMSM
	{
		MMSM_OFF = 0,
		MMSM_ON,
		MMSM_AUTO,
		MMSM_AUTODETECT
	};

public:
	struct _options
	{
		u32 nvstencil : 1;
		u32 nvdbt : 1;
		u32 distortion : 1;
		u32 tshadows : 1; // transluent shadows
		u32 volumetriclight : 1; // Volumetric lights
		u32 forceskinw : 1;
		u32 smapsize : 16;
		u32 smap_format : 32;
		u32 advanced_mode : 1; // SSDO, volumetric lights, sunshafts, etc...
		u32 aa_mode : 8;
		u32 volumetricfog : 1;
		u32 sm_minmax : 2;
		u32 sm_minmax_area_thold;
		u32 hud_shadows : 1;
		u32 cspecular : 1;
		u32 sunshafts : 1; // bool
		u32 ssao : 8; // u8
		u32 ssr : 1; // bool
		u32 ssr_replace : 1; // bool
		u32 sun_il : 1; // bool
		u32 spot_il : 1; // bool
		u32 planar : 1; // bool
		u32 gbd_opt : 1; // bool
		u32 pt_downsample : 1; // bool
		u32 gbd_save : 1; // bool
		u32 tessellation : 1;
		u32 msaa_samples : 16;
		u32 msaa_samples_reflections : 16;
		u32 txaa : 1; // bool
		u32 wet_surfaces : 1; // bool
		u32 ssaa : 32;
		u32 vsm : 1;
	} o;

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
	CSector*													pLastSector;
	Fvector														vLastCameraPos;
	u32															uLastLTRACK;
	xr_vector<IRender_Portal*>									Portals;
	xr_vector<IRender_Sector*>									Sectors;
	xrXRC														Sectors_xrc;
	CDB::MODEL*													rmPortals;
	CHOM														HOM;
	R_occlusion													HWOCC;

	// Global vertex-buffer container
	xr_vector<FSlideWindowItem>									SWIs;
	xr_vector<ref_shader>										Shaders;
	typedef svector<D3DVERTEXELEMENT9,MAXD3DDECLLENGTH+1>		VertexDeclarator;
	xr_vector<VertexDeclarator>									nDC,xDC;
	xr_vector<ID3DBuffer*>										nVB,xVB;
	xr_vector<ID3DBuffer*>										nIB,xIB;
	xr_vector<dxRender_Visual*>									Visuals;
	CPSLibrary													PSLibrary;

	CDetailManager*												Details;
	CModelPool*													Models;
	CWallmarksEngine*											Wallmarks;

	CRenderTarget*												Target;			// Render-target

	float														fWidth;
	float														fHeight;

	int															m_object_id;
	cl_object_id												m_C_object_id;
	
	// constant
	Fvector4													m_planar_env;
	cl_planar_env												m_C_planar_env;
	Fvector4													m_planar_amb;
	cl_planar_amb												m_C_planar_amb;
	Fvector4													m_planar_bias;
	cl_planar_bias												m_C_planar_bias;

	// matrices
	Fmatrix														m_planar_shadow;
	cl_planar_shadow											m_C_planar_shadow;
	Fmatrix														m_planar_vp_camera;
	cl_planar_vp_camera											m_C_planar_vp_camera;

	// for smap
	Fmatrix														m_planar_shadow_project; 
	Fmatrix														m_planar_shadow_bias_t;

	CLight_DB													Lights;
	CLight_Compute_XFORM_and_VIS								LR;
	xr_vector<light*>											Lights_LastFrame;
	SMAP_Allocator												LP_smap_pool;
	light_Package												LP_normal;
	light_Package												LP_pending;

	xr_vector<Fbox3,render_alloc<Fbox3> >						main_coarse_structure;

	shared_str													c_sbase			;
	shared_str													c_lmaterial		;
	float														o_hemi			;
	float														o_hemi_cube[CROS_impl::NUM_FACES]	;
	float														o_sun			;
	ID3DQuery*													q_sync_point[CHWCaps::MAX_GPUS];
	u32															q_sync_count	;

	BOOL														m_torch_enabled;

	bool														m_bMakeAsyncSS;
	bool														m_bFirstFrameAfterReset;	// Determines weather the frame is the first after resetting device.
	xr_vector<sun::cascade>										m_sun_cascades;

private:
	// Loading / Unloading
	void							LoadBuffers					(CStreamReader	*fs,	BOOL	_alternative);
	void							LoadVisuals					(IReader	*fs);
	void							LoadLights					(IReader	*fs);
	void							LoadPortals					(IReader	*fs);
	void							LoadSectors					(IReader	*fs);
	void							LoadSWIs					(CStreamReader	*fs);
	void							Load3DFluid					();

	BOOL							add_Dynamic					(dxRender_Visual*pVisual, u32 planes);		// normal processing
	void							add_Static					(dxRender_Visual*pVisual, u32 planes);
	void							add_leafs_Dynamic			(dxRender_Visual*pVisual);					// if detected node's full visibility
	void							add_leafs_Static			(dxRender_Visual*pVisual);					// if detected node's full visibility

	void							update_options			();

	LPCSTRVec						m_vecShaderErrorBuf;			// for shader errors, slow...
	
	BOOL							m_need_adv_cache;
	BOOL							m_need_warnings;
	BOOL							m_need_recomp;
	BOOL							m_need_disasm;
public:
	IRender_Sector*					rimp_detectSector			(Fvector& P, Fvector& D);
	void							render_main					(Fmatrix& mCombined, bool _fportals);
	void							render_forward				();
	void							render_lights				(light_Package& LP	);
	void							render_menu					();
	void							render_rain					();
	
	void							planar_render				(ref_texture t_env_0, ref_texture t_env_1, Fvector4 env, Fvector4 amb);
	void							planar_save_shadow			(Fmatrix xf_project, Fmatrix bias_t);

	void							render_sun_cascade			(u32 cascade_ind);
	void							init_cacades				();
	void							render_sun_cascades			();

	void							render_sun_vsm				();
public:
	ShaderElement*					rimp_select_sh				(dxRender_Visual	*pVisual, float cdist_sq);
	ShaderElement*					rimp_select_sh_static		(dxRender_Visual	*pVisual, float cdist_sq);
	ShaderElement*					rimp_select_sh_dynamic		(dxRender_Visual	*pVisual, float cdist_sq);
	D3DVERTEXELEMENT9*				getVB_Format				(int id, BOOL	_alt=FALSE);
	ID3DBuffer*						getVB						(int id, BOOL	_alt=FALSE);
	ID3DBuffer*						getIB						(int id, BOOL	_alt=FALSE);
	FSlideWindowItem*				getSWI						(int id);
	IRender_Portal*					getPortal					(int id);
	IRender_Sector*					getSectorActive				();
	IRenderVisual*					model_CreatePE				(LPCSTR name);
	IRender_Sector*					detectSector				(const Fvector& P, Fvector& D);
	int								translateSector				(IRender_Sector* pSector);

	// HW-occlusion culling
	IC u32							occq_begin					(u32&	ID		)	{ return HWOCC.occq_begin	(ID);	}
	IC void							occq_end					(u32&	ID		)	{ HWOCC.occq_end	(ID);			}
	IC R_occlusion::occq_result		occq_get					(u32&	ID		)	{ return HWOCC.occq_get		(ID);	}

	ICF void						apply_object				(IRenderable*	O)
	{
		if (0==O)					return;
		if (0==O->renderable_ROS())	return;
		CROS_impl& LT				= *((CROS_impl*)O->renderable_ROS());
		LT.update_smooth			(O)								;
		o_hemi						= 0.75f*LT.get_hemi			()	;
		//o_hemi						= 0.5f*LT.get_hemi			()	;
		o_sun						= 0.75f*LT.get_sun			()	;
		CopyMemory(o_hemi_cube, LT.get_hemi_cube(), CROS_impl::NUM_FACES*sizeof(float));
	}
	IC void							apply_lmaterial				()
	{
		R_constant*		C	= &*RCache.get_c	(c_sbase);		// get sampler
		if (0==C)			return;
		VERIFY				(RC_dest_sampler	== C->destination);
		VERIFY				(RC_dx10texture		== C->type);
		CTexture*		T	= RCache.get_ActiveTexture	(u32(C->samp.index));
		VERIFY				(T);
		float	mtl			= 2.2;
		if (T != NULL)
			mtl = T->m_material;
		else
			Log("! warning: can't read material from texture, set default");
		if (!mtl) mtl = 2.2;
		RCache.hemi.set_material (o_hemi,o_sun,0,(mtl+.5f)/4.f);
		RCache.hemi.set_pos_faces(o_hemi_cube[CROS_impl::CUBE_FACE_POS_X],
			o_hemi_cube[CROS_impl::CUBE_FACE_POS_Y],
			o_hemi_cube[CROS_impl::CUBE_FACE_POS_Z]);
		RCache.hemi.set_neg_faces	(o_hemi_cube[CROS_impl::CUBE_FACE_NEG_X],
			o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Y],
			o_hemi_cube[CROS_impl::CUBE_FACE_NEG_Z]);
	}

public:
	// feature level
	virtual	GenerationLevel			get_generation			()	{ return IRender_interface::GENERATION_R2; }

	virtual DWORD					get_dx_level			()	{ return HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1 ? 0x000A0001 : 0x000A0000; }

	virtual bool					is_sun_static				();

	// Loading / Unloading
	virtual void					create						();
	virtual void					destroy						();
	virtual	void					reset_begin					();
	virtual	void					reset_end					();

	virtual	void					level_Load					(IReader*);
	virtual void					level_Unload				();

	ID3DBaseTexture*				texture_load				(LPCSTR	fname, u32& msize, bool bStaging = false);

	virtual HRESULT					shader_compile(
		LPCSTR							name,
		DWORD const*					pSrcData,
		UINT                            SrcDataLen,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		void*& result);
	
	// Information
	virtual void					Statistics					(CGameFont* F);
	virtual LPCSTR					getShaderPath				()									{ return "r3\\";	}
	virtual ref_shader				getShader					(int id);
	virtual IRender_Sector*			getSector					(int id);
	virtual IRenderVisual*			getVisual					(int id);
	virtual IRender_Sector*			detectSector				(const Fvector& P);
	virtual IRender_Target*			getTarget					();

	// Torch
	virtual void					set_TorchEnabled			(bool state);

	// Main 
	virtual void					flush						();
	virtual void					set_Object					(IRenderable*		O	);
	virtual	void					add_Occluder				(Fbox2&	bb_screenspace	);			// mask screen region as oclluded
	virtual void					add_Visual					(IRenderVisual*	V	);			// add visual leaf	(no culling performed at all)
	virtual void					add_Geometry				(IRenderVisual*	V	);			// add visual(s)	(all culling performed)

	// wallmarks
	virtual void					add_StaticWallmark			(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					add_StaticWallmark			(IWallMarkArray *pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					add_StaticWallmark			(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V);
	virtual void					clear_static_wallmarks		();
	virtual void					add_SkeletonWallmark		(intrusive_ptr<CSkeletonWallmark> wm);
	virtual void					add_SkeletonWallmark		(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size);
	virtual void					add_SkeletonWallmark		(const Fmatrix* xf, IKinematics* obj, IWallMarkArray *pArray, const Fvector& start, const Fvector& dir, float size);

	//
	virtual IBlender*				blender_create				(CLASS_ID cls);
	virtual void					blender_destroy				(IBlender* &);

	//
	virtual IRender_ObjectSpecific*	ros_create					(IRenderable*		parent);
	virtual void					ros_destroy					(IRender_ObjectSpecific* &);

	// Lighting
	virtual IRender_Light*			light_create				();
	virtual IRender_Glow*			glow_create					();

	// Models
	virtual IRenderVisual*			model_CreateParticles		(LPCSTR name);
	virtual IRender_DetailModel*	model_CreateDM				(IReader* F);
	virtual IRenderVisual*			model_Create				(LPCSTR name, IReader* data=0);
	virtual IRenderVisual*			model_CreateChild			(LPCSTR name, IReader* data);
	virtual IRenderVisual*			model_Duplicate				(IRenderVisual*	V);
	virtual void					model_Delete				(IRenderVisual* &	V, BOOL bDiscard);
	virtual void 					model_Delete				(IRender_DetailModel* & F);
	virtual void					model_Logging				(BOOL bEnable)				{ Models->Logging(bEnable);	}
	virtual void					models_Prefetch				();
	virtual void					models_Clear				(BOOL b_complete);

	// Occlusion culling
	virtual BOOL					occ_visible					(vis_data&	V);
	virtual BOOL					occ_visible					(Fbox&		B);
	virtual BOOL					occ_visible					(sPoly&		P);

	// Main
	virtual void					Calculate					();
	virtual void					Render						();
	virtual void					RenderDeffered				();
	virtual void					Screenshot					(ScreenshotMode mode=SM_NORMAL, LPCSTR name = 0);
	virtual void					Screenshot					(ScreenshotMode mode, CMemoryWriter& memory_writer);
	virtual void					ScreenshotAsyncBegin		();
	virtual void					ScreenshotAsyncEnd			(CMemoryWriter& memory_writer);
	virtual void		_BCL		OnFrame						();

	// Render mode
	virtual void					rmNear						();
	virtual void					rmFar						();
	virtual void					rmNormal					();

	// Constructor/destructor/loader
	CRender							();
	virtual ~CRender				();

	void addShaderOption(const char* name, const char* value);
	void clearAllShaderOptions() {m_ShaderOptions.clear();}

private:
	xr_vector<D3D_SHADER_MACRO>									m_ShaderOptions;

private:
	template<typename T>
	HRESULT							shader_compile_help(
		LPCSTR							name,
		DWORD const*					pSrcData,
		UINT                            SrcDataLen,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		T*&								result);
		
protected:
	virtual	void					ScreenshotImpl				(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer);

private:
	FS_FileSet						m_file_set;
};

extern CRender						RImplementation;
