#include "stdafx.h"
#include "r2.h"
#include "..\xrEngine\fbasicvisual.h"
#include "..\xrEngine\xr_object.h"
#include "..\xrEngine\CustomHUD.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "..\xrEngine\SkeletonCustom.h"
#include "..\xrRender\LightTrack.h"
#include <boost/crc.hpp>
#include "../xrEngine\r_constants.h"

#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "d3dx11.lib")
//////////////////////////////////////////////////////////////////////////
CRender RImplementation;
//////////////////////////////////////////////////////////////////////////
class CGlow : public IRender_Glow
{
  public:
	bool bActive;

  public:
	CGlow() : bActive(false)
	{
	}
	virtual void set_active(bool b)
	{
		bActive = b;
	}
	virtual bool get_active()
	{
		return bActive;
	}
	virtual void set_position(const Fvector& P)
	{
	}
	virtual void set_direction(const Fvector& D)
	{
	}
	virtual void set_radius(float R)
	{
	}
	virtual void set_texture(LPCSTR name)
	{
	}
	virtual void set_color(const Fcolor& C)
	{
	}
	virtual void set_color(float r, float g, float b)
	{
	}
};

float r_dtex_range = 50.f;
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(IRender_Visual* pVisual, float cdist_sq)
{
	int id = SE_R2_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(IRender_Visual* pVisual, float cdist_sq)
{
	int id = SE_R2_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}
//////////////////////////////////////////////////////////////////////////
static class cl_parallax : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float h = ps_r2_df_parallax_h;
		RCache.set_c(C, h, -(h / 2.0f), 1.f / r_dtex_range, 1.f / r_dtex_range);
	}
} binder_parallax;
//////////////////////////////////////////////////////////////////////////
/* static class cl_sun_far : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float fValue = ps_r2_sun_far;
		RCache.set_c(C, fValue, fValue, fValue, 0);
	}
} binder_sun_far;*/
//////////////////////////////////////////////////////////////////////////
/* static class cl_sun_dir : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		light* sun = (light*)RImplementation.Lights.sun_adapted._get();

		Fvector L_dir;
		Device.mView.transform_dir(L_dir, sun->direction);
		L_dir.normalize();

		RCache.set_c(C, L_dir.x, L_dir.y, L_dir.z, 0);
	}
} binder_sun_dir;*/
//////////////////////////////////////////////////////////////////////////
/* static class cl_sun_color : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		light* sun = (light*)RImplementation.Lights.sun_adapted._get();
		RCache.set_c(C, sun->color.r, sun->color.g, sun->color.b, 0);
	}
} binder_sun_color;*/
//////////////////////////////////////////////////////////////////////////
static class cl_hdr_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c("hdr_params", ps_r_hdr_white_intensity, ps_r_hdr_gamma, ps_r_hdr_brightness, 0);
	}
} binder_hdr_params;
//////////////////////////////////////////////////////////////////////////
void CheckHWSupporting()
{
	R_ASSERT2(CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor) >= CAP_VERSION(3, 0),
			  make_string("Your graphics accelerator don`t meet minimal mod system requirements (DX9.0c supporting)"));

	R_ASSERT2(HW.Caps.raster.dwInstructions >= 512, 
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (Instructions count less than 512)"));


	R_ASSERT2(HW.Caps.raster.dwMRT_count >= 3,
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (Multiple render targets)"));

	R_ASSERT2(HW.Caps.raster.b_MRT_mixdepth, 
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (Multiple render targets independent depths)"));

	R_ASSERT2(HW.support(D3DFMT_D24X8, D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL), 
		make_string("Your graphics accelerator don`t meet minimal mod system requirements (D24X8 rendertarget format)"));

	R_ASSERT2(HW.support(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_FILTER),
			  make_string("Your graphics accelerator don`t meet minimal mod system requirements (Floating point 16-bits rendertarget format)"));

	R_ASSERT2(HW.support(D3DFMT_A16B16G16R16F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING),
			  make_string("Your graphics accelerator don`t meet minimal mod system requirements (Post-Pixel Shader blending)"));
}
//////////////////////////////////////////////////////////////////////////
// update with vid_restart
void CRender::update_options()
{
	o.smapsize = ps_r2_sun_quality <= 1 ? 1024 :
				 ps_r2_sun_quality <= 4 ? 1536 :
				 ps_r2_sun_quality <= 5 ? 2048 : 0;

	o.sun_depth_near_bias = -0.0001f;
	o.sun_depth_near_scale = 1.0f;

	o.sun_depth_far_bias = -0.001f;
	o.sun_depth_far_scale = 1.0f;

	o.nvstencil = (HW.Caps.id_vendor == 0x10DE) && (HW.Caps.id_device >= 0x40);
	o.nvdbt = HW.support((D3DFORMAT)MAKEFOURCC('N', 'V', 'D', 'B'), D3DRTYPE_SURFACE, 0);

	o.use_ssao = ps_r2_ao > 0;
	o.use_soft_water = ps_r2_postprocess_flags.test(R2FLAG_SOFT_PARTICLES);
	o.use_soft_particles = ps_r2_postprocess_flags.test(R2FLAG_SOFT_PARTICLES);
	o.use_atest_aa = ps_r2_aa_transluency > 0;

	bool intz = HW.support((D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z'), D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL);
	bool rawz = HW.support((D3DFORMAT)MAKEFOURCC('R', 'A', 'W', 'Z'), D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL);

	Msg("* depth format 'INTZ' check: %u", intz);
	Msg("* depth format 'RAWZ' check: %u", rawz);

	// check if we can optimize G-Buffer
	o.gbuffer_opt_mode = ps_r2_gbuffer_opt;

	if (ps_r2_gbuffer_opt == 2)
	{
		if (intz)
			o.gbuffer_opt_mode = 2;
		else if (rawz)
			o.gbuffer_opt_mode = 3;
		else
			o.gbuffer_opt_mode = 1;
	}	
	
	sprintf(c_gbuffer_opt_mode, "%d", o.gbuffer_opt_mode);

	switch (o.gbuffer_opt_mode)
	{
	case 3:
		Msg("* G-Buffer optimization mode: full 'RAWZ'");
		break;
	case 2:
		Msg("* G-Buffer optimization mode: full 'INTZ'");
		break;
	case 1:
		Msg("* G-Buffer optimization mode: partial");
		break;
	default:
		Msg("* G-Buffer optimization mode: none");
	}

	sprintf(c_smapsize, "%d", o.smapsize);
	sprintf(c_debugview, "%d", ps_r2_debug_render);
	sprintf(c_vignette, "%d", ps_vignette_mode);
	sprintf(c_aa_type, "%d", ps_r2_aa);
	sprintf(c_fxaa_quality, "%d", ps_r2_aa_quality);
	sprintf(c_bloom_quality, "%d", ps_r2_bloom_quality);
	sprintf(c_shadow_filter, "%d", ps_r2_shadow_filtering);
	sprintf(c_bump_quality, "%d", ps_r2_bump_quality);
	sprintf(c_ao_quality, "%d", ps_r2_ao_quality);
}

//////////////////////////////////////////////////////////////////////
CShaderMacros CRender::FetchShaderMacros()
{
	CShaderMacros macros;

	// build id
	macros.add("BUILD_ID", c_build_id);

	// debug view
	macros.add("DEBUG_VIEW_MODE", c_debugview);

	// skinning
	macros.add(m_skinning < 0, "SKIN_NONE", "1");
	macros.add(0 == m_skinning, "SKIN_0", "1");
	macros.add(1 == m_skinning, "SKIN_1", "1");
	macros.add(2 == m_skinning, "SKIN_2", "1");
	macros.add(3 == m_skinning, "SKIN_3", "1");
	macros.add(4 == m_skinning, "SKIN_4", "1");

	macros.add(o.forceskinw, "SKIN_COLOR", "1");

	macros.add("SMAP_SIZE", c_smapsize);
	macros.add("SHADOW_FILTER", c_shadow_filter);

	macros.add("VIGNETTE_MODE", c_vignette);

	macros.add(ps_render_flags.test(RFLAG_SEPIA), "USE_SEPIA", "1");

	macros.add(ps_render_flags.test(RFLAG_CHROMATIC_ABBERATION), "USE_CHROMATIC_ABBERATION", "1");

	macros.add("BLOOM_QUALITY", c_bloom_quality);

	macros.add("BUMP_QUALITY", c_bump_quality);

	macros.add("GBUFFER_OPT_MODE", c_gbuffer_opt_mode);

	macros.add(o.use_ssao, "AO_ENABLED", "1");

	macros.add(o.use_soft_water, "USE_SOFT_WATER", "1");

	macros.add(o.use_soft_particles, "USE_SOFT_PARTICLES", "1");

	macros.add(o.use_atest_aa, "ALPHA_TEST_AA", "1");

	macros.add("AA_TYPE", c_aa_type);

	macros.add("AA_QUALITY", c_fxaa_quality);

	macros.add("AO_QUALITY", c_ao_quality);

	return macros;
}
//////////////////////////////////////////////////////////////////////////
extern XRCORE_API u32 build_id;
void CRender::create()
{
	CheckHWSupporting();
	xrRender_console_apply_conditions();

	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

	m_skinning = -1;

	sprintf(c_build_id, "%d", build_id);

	// options
	o.noshadows = (strstr(Core.Params, "-noshadows")) ? TRUE : FALSE;
	o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;

	// constants
	::Device.Resources->RegisterConstantSetup("parallax_heigt", &binder_parallax);
	//::Device.Resources->RegisterConstantSetup("sun_far", &binder_sun_far);
	//::Device.Resources->RegisterConstantSetup("sun_dir", &binder_sun_dir);
	//::Device.Resources->RegisterConstantSetup("sun_color", &binder_sun_color);
	::Device.Resources->RegisterConstantSetup("hdr_params", &binder_hdr_params);

	c_lmaterial = "L_material";
	c_sbase = "s_base";

	update_options();
	Target = xr_new<CRenderTarget>(); // Main target

	Models = xr_new<CModelPool>();
	PSLibrary.OnCreate();
	HWOCC.occq_create(occq_size);

	// rmNormal					();
	marker = 0;
	ZeroMemory(q_sync_point, sizeof(q_sync_point));
#pragma message(Reminder("Fix sync points"))
	//for (u32 i = 0; i < HW.Caps.iGPUNum; ++i)
	//	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[i]));

	xrRender_apply_tf();
	::PortalTraverser.initialize();
}

void CRender::destroy()
{
	::PortalTraverser.destroy();
	for (u32 i = 0; i < HW.Caps.iGPUNum; ++i)
		_RELEASE(q_sync_point[i]);
	HWOCC.occq_destroy();
	xr_delete(Models);
	xr_delete(Target);
	PSLibrary.OnDestroy();
	Device.seqFrame.Remove(this);
	r_dsgraph_destroy(); // FIX BY IXRAY (THANKS BY DEATHMAN)
}

void CRender::reset_begin()
{
	// Update incremental shadowmap-visibility solver
	// BUG-ID: 10646
	{
		u32 it = 0;
		for (it = 0; it < Lights_LastFrame.size(); it++)
		{
			if (0 == Lights_LastFrame[it])
				continue;
			try
			{
				Lights_LastFrame[it]->svis.resetoccq();
			}
			catch (...)
			{
				Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear();
	}

	xr_delete(Target);
	HWOCC.occq_destroy();
	for (u32 i = 0; i < HW.Caps.iGPUNum; ++i)
		_RELEASE(q_sync_point[i]);
}

void CRender::reset_end()
{
#pragma message(Reminder("Fix sync points"))
	//for (u32 i = 0; i < HW.Caps.iGPUNum; ++i)
	//	R_CHK(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &q_sync_point[i]));
	HWOCC.occq_create(occq_size);

	update_options();
	Target = xr_new<CRenderTarget>();

	xrRender_apply_tf();

	// Set this flag true to skip the first render frame,
	// that some data is not ready in the first frame (for example device camera position)
	m_bFirstFrameAfterReset = true;
}

void CRender::OnFrame()
{
	Models->DeleteQueue();
	if (ps_render_flags.test(RFLAG_EXP_MT_CALC))
	{
		// MT-details (@front)
		Device.seqParallel.insert(Device.seqParallel.begin(),
								  fastdelegate::FastDelegate0<>(Details, &CDetailManager::MT_CALC));

		// MT-HOM (@front)
		Device.seqParallel.insert(Device.seqParallel.begin(), fastdelegate::FastDelegate0<>(&HOM, &CHOM::MT_RENDER));
	}
}

BOOL CRender::is_sun()
{
	Fcolor sun_color = ((light*)Lights.sun_adapted._get())->color;
	return (ps_r2_lighting_flags.test(R2FLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS));
}

// Implementation
IRender_ObjectSpecific* CRender::ros_create(IRenderable* parent)
{
	return xr_new<CROS_impl>();
}
void CRender::ros_destroy(IRender_ObjectSpecific*& p)
{
	xr_delete(p);
}
IRender_Visual* CRender::model_Create(LPCSTR name, IReader* data)
{
	return Models->Create(name, data);
}
IRender_Visual* CRender::model_CreateChild(LPCSTR name, IReader* data)
{
	return Models->CreateChild(name, data);
}
IRender_Visual* CRender::model_Duplicate(IRender_Visual* V)
{
	return Models->Instance_Duplicate(V);
}
void CRender::model_Delete(IRender_Visual*& V, BOOL bDiscard)
{
	Models->Delete(V, bDiscard);
}
IRender_DetailModel* CRender::model_CreateDM(IReader* F)
{
	CDetail* D = xr_new<CDetail>();
	D->Load(F);
	return D;
}
void CRender::model_Delete(IRender_DetailModel*& F)
{
	if (F)
	{
		CDetail* D = (CDetail*)F;
		D->Unload();
		xr_delete(D);
		F = NULL;
	}
}
IRender_Visual* CRender::model_CreatePE(LPCSTR name)
{
	PS::CPEDef* SE = PSLibrary.FindPED(name);
	R_ASSERT3(SE, "Particle effect doesn't exist", name);
	return Models->CreatePE(SE);
}
IRender_Visual* CRender::model_CreateParticles(LPCSTR name)
{
	PS::CPEDef* SE = PSLibrary.FindPED(name);
	if (SE)
		return Models->CreatePE(SE);
	else
	{
		PS::CPGDef* SG = PSLibrary.FindPGD(name);
		R_ASSERT3(SG, "Particle effect or group doesn't exist", name);
		return Models->CreatePG(SG);
	}
}
void CRender::models_Prefetch()
{
	Models->Prefetch();
}
void CRender::models_Clear(BOOL b_complete)
{
	Models->ClearPool(b_complete);
}

ref_shader CRender::getShader(int id)
{
	VERIFY(id < int(Shaders.size()));
	return Shaders[id];
}
IRender_Portal* CRender::getPortal(int id)
{
	VERIFY(id < int(Portals.size()));
	return Portals[id];
}
IRender_Sector* CRender::getSector(int id)
{
	VERIFY(id < int(Sectors.size()));
	return Sectors[id];
}
IRender_Sector* CRender::getSectorActive()
{
	return pLastSector;
}
IRender_Visual* CRender::getVisual(int id)
{
	VERIFY(id < int(Visuals.size()));
	return Visuals[id];
}
D3DVERTEXELEMENT9* CRender::getVB_Format(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xDC.size()));
		return xDC[id].begin();
	}
	else
	{
		VERIFY(id < int(nDC.size()));
		return nDC[id].begin();
	}
}

#pragma mssage(Reminder("Fix VB/IB D3D9 --> D3D11 pointer conversion"))
IDirect3DVertexBuffer9* CRender::getVB(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xVB.size()));
		return (IDirect3DVertexBuffer9*)xVB[id];
	}
	else
	{
		VERIFY(id < int(nVB.size()));
		return (IDirect3DVertexBuffer9*)nVB[id];
	}
}
IDirect3DIndexBuffer9* CRender::getIB(int id, BOOL _alt)
{
	if (_alt)
	{
		VERIFY(id < int(xIB.size()));
		return (IDirect3DIndexBuffer9*)xIB[id];
	}
	else
	{
		VERIFY(id < int(nIB.size()));
		return (IDirect3DIndexBuffer9*)nIB[id];
	}
}
FSlideWindowItem* CRender::getSWI(int id)
{
	VERIFY(id < int(SWIs.size()));
	return &SWIs[id];
}
IRender_Target* CRender::getTarget()
{
	return Target;
}

