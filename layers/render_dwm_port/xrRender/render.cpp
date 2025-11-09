#include "stdafx.h"
#include "render.h"
#include "fbasicvisual.h"
#include "../xrEngine/xr_object.h"
#include "../xrEngine/CustomHUD.h"
#include "../xrEngine/igame_persistent.h"
#include "../xrEngine/environment.h"
#include "SkeletonCustom.h"
#include "LightTrack.h"
//#include "dxRenderDeviceRender.h"
//#include "dxWallMarkArray.h"
//#include "dxUIShader.h"
#include "ShaderResourceTraits.h"

#include "3DFluidManager.h"
#include "D3DX11Core.h"

CRender RImplementation;

void cl_object_id::setup(R_constant* C) { RCache.set_c(C, RImplementation.m_object_id); }
void cl_planar_env			::setup(R_constant* C) { RCache.set_c(C, RImplementation.m_planar_env					); }
void cl_planar_amb			::setup(R_constant* C) { RCache.set_c(C, RImplementation.m_planar_amb					); }
void cl_planar_bias			::setup(R_constant* C) { RCache.set_c(C, RImplementation.m_planar_bias					); }
void cl_planar_shadow		::setup(R_constant* C) { RCache.set_c(C, RImplementation.m_planar_shadow				); }
void cl_planar_vp_camera	::setup(R_constant* C) { RCache.set_c(C, RImplementation.m_planar_vp_camera				); }

//////////////////////////////////////////////////////////////////////////
class CGlow				: public IRender_Glow
{
public:
	bool				bActive;
public:
	CGlow() : bActive(false)		{ }
	virtual void					set_active			(bool b)					{ bActive=b;		}
	virtual bool					get_active			()							{ return bActive;	}
	virtual void					set_position		(const Fvector& P)			{ }
	virtual void					set_direction		(const Fvector& D)			{ }
	virtual void					set_radius			(float R)					{ }
	virtual void					set_texture			(LPCSTR name)				{ }
	virtual void					set_color			(const Fcolor& C)			{ }
	virtual void					set_color			(float r, float g, float b)	{ }
};

float		r_dtex_range		= 50.f;

//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(IRender_Visual* pVisual, float cdist_sq)
{
	return rimp_select_sh(pVisual, cdist_sq);
	/*int		id = SE_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_NORMAL_HQ : SE_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();*/
}
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(IRender_Visual* pVisual, float cdist_sq)
{
	return rimp_select_sh(pVisual, cdist_sq);
	/*int		id = SE_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_NORMAL_HQ : SE_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();*/
}
ShaderElement* CRender::rimp_select_sh(IRender_Visual* pVisual, float cdist_sq)
{
	u32 id;

	bool hq = _sqrt(cdist_sq) - pVisual->vis.sphere.R < r_dtex_range;

	switch (RImplementation.phase)
	{
	case PHASE_NORMAL:		// deffer
		id = hq ? SE_NORMAL_HQ : SE_NORMAL_LQ;
		break;
	case PHASE_SMAP:		// deffer
		id = SE_SHADOW;
		break;
	case PHASE_ZPREPASS:	// deffer
		id = SE_ZPREPASS;
		break;
	case PHASE_RSMAP:		// deffer
		id = SE_RSM_FILL_RTS;
		break;
	case PHASE_PLANAR:		// forward
		id = SE_PLANAR;
		break;
	default:
		id = SE_UNKNOWN;
		Log("! invalid render phase, set SE_UNKNOWN by default");
	}

	return pVisual->shader->E[id]._get();
}

static class cl_LOD		: public R_constant_setup
{
	virtual void setup	(R_constant* C)
	{
		RCache.LOD.set_LOD(C);
	}
} binder_LOD;

static class cl_pos_decompress_params		: public R_constant_setup		{	virtual void setup	(R_constant* C)
{
	float VertTan =  -1.0f * tanf( deg2rad(float(Device.fFOV) /2.0f ) );
	float HorzTan =  - VertTan / Device.fASPECT;