IRender_Light* CRender::light_create()
{
	return Lights.Create();
}
IRender_Glow* CRender::glow_create()
{
	return xr_new<CGlow>();
}

void CRender::flush()
{
	r_dsgraph_render_graph(0);
}

BOOL CRender::occ_visible(vis_data& P)
{
	return HOM.visible(P);
}
BOOL CRender::occ_visible(sPoly& P)
{
	return HOM.visible(P);
}
BOOL CRender::occ_visible(Fbox& P)
{
	return HOM.visible(P);
}

void CRender::add_Visual(IRender_Visual* V)
{
	add_leafs_Dynamic(V);
}
void CRender::add_Geometry(IRender_Visual* V)
{
	add_Static(V, View->getMask());
}
void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)
		return;
	VERIFY2(_valid(P) && _valid(s) && T && verts && (s > EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::clear_static_wallmarks()
{
	Wallmarks->clear();
}

void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm)
{
	Wallmarks->AddSkeletonWallmark(wm);
}
void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start,
								   const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}
void CRender::add_Occluder(Fbox2& bb_screenspace)
{
	HOM.occlude(bb_screenspace);
}
void CRender::set_Object(IRenderable* O)
{
	val_pObject = O;
}
void CRender::rmNear()
{
	IRender_Target* T = getTarget();
	D3D11_VIEWPORT VP = {0, 0, (float)T->get_width(), (float)T->get_height(), 0, 0.02f};

	HW.pContext->RSSetViewports(1, &VP);
	// CHK_DX				(HW.pDevice->SetViewport(&VP));
}
void CRender::rmFar()
{
	IRender_Target* T = getTarget();
	D3D11_VIEWPORT VP = {0, 0, (float)T->get_width(), (float)T->get_height(), 0.99999f, 1.f};

	HW.pContext->RSSetViewports(1, &VP);
	// CHK_DX				(HW.pDevice->SetViewport(&VP));
}
void CRender::rmNormal()
{
	IRender_Target* T = getTarget();
	D3D11_VIEWPORT VP = {0, 0, (float)T->get_width(), (float)T->get_height(), 0, 1.f};

	HW.pContext->RSSetViewports(1, &VP);
	// CHK_DX				(HW.pDevice->SetViewport(&VP));
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender() : m_bFirstFrameAfterReset(false)
{
	init_cacades();
}

CRender::~CRender()
{
}

#include "..\xrEngine\GameFont.h"
// #include "../xrRender/xrRender_console.cpp"
void CRender::Statistics(CGameFont* _F)
{
	CGameFont& F = *_F;
	F.OutNext(" **** LT:%2d,LV:%2d **** ", stats.l_total, stats.l_visible);
	stats.l_visible = 0;
	F.OutNext("    S(%2d)   | (%2d)NS   ", stats.l_shadowed, stats.l_unshadowed);
	F.OutNext("smap use[%2d], merge[%2d], finalclip[%2d]", stats.s_used, stats.s_merged - stats.s_used,
			  stats.s_finalclip);
	stats.s_used = 0;
	stats.s_merged = 0;
	stats.s_finalclip = 0;
	F.OutSkip();
	F.OutNext(" **** Occ-Q(%03.1f) **** ", 100.f * f32(stats.o_culled) / f32(stats.o_queries ? stats.o_queries : 1));
	F.OutNext(" total  : %2d", stats.o_queries);
	stats.o_queries = 0;
	F.OutNext(" culled : %2d", stats.o_culled);
	stats.o_culled = 0;
	F.OutSkip();
	u32 ict = stats.ic_total + stats.ic_culled;
	F.OutNext(" **** iCULL(%03.1f) **** ", 100.f * f32(stats.ic_culled) / f32(ict ? ict : 1));
	F.OutNext(" visible: %2d", stats.ic_total);
	stats.ic_total = 0;
	F.OutNext(" culled : %2d", stats.ic_culled);
	stats.ic_culled = 0;
#ifdef DEBUG
	HOM.stats();
#endif
}