	RCache.set_c(C, HorzTan, VertTan, (2.0f * HorzTan) / RImplementation.fWidth, (2.0f * VertTan) / RImplementation.fHeight);

}}	binder_pos_decompress_params;

static class cl_screen_res : public R_constant_setup		{	virtual void setup	(R_constant* C)
{
	RCache.set_c(C, RImplementation.fWidth, RImplementation.fHeight, 1.0f / RImplementation.fWidth, 1.0f / RImplementation.fHeight);

}}	binder_screen_res;

static class cl_sun_shafts_intensity : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E.m_fSunShaftsIntensity;
		RCache.set_c	(C, fValue, fValue, fValue, 0);
	}
}	binder_sun_shafts_intensity;

//////////////////////////////////////////////////////////////////////////
// Just two static storage
void CRender::update_options()
{
	fWidth = (float)Device.dwWidth;
	fHeight = (float)Device.dwHeight;

	m_need_adv_cache = TRUE;
	m_need_warnings = FALSE;
	m_need_recomp = FALSE;

	m_need_disasm = !!strstr(Core.Params, "-disasm");

	// smap
	u32 smap_size_d = 512;
	o.smapsize = is_sun_static() ? smap_size_d : r__smap_size;

	//	For ATI it's much faster on DX11 to use D32F format
	if (HW.Caps.id_vendor == 0x1002)
	{
		o.smap_format = DXGI_FORMAT_D32_FLOAT;
		Log("* using d32 smap depth format");
	}
	else
	{
		o.smap_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		Log("* using d24 smap depth format");
	}

	o.nvstencil = opt(R__NEED_NVDSTENCIL) && HW.Caps.id_vendor == 0x10DE && HW.Caps.id_device >= 0x40;
	o.nvdbt = opt(R__USE_NVDBT);
	o.tshadows = opt(R__USE_TRANS_SHADOWS);
	o.forceskinw = !!strstr(Core.Params, "-skinw");

	o.tessellation = opt(R__NEED_TESSELATION) && HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0;

	o.wet_surfaces = opt(R__USE_WET_SURFACES);

	o.ssaa = r__ssaa;

	o.advanced_mode = opt(R__USE_ADVANCED_MODE);

	if (is_sun_static())
		o.advanced_mode = FALSE;

	if (o.advanced_mode)
	{
		switch (r__aa)
		{
		case FALSE:			o.aa_mode = FALSE;		o.msaa_samples = 1; o.txaa = FALSE;	Msg("* Antialiasing type: disabled");		break;
		case AA_MLAA:		o.aa_mode = AA_MLAA;	o.msaa_samples = 1; o.txaa = FALSE;	Msg("* Antialiasing type: MLAA");			break;
		case AA_FXAA:		o.aa_mode = AA_FXAA;	o.msaa_samples = 1; o.txaa = FALSE;	Msg("* Antialiasing type: FXAA");			break;
		case AA_MSAA_FXAA:	o.aa_mode = AA_MSAA;	o.msaa_samples = 2; o.txaa = FALSE;	Msg("* Antialiasing type: MSAA + FXAA");	break;
		case AA_MSAA2S:		o.aa_mode = AA_MSAA;	o.msaa_samples = 2; o.txaa = FALSE;	Msg("* Antialiasing type: MSAA 2X");		break;
		case AA_MSAA4S:		o.aa_mode = AA_MSAA;	o.msaa_samples = 4; o.txaa = FALSE;	Msg("* Antialiasing type: MSAA 4X");		break;
		case AA_MSAA8S:		o.aa_mode = AA_MSAA;	o.msaa_samples = 8; o.txaa = FALSE;	Msg("* Antialiasing type: MSAA 8X");		break;
		case AA_TAA:		o.aa_mode = AA_TAA;		o.msaa_samples = 1; o.txaa = FALSE;	Msg("* Antialiasing type: TAA");			break;
		case AA_TAA_V2:		o.aa_mode = AA_TAA_V2;	o.msaa_samples = 1; o.txaa = FALSE;	Msg("* Antialiasing type: TAA V2");			break;
		case AA_TXAA:		o.aa_mode = AA_TXAA;	o.msaa_samples = 1; o.txaa = TRUE;	Msg("* Antialiasing type: TXAA");			break;
		case AA_TXAA2S:		o.aa_mode = AA_MSAA;	o.msaa_samples = 2; o.txaa = TRUE;	Msg("* Antialiasing type: TXAA 2X");		break;
		case AA_TXAA4S:		o.aa_mode = AA_MSAA;	o.msaa_samples = 4; o.txaa = TRUE;	Msg("* Antialiasing type: TXAA 4X");		break;
		}

		if (o.aa_mode == AA_MSAA)
			o.ssaa = FALSE;

		if (o.msaa_samples > 1)
		{
			switch (HW.FeatureLevel)
			{
			case D3D_FEATURE_LEVEL_10_0: Msg("MSAA Type: 10.0"); break;
			default: Msg("MSAA Type: 11.0");
			}
		}

		o.sm_minmax = opt(R__NEED_MINMAX_SM);
		if (o.sm_minmax)
		{
			if (HW.Caps.id_vendor == 0x1002)
				o.sm_minmax = MMSM_AUTO;
			else
				o.sm_minmax = MMSM_OFF;
		}

		o.msaa_samples_reflections = r__msaa_reflections;
		o.volumetricfog = opt(R__USE_VSMOKE);
		o.volumetriclight = opt(R__USE_VLIGHT);
		o.hud_shadows = opt(R__USE_HUD_SHADOWS);
		o.cspecular = opt(R__USE_CSPECULAR);
		o.sunshafts = r__sunshafts_mode > 0;
		o.ssao = r__ssao_mode;
		o.ssr = opt(R__USE_SSR);
		o.ssr_replace = opt(R__REP_SSR_TO_PLANAR);
		o.sun_il = opt(R__USE_SUN_IL);
		o.spot_il = opt(R__USE_SPOT_IL);
		o.planar = opt(R__USE_PLANAR);
		o.gbd_opt = opt(R__USE_GBD_OPT);
		o.pt_downsample = opt(R__PT_DOWNSAMPLE);
	}
	else
	{
		o.aa_mode = FALSE;
		o.sm_minmax = FALSE;
		o.volumetricfog = FALSE;
		o.volumetriclight = FALSE;
		o.hud_shadows = FALSE;
		o.cspecular = FALSE;
		o.sunshafts = FALSE;
		o.ssao = FALSE;
		o.ssr = FALSE;
		o.ssr_replace = FALSE;
		o.sun_il = FALSE;
		o.spot_il = FALSE;
		o.planar = FALSE;
		o.gbd_opt = FALSE;
		o.msaa_samples = 1;
		o.msaa_samples_reflections = 1;
		o.txaa = FALSE;
		o.pt_downsample = FALSE;
	}

	if (o.planar)
	{
		o.ssr = FALSE;
		o.ssr_replace = FALSE;
	}

	if (
		o.sun_il ||
		o.spot_il ||
		o.ssao == SSAO_PATH_TRACING
		)
	{
		Log("* Keep G_Buffer of the previous frame: enabled");
		o.gbd_save = TRUE;
	}
	else
	{
		Log("* Keep G_Buffer of the previous frame: disabled");
		o.gbd_save = FALSE;
	}

	o.sm_minmax_area_thold = 1600 * 1200;

	// recompile all shaders
	if (strstr(Core.Params, "-noscache"))
	{
		m_need_adv_cache = FALSE;
		m_need_warnings = FALSE;
		m_need_recomp = TRUE;
	}

	// if "-warnings" is on "-noscache" must be enabled
	if (strstr(Core.Params, "-warnings"))
	{
		m_need_adv_cache = FALSE;
		m_need_warnings = TRUE;
		m_need_recomp = TRUE;
	}

	// distortion
	{
		o.distortion = TRUE;
	}

#ifdef __GFSDK_DX11__
	if (o.txaa == TRUE)
	{
		if (HW.m_TXAA_initialized)
		{
			//Log("* TXAA supported and used");
		}
		else
		{
			o.txaa = FALSE;

			// TXAA 2X, 4X, replace with MSAA
			//if (o.aa_mode == AA_MSAA)
			//{
				//Log("! TXAA doesn't supported ... Enabled MSAA 2X/4X");
				//o.aa_mode = AA_MSAA;
				//o.msaa_samples = o.msaa_samples;
			//}

			// TXAA 1X, replace with TAA
			//if (o.aa_mode == AA_TXAA)
			{
				Log("! TXAA doesn't supported ... Enabled TAA");
				o.aa_mode = AA_TAA_V2;
				o.msaa_samples = 1;
			}
		}
	}

	if (o.ssao == SSAO_HBAO_PLUS)
	{
		if (HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0)
		{
			//Log("* HBAO+ supported and used");
		}
		else
		{
			Log("! HBAO+ doesn't supported ... Enabled simple SSAO");
			o.ssao = SSAO_SSAO;
		}
	}
#endif

	// Turn off SSPR if CS isn't support
	if (!HW.m_cs_support || HW.FeatureLevel <= D3D_FEATURE_LEVEL_10_1)
	{
		o.ssr_replace = FALSE;
	}

	// Variance shadow mapping
	o.vsm = opt(R__USE_VSM);
	//o.vsm = r__smap_filter == 4;

	if (o.vsm)
	{
		Msg("! GI isn't supported with Variance Shadow Mapping");
		o.sun_il = FALSE;
		o.spot_il = FALSE;
	}
}

bool					CRender::is_sun_static()
{
	extern	ENGINE_API	u32		renderer_value;
	//return renderer_value == RenderCreationParams::R_R4A || renderer_value == RenderCreationParams::R_R1;
	return false;
}

void					CRender::create					()
{
	Device.seqFrame.Add	(this,REG_PRIORITY_HIGH+0x12345678);

	// init verify
	{
		BOOL mrt = (HW.Caps.raster.dwMRT_count >= 3);
		VERIFY2(mrt && (HW.Caps.raster.dwInstructions >= 256), "Hardware doesn't meet minimum feature-level");
	}

	m_skinning = -1;
	m_MSAASample = -1;

	update_options();

	// constants
	Device.Resources->RegisterConstantSetup("sun_shafts_intensity",		&binder_sun_shafts_intensity);
	Device.Resources->RegisterConstantSetup("pos_decompression_params",	&binder_pos_decompress_params);
	Device.Resources->RegisterConstantSetup("screen_res",					&binder_screen_res);
	Device.Resources->RegisterConstantSetup("triLOD",						&binder_LOD);

	c_lmaterial					= "L_material";
	c_sbase						= "s_base";

	m_bMakeAsyncSS				= false;

	Target						= xr_new<CRenderTarget>		();	// Main target

	Models						= xr_new<CModelPool>		();
	PSLibrary.OnCreate			();
	HWOCC.occq_create			(occq_size);

	Target->enable_SSAA();

	rmNormal					();
	marker						= 0;

	D3D_QUERY_DESC			qdesc;
	qdesc.MiscFlags				= 0;
	qdesc.Query					= D3D_QUERY_EVENT;

	ZeroMemory(q_sync_point, sizeof(q_sync_point));

	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		R_CHK(HW.pDevice11->CreateQuery(&qdesc, &q_sync_point[i]));

	HW.pContext11->End(q_sync_point[0]);

	xrRender_apply_tf			();
	::PortalTraverser.initialize();

	FluidManager.Initialize( 70, 70, 70 );
	FluidManager.SetScreenSize((int)fWidth, (int)fHeight);

	//Msg("* Scene render base resolution: %ux%u", (u32)fWidth, (u32)fHeight);
	//Msg("* HUD render base resolution: %ux%u", Device.dwWidth, Device.dwHeight);
}

void					CRender::destroy				()
{
	m_bMakeAsyncSS				= false;

	FluidManager.Destroy();

	::PortalTraverser.destroy	();

	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		_RELEASE				(q_sync_point[i]);
	
	HWOCC.occq_destroy			();
	xr_delete					(Models);
	xr_delete					(Target);
	PSLibrary.OnDestroy			();
	Device.seqFrame.Remove		(this);
	r_dsgraph_destroy			();
}

void CRender::reset_begin()
{
	// Update incremental shadowmap-visibility solver
	// BUG-ID: 10646
	{
		u32 it=0;
		for (it=0; it<Lights_LastFrame.size(); it++)	{
			if (0==Lights_LastFrame[it])	continue	;
			try {
				Lights_LastFrame[it]->svis.resetoccq ()	;
			} catch (...)
			{
				Msg	("! Failed to flush-OCCq on light [%d] %X",it,*(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear	();
	}

	/*if (Details)
	{
		Details->Unload();
		xr_delete(Details);
	}*/

	xr_delete					(Target);
	HWOCC.occq_destroy			();
	//_RELEASE					(q_sync_point[1]);
	//_RELEASE					(q_sync_point[0]);
	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		_RELEASE				(q_sync_point[i]);
}

void CRender::reset_end()
{
	D3D_QUERY_DESC			qdesc;
	qdesc.MiscFlags				= 0;
	qdesc.Query					= D3D_QUERY_EVENT;

	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		R_CHK(HW.pDevice11->CreateQuery(&qdesc,&q_sync_point[i]));

	//	Prevent error on first get data
	HW.pContext11->End(q_sync_point[0]);

	HWOCC.occq_create			(occq_size);

	if (m_need_recomp)
		update_options();

	Target						=	xr_new<CRenderTarget>	();

	/*if (Details == NULL)
	{
		Details = xr_new<CDetailManager>();
		Details->Load();
	}*/

	xrRender_apply_tf			();

	FluidManager.SetScreenSize(Device.dwWidth, Device.dwHeight);

	// clear shader errors vector
	if (m_need_warnings)
		for(u32 i = 0; i < m_vecShaderErrorBuf.size(); i++)
			m_vecShaderErrorBuf.pop_back();

	// Set this flag true to skip the first render frame,
	// that some data is not ready in the first frame (for example device camera position)
	m_bFirstFrameAfterReset		= true;

	//Msg("* Scene render base resolution: %ux%u", (u32)fWidth, (u32)fHeight);
	//Msg("* HUD render base resolution: %ux%u", Device.dwWidth, Device.dwHeight);
}

void CRender::OnFrame()
{
	Models->DeleteQueue			();

	// MT-details (@front)
	Device.seqParallel.insert	(Device.seqParallel.begin(),
		fastdelegate::FastDelegate0<>(Details,&CDetailManager::MT_CALC));

	// MT-HOM (@front)
	Device.seqParallel.insert	(Device.seqParallel.begin(),
		fastdelegate::FastDelegate0<>(&HOM,&CHOM::MT_RENDER));
}


// Implementation
IRender_ObjectSpecific*	CRender::ros_create				(IRenderable* parent)				{ return xr_new<CROS_impl>();			}
void					CRender::ros_destroy			(IRender_ObjectSpecific* &p)		{ xr_delete(p);							}
IRender_Visual*			CRender::model_Create			(LPCSTR name, IReader* data)		{ return Models->Create(name,data);		}
IRender_Visual*			CRender::model_CreateChild		(LPCSTR name, IReader* data)		{ return Models->CreateChild(name,data);}
IRender_Visual*			CRender::model_Duplicate		(IRender_Visual* V)					{ return Models->Instance_Duplicate((IRender_Visual*)V);	}
void					CRender::model_Delete			(IRender_Visual* &V, BOOL bDiscard)	
{ 
	IRender_Visual* pVisual = (IRender_Visual*)V;
	Models->Delete(pVisual, bDiscard);
	V = 0;
}
IRender_DetailModel*	CRender::model_CreateDM			(IReader*	F)
{
	CDetail*	D		= xr_new<CDetail> ();
	D->Load				(F);
	return D;
}
void					CRender::model_Delete			(IRender_DetailModel* & F)
{
	if (F)
	{
		CDetail*	D	= (CDetail*)F;
		D->Unload		();
		xr_delete		(D);
		F				= NULL;
	}
}
IRender_Visual*			CRender::model_CreatePE			(LPCSTR name)	
{ 
	PS::CPEDef*	SE			= PSLibrary.FindPED	(name);		R_ASSERT3(SE,"Particle effect doesn't exist",name);
	return					Models->CreatePE	(SE);
}
IRender_Visual*			CRender::model_CreateParticles	(LPCSTR name)	
{ 
	PS::CPEDef*	SE			= PSLibrary.FindPED	(name);
	if (SE) return			Models->CreatePE	(SE);
	else{
		PS::CPGDef*	SG		= PSLibrary.FindPGD	(name);		R_ASSERT3(SG,"Particle effect or group doesn't exist",name);
		return				Models->CreatePG	(SG);
	}
}
void					CRender::models_Prefetch		()					{ Models->Prefetch	();}
void					CRender::models_Clear			(BOOL b_complete)	{ Models->ClearPool	(b_complete);}

ref_shader				CRender::getShader				(int id)			{ VERIFY(id<int(Shaders.size()));	return Shaders[id];	}
IRender_Portal*			CRender::getPortal				(int id)			{ VERIFY(id<int(Portals.size()));	return Portals[id];	}
IRender_Sector*			CRender::getSector				(int id)			{ VERIFY(id<int(Sectors.size()));	return Sectors[id];	}
IRender_Sector*			CRender::getSectorActive		()					{ return pLastSector;									}
IRender_Visual*			CRender::getVisual				(int id)			{ VERIFY(id<int(Visuals.size()));	return Visuals[id];	}
D3DVERTEXELEMENT9*		CRender::getVB_Format			(int id, BOOL	_alt)	{ 
	if (_alt)	{ VERIFY(id<int(xDC.size()));	return xDC[id].begin();	}
	else		{ VERIFY(id<int(nDC.size()));	return nDC[id].begin(); }
}
ID3DBuffer*	CRender::getVB					(int id, BOOL	_alt)	{
	if (_alt)	{ VERIFY(id<int(xVB.size()));	return xVB[id];		}
	else		{ VERIFY(id<int(nVB.size()));	return nVB[id];		}
}
ID3DBuffer*	CRender::getIB					(int id, BOOL	_alt)	{
	if (_alt)	{ VERIFY(id<int(xIB.size()));	return xIB[id];		}
	else		{ VERIFY(id<int(nIB.size()));	return nIB[id];		}
}
FSlideWindowItem*		CRender::getSWI					(int id)			{ VERIFY(id<int(SWIs.size()));		return &SWIs[id];	}
IRender_Target*			CRender::getTarget				()					{ return Target;										}

IRender_Light*			CRender::light_create			()					{ return Lights.Create();								}
IRender_Glow*			CRender::glow_create			()					{ return xr_new<CGlow>();								}

void					CRender::flush					()					{ r_dsgraph_render_graph	(0);						}

BOOL					CRender::occ_visible			(vis_data& P)		{ return HOM.visible(P);								}
BOOL					CRender::occ_visible			(sPoly& P)			{ return HOM.visible(P);								}
BOOL					CRender::occ_visible			(Fbox& P)			{ return HOM.visible(P);								}

void					CRender::add_Visual				(IRender_Visual*		V )	{ add_leafs_Dynamic((IRender_Visual*)V);								}
void					CRender::add_Geometry			(IRender_Visual*		V )	{ add_Static((IRender_Visual*)V,View->getMask());					}
/* void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)
		return;
	VERIFY2(_valid(P) && _valid(s) && T && verts && (s > EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}*/

void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)
		return;
	VERIFY2(_valid(P) && _valid(s) && T && verts && (s > EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}


void					CRender::clear_static_wallmarks	()
{
	Wallmarks->clear				();
}

void CRender::add_SkeletonWallmark	(intrusive_ptr<CSkeletonWallmark> wm)
{
	Wallmarks->AddSkeletonWallmark				(wm);
}
/*
void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
								   const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}
*/
void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
								   const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}

void					CRender::set_TorchEnabled(bool b	)
{ 
	m_torch_enabled = b ? TRUE : FALSE; 
}
void					CRender::add_Occluder			(Fbox2&	bb_screenspace	)
{
	HOM.occlude			(bb_screenspace);
}
void					CRender::set_Object				(IRenderable*	O )	
{ 
	val_pObject				= O;
}
void					CRender::rmNear				()
{
	IRender_Target* T	=	getTarget	();
	D3D_VIEWPORT VP		=	{0,0,(float)T->get_width(),(float)T->get_height(),0,0.02f };

	RCache.set_Viewport(&VP);
}
void					CRender::rmFar				()
{
	IRender_Target* T	=	getTarget	();
	D3D_VIEWPORT VP		=	{0,0,(float)T->get_width(),(float)T->get_height(),0.99999f,1.f };

	RCache.set_Viewport(&VP);
}
void					CRender::rmNormal			()
{
	IRender_Target* T	=	getTarget	();
	D3D_VIEWPORT VP		= {0,0,(float)T->get_width(),(float)T->get_height(),0,1.f };

	RCache.set_Viewport(&VP);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender()
:m_bFirstFrameAfterReset(false)
{
	//m_torch_enabled = FALSE; // first frame torch light disabled
	m_object_id = 0;
	//m_planar_env.set(0, 0, 0, 0);
	//m_planar_amb.set(0, 0, 0, 0);
	//m_planar_bias.set(0, 0, 0, 0);

	init_cacades();
}

CRender::~CRender()
{
}

#include "../xrEngine/GameFont.h"
void	CRender::Statistics	(CGameFont* _F)
{
	CGameFont&	F	= *_F;
	F.OutNext	(" **** LT:%2d,LV:%2d **** ",stats.l_total,stats.l_visible		);	stats.l_visible = 0;
	F.OutNext	("    S(%2d)   | (%2d)NS   ",stats.l_shadowed,stats.l_unshadowed);
	F.OutNext	("smap use[%2d], merge[%2d], finalclip[%2d]",stats.s_used,stats.s_merged-stats.s_used,stats.s_finalclip);
	stats.s_used = 0; stats.s_merged = 0; stats.s_finalclip = 0;
	F.OutSkip	();
	F.OutNext	(" **** Occ-Q(%03.1f) **** ",100.f*f32(stats.o_culled)/f32(stats.o_queries?stats.o_queries:1));
	F.OutNext	(" total  : %2d",	stats.o_queries	);	stats.o_queries = 0;
	F.OutNext	(" culled : %2d",	stats.o_culled	);	stats.o_culled	= 0;
	F.OutSkip	();
	u32	ict		= stats.ic_total + stats.ic_culled;
	F.OutNext	(" **** iCULL(%03.1f) **** ",100.f*f32(stats.ic_culled)/f32(ict?ict:1));
	F.OutNext	(" visible: %2d",	stats.ic_total	);	stats.ic_total	= 0;
	F.OutNext	(" culled : %2d",	stats.ic_culled	);	stats.ic_culled	= 0;
#ifdef DEBUG
	HOM.stats	();
#endif
}

CShaderMacros CRender::FetchShaderMacros(void)
{
	CShaderMacros macros;

	// shader model version
	macros.add(HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0, "SM_4_0", "1");
	macros.add(HW.FeatureLevel == D3D_FEATURE_LEVEL_10_1, "SM_4_1", "1");
	macros.add(HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0, "SM_5_0", "1");

	// MSAA for nvidia 8000, 9000 and 200 series
	macros.add(HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0, "iSample", "0");

	// force skinw
	macros.add(o.forceskinw, "SKIN_COLOR", "1");

	// skinning
	macros.add(m_skinning < 0, "SKIN_NONE", "1");
	macros.add(m_skinning == 0, "SKIN_0", "1");
	macros.add(m_skinning == 1, "SKIN_1", "1");
	macros.add(m_skinning == 2, "SKIN_2", "1");
	macros.add(m_skinning == 3, "SKIN_3", "1");
	macros.add(m_skinning == 4, "SKIN_4", "1");

	// shadow map
	{
		static string32 c_smap;
		_itoa(o.smapsize, c_smap, 10);
		macros.add("SMAP_size", c_smap);

		static string32 c_smap_filter;
		_itoa(r__smap_filter, c_smap_filter, 10);
		macros.add("SHADOW_FILTERING", c_smap_filter);
	}

	// parallax
	{
		static string32 c_parallax;
		_itoa(r__parallax_mode, c_parallax, 10);
		macros.add("DEFFER_IBM_MODE", c_parallax);
	}

	// debug options
	macros.add(opt(R__DBG_FLAT_WATER), "WATER_DISABLE_NORMAL", "1");
	macros.add(opt(R__DBG_NOALBEDO), "DISABLE_ALBEDO", "1");

	// path-tracing downsample 1 ray for 4 pixels
	macros.add(o.advanced_mode && o.pt_downsample, "USE_PT_DOWNSAMPLE", "1");

	// hud shadows
	macros.add(o.advanced_mode && o.hud_shadows, "USE_HUD_SHADOWS", "1");

	// colored specular
	macros.add(o.advanced_mode && o.cspecular, "USE_CSPECULAR", "1");

	// indirect lighting by RSM
	macros.add(o.advanced_mode && (o.sun_il || o.spot_il), "USE_IL", "1");

	// temporal antialiasing, TAA or TXAA
	macros.add(o.advanced_mode && (o.aa_mode == AA_TAA || o.aa_mode == AA_TAA_V2 
		|| o.txaa == TRUE), "USE_TAA", "1");

	// planar reflections
	macros.add(o.advanced_mode && o.planar, "PLANAR_MODE", "1");

	// MSAA for planar reflections
	macros.add(o.advanced_mode && o.msaa_samples_reflections > 1 &&
							 o.msaa_samples_reflections == o.msaa_samples,
						 "USE_MSAA_REFLECTIONS_WITHOUT_RESOLVE", "1");

	// SSAO
	macros.add(o.advanced_mode && o.ssao, "USE_SSAO", "1");
	macros.add(o.advanced_mode && r__ssao_mode == SSAO_PATH_TRACING,
						 "USE_SSAO_PATH_TRACING", "1");

	// water distortion
	macros.add(o.advanced_mode && opt(R__USE_WATER_DIST), "USE_WATER_DISTORT", "1");

	// MSAA for main scene
	{
		if (o.advanced_mode && o.msaa_samples > 1)
		{
			static string32 c_msaa;
			_itoa(o.msaa_samples, c_msaa, 10);
			macros.add("MSAA_SAMPLES", c_msaa);
		}
	}

	// screen space reflections
	{
		// ssr by raymarching
		if (o.advanced_mode && o.ssr)
		{
			macros.add("REFLECTIONS_QUALITY", "6");
		}
		// sky reflection
		else
		{
			macros.add("REFLECTIONS_QUALITY", "1");
		}

		// planar ssr
		macros.add(o.advanced_mode && o.ssr && o.ssr_replace, "PLANAR_SSR_MODE", "1");
	}

	// static sun
	macros.add(is_sun_static(), "DX11_STATIC_DEFFERED_RENDERER", "1");

	// soft water
	macros.add(opt(R__USE_SOFT_WATER), "USE_SOFT_WATER", "1");

	// soft particles
	macros.add(opt(R__USE_SOFT_PARTICLES), "USE_SOFT_PARTICLES", "1");

	// disable bump mapping
	macros.add(!opt(R__USE_BUMP), "DX11_STATIC_DISABLE_BUMP_MAPPING", "1");

	// depth of field
	macros.add(opt(R__USE_DOF), "USE_DOF", "1");

	// tesselation
	macros.add(o.tessellation, "USE_TESSELATION", "1");

	// variance shadow mapping
	macros.add(o.vsm, "USE_VSM", "1");

	return macros;
}