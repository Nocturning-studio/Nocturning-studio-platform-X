// CRender.cpp: implementation of the CRender class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "../xrEngine/device.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "..\xrEngine\fbasicvisual.h"
#include "..\xrEngine\CustomHUD.h"
#include "..\xrEngine\xr_object.h"
#include "..\xrEngine\fmesh.h"
#include "..\xrEngine\SkeletonCustom.h"
#include "..\xrRender\lighttrack.h"

#pragma comment(lib, "d3dx9.lib")

using namespace R_dsgraph;

CRender RImplementation;

//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_dynamic(IRender_Visual* pVisual, float cdist_sq)
{
	switch (phase)
	{
	case PHASE_NORMAL:
		return (RImplementation.L_Projector->shadowing() ? pVisual->shader->E[SE_R1_NORMAL_HQ]
														 : pVisual->shader->E[SE_R1_NORMAL_LQ])
			._get();
	case PHASE_POINT:
		return pVisual->shader->E[SE_R1_LPOINT]._get();
	case PHASE_SPOT:
		return pVisual->shader->E[SE_R1_LSPOT]._get();
	default:
		NODEFAULT;
	}
#ifdef DEBUG
	return 0;
#endif
}
//////////////////////////////////////////////////////////////////////////
ShaderElement* CRender::rimp_select_sh_static(IRender_Visual* pVisual, float cdist_sq)
{
	switch (phase)
	{
	case PHASE_NORMAL:
		return (((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < 44) ? pVisual->shader->E[SE_R1_NORMAL_HQ]
																 : pVisual->shader->E[SE_R1_NORMAL_LQ])
			._get();
	case PHASE_POINT:
		return pVisual->shader->E[SE_R1_LPOINT]._get();
	case PHASE_SPOT:
		return pVisual->shader->E[SE_R1_LSPOT]._get();
	default:
		NODEFAULT;
	}
#ifdef DEBUG
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
// update with vid_restart
void CRender::update_options()
{
	sprintf(c_vignette, "%d", ps_vignette_mode);
	
	o.aa_type = 0;
	o.msaa_samples = D3DMULTISAMPLE_NONE;
	o.csaa_samples = 0;
	o.ssaa_samples = 1;

	switch (ps_r1_aa)
	{
	case MSAA_2X:
	case MSAA_4X:
	case MSAA_8X:
		o.aa_type = MSAA;
		o.msaa_samples = (D3DMULTISAMPLE_TYPE)(ps_r1_aa % 10);
		break;
	case CSAA_4X:
	case CSAA_8X:
		o.aa_type = MSAA;
		o.msaa_samples = D3DMULTISAMPLE_NONMASKABLE;
		o.csaa_samples = (D3DMULTISAMPLE_TYPE)(ps_r1_aa % 10);
		break;
	case SSAA_2X:
	case SSAA_4X:
		o.aa_type = SSAA;
		o.ssaa_samples = (D3DMULTISAMPLE_TYPE)(ps_r1_aa % 10);
		break;
	}

	o.csaa_samples = HW.Caps.max_coverage > o.csaa_samples ? o.csaa_samples : HW.Caps.max_coverage;

	Msg("* SSAA: %dx, MSAA %dx, CSAA: %dx (max coverage: %d)", o.ssaa_samples, o.msaa_samples, o.csaa_samples*2, HW.Caps.max_coverage);
}

//////////////////////////////////////////////////////////////////////////
extern XRCORE_API u32 build_id;
void CRender::create()
{
	xrRender_console_apply_conditions();

	L_DB = 0;
	L_Shadows = 0;
	L_Projector = 0;

	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

	// c-setup
	::Device.Resources->RegisterConstantSetup("L_dynamic_pos", &r1_dlight_binder_PR);
	::Device.Resources->RegisterConstantSetup("L_dynamic_color", &r1_dlight_binder_color);
	::Device.Resources->RegisterConstantSetup("L_dynamic_xform", &r1_dlight_binder_xform);

	u32 v_dev = CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
	u32 v_need = CAP_VERSION(3, 0);
	R_ASSERT2(v_dev >= v_need,
			  make_string ("Your graphics accelerator don`t meet minimal mod system requirements (DX9.0c supporting)"));

	m_skinning = -1;

	o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;
	c_ldynamic_props = "L_dynamic_props";

	sprintf(c_build_id, "%d", build_id);

	update_options();
	Target = xr_new<CRenderTarget>();

	Models = xr_new<CModelPool>();
	L_Dynamic = xr_new<CLightR_Manager>();
	PSLibrary.OnCreate();
	HWOCC.occq_create			(occq_size);

	xrRender_apply_tf();
	::PortalTraverser.initialize();
}

void CRender::destroy()
{
	::PortalTraverser.destroy();
	HWOCC.occq_destroy			();
	PSLibrary.OnDestroy();

	xr_delete(L_Dynamic);
	xr_delete(Models);

	//*** Components
	xr_delete(Target);
	Device.seqFrame.Remove(this);

	r_dsgraph_destroy();
}

void CRender::reset_begin()
{
	xr_delete(Target);
	HWOCC.occq_destroy			();
}

void CRender::reset_end()
{
	xrRender_apply_tf();
	HWOCC.occq_create(occq_size);
	update_options();
	Target = xr_new<CRenderTarget>();
	if (L_Projector)
		L_Projector->invalidate();
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
D3DVERTEXELEMENT9* CRender::getVB_Format(int id)
{
	VERIFY(id < int(DCL.size()));
	return DCL[id].begin();
}
IDirect3DVertexBuffer9* CRender::getVB(int id)
{
	VERIFY(id < int(VB.size()));
	return VB[id];
}
IDirect3DIndexBuffer9* CRender::getIB(int id)
{
	VERIFY(id < int(IB.size()));
	return IB[id];
}
IRender_Target* CRender::getTarget()
{
	return Target;
}
FSlideWindowItem* CRender::getSWI(int id)
{
	VERIFY(id < int(SWIs.size()));
	return &SWIs[id];
}

IRender_Light* CRender::light_create()
{
	return L_DB->Create();
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
ENGINE_API extern BOOL g_bRendering;
void CRender::add_Visual(IRender_Visual* V)
{
	VERIFY(g_bRendering);
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
	VERIFY(_valid(bb_screenspace));
	HOM.occlude(bb_screenspace);
}

#include "../xrEngine/PS_instance.h"
void CRender::set_Object(IRenderable* O)
{
	VERIFY(g_bRendering);
	val_pObject = O; // NULL is OK, trust me :)
	if (val_pObject)
	{
		VERIFY(dynamic_cast<CObject*>(O) || dynamic_cast<CPS_Instance*>(O));
		if (O->renderable.pROS)
		{
			VERIFY(dynamic_cast<CROS_impl*>(O->renderable.pROS));
		}
	}
	if (PHASE_NORMAL == phase)
	{
		if (L_Shadows)
			L_Shadows->set_object(O);

		if (L_Projector)
			L_Projector->set_object(O);
	}
	else
	{
		if (L_Shadows)
			L_Shadows->set_object(0);

		if (L_Projector)
			L_Projector->set_object(0);
	}
}
void CRender::apply_object(IRenderable* O)
{
	if (0 == O)
		return;
	if (PHASE_NORMAL == phase && O->renderable_ROS())
	{
		CROS_impl& LT = *((CROS_impl*)O->renderable.pROS);
		VERIFY(dynamic_cast<CObject*>(O) || dynamic_cast<CPS_Instance*>(O));
		VERIFY(dynamic_cast<CROS_impl*>(O->renderable.pROS));
		float o_hemi = 0.5f * LT.get_hemi();
		float o_sun = 0.5f * LT.get_sun();
		RCache.set_c(c_ldynamic_props, o_sun, o_sun, o_sun, o_hemi);
		// shadowing
		if ((LT.shadow_recv_frame == Device.dwFrame) && O->renderable_ShadowReceive())
			RImplementation.L_Projector->setup(LT.shadow_recv_slot);
	}
}

// Misc
float g_fSCREEN;
static BOOL gm_Nearer = 0;

IC void gm_SetNearer(BOOL bNearer)
{
	if (bNearer != gm_Nearer)
	{
		gm_Nearer = bNearer;
		if (gm_Nearer)
			RImplementation.rmNear();
		else
			RImplementation.rmNormal();
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender()
{
}

CRender::~CRender()
{
}

extern float r_ssaDISCARD;
extern float r_ssaDONTSORT;
extern float r_ssaLOD_A, r_ssaLOD_B;
extern float r_ssaGLOD_start, r_ssaGLOD_end;
extern float r_ssaHZBvsTEX;

ICF bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
	float d1 = _1->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	float d2 = _2->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	return d1 < d2;
}

void CRender::Calculate()
{
	Device.Statistic->RenderCALC.Begin();

	// Transfer to global space to avoid deep pointer access
	IRender_Target* T = getTarget();
	float fov_factor = _sqr(90.f / Device.fFOV);
	g_fSCREEN = float(T->get_width() * T->get_height()) * fov_factor * (EPS_S + ps_r_LOD);
	r_ssaDISCARD = _sqr(ps_r_ssaDISCARD) / g_fSCREEN;
	r_ssaDONTSORT = _sqr(ps_r_ssaDONTSORT / 3) / g_fSCREEN;
	r_ssaLOD_A = _sqr(ps_r1_ssaLOD_A / 3) / g_fSCREEN;
	r_ssaLOD_B = _sqr(ps_r1_ssaLOD_B / 3) / g_fSCREEN;
	r_ssaGLOD_start = _sqr(ps_r_GLOD_ssa_start / 3) / g_fSCREEN;
	r_ssaGLOD_end = _sqr(ps_r_GLOD_ssa_end / 3) / g_fSCREEN;
	r_ssaHZBvsTEX = _sqr(ps_r_ssaHZBvsTEX / 3) / g_fSCREEN;

	// Frustum & HOM rendering
	ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
	View = 0;
	if (!ps_render_flags.test(RFLAG_EXP_MT_CALC))
	{
		HOM.Enable();
		HOM.Render(ViewBase);
	}
	gm_SetNearer(FALSE);
	phase = PHASE_NORMAL;

	// Detect camera-sector
	if (!vLastCameraPos.similar(Device.vCameraPosition, EPS_S))
	{
		CSector* pSector = (CSector*)detectSector(Device.vCameraPosition);
		if (0 == pSector)
			pSector = pLastSector;
		pLastSector = pSector;
		vLastCameraPos.set(Device.vCameraPosition);
	}

	// Check if camera is too near to some portal - if so force DualRender
	if (rmPortals)
	{
		Fvector box_radius;
		box_radius.set(EPS_L * 2, EPS_L * 2, EPS_L * 2);
		Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
		Sectors_xrc.box_query(rmPortals, Device.vCameraPosition, box_radius);
		for (int K = 0; K < Sectors_xrc.r_count(); K++)
		{
			CPortal* pPortal = (CPortal*)Portals[rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
			pPortal->bDualRender = TRUE;
		}
	}
	//
	if (L_DB)
		L_DB->Update();

	// Main process
	marker++;
	if (pLastSector)
	{
		// Traverse sector/portal structure
		PortalTraverser.traverse(pLastSector, ViewBase, Device.vCameraPosition, Device.mFullTransform,
								 CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE);

		// Determine visibility for static geometry hierrarhy
		if (psDeviceFlags.test(rsDrawStatic))
		{
			for (u32 s_it = 0; s_it < PortalTraverser.r_sectors.size(); s_it++)
			{
				CSector* sector = (CSector*)PortalTraverser.r_sectors[s_it];
				IRender_Visual* root = sector->root();
				for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
				{
					set_Frustum(&(sector->r_frustums[v_it]));
					add_Geometry(root);
				}
			}
		}

		// Traverse object database
		if (psDeviceFlags.test(rsDrawDynamic))
		{
			g_SpatialSpace->q_frustum(lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE + STYPE_LIGHTSOURCE,
									  ViewBase);

			// Exact sorting order (front-to-back)
			std::sort(lstRenderables.begin(), lstRenderables.end(), pred_sp_sort);

			// Determine visibility for dynamic part of scene
			set_Object(0);
			g_pGameLevel->pHUD->Render_First(); // R1 shadows
			g_pGameLevel->pHUD->Render_Last();
			u32 uID_LTRACK = 0xffffffff;
			if (phase == PHASE_NORMAL)
			{
				uLastLTRACK++;
				if (lstRenderables.size())
					uID_LTRACK = uLastLTRACK % lstRenderables.size();

				// update light-vis for current entity / actor
				CObject* O = g_pGameLevel->CurrentViewEntity();
				if (O)
				{
					CROS_impl* R = (CROS_impl*)O->ROS();
					if (R)
						R->update(O);
				}
			}
			for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
			{
				ISpatial* spatial = lstRenderables[o_it];
				spatial->spatial_updatesector();
				CSector* sector = (CSector*)spatial->spatial.sector;
				if (0 == sector)
					continue; // disassociated from S/P structure
				if (PortalTraverser.i_marker != sector->r_marker)
					continue; // inactive (untouched) sector
				for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
				{
					set_Frustum(&(sector->r_frustums[v_it]));
					if (!View->testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
						continue;

					if (spatial->spatial.type & STYPE_RENDERABLE)
					{
						// renderable
						IRenderable* renderable = spatial->dcast_Renderable();
						if (0 == renderable)
						{
							// It may be an glow
							CGlow* glow = dynamic_cast<CGlow*>(spatial);
							VERIFY(glow);
							L_Glows->add(glow);
						}
						else
						{
							// Occlusion
							vis_data& v_orig = renderable->renderable.visual->vis;
							vis_data v_copy = v_orig;
							v_copy.box.xform(renderable->renderable.xform);
							BOOL bVisible = HOM.visible(v_copy);
							v_orig.accept_frame = v_copy.accept_frame;
							v_orig.marker = v_copy.marker;
							v_orig.hom_frame = v_copy.hom_frame;
							v_orig.hom_tested = v_copy.hom_tested;
							if (!bVisible)
								break; // exit loop on frustums

							// rendering
							if (o_it == uID_LTRACK && renderable->renderable_ROS())
							{
								// track lighting environment
								CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();
								T->update(renderable);
							}
							set_Object(renderable);
							renderable->renderable_Render();
							set_Object(0); //? is it needed at all
						}
					}
					else
					{
						VERIFY(spatial->spatial.type & STYPE_LIGHTSOURCE);
						// lightsource
						light* L = (light*)spatial->dcast_Light();
						VERIFY(L);
						if (L->spatial.sector)
						{
							vis_data& vis = L->get_homdata();
							if (HOM.visible(vis))
								L_DB->add_light(L);
						}
					}
					break; // exit loop on frustums
				}
			}
		}

		// Calculate miscelaneous stuff
		L_Shadows->calculate();
		L_Projector->calculate();
	}
	else
	{
		set_Object(0);
		/*
		g_pGameLevel->pHUD->Render_First					();
		g_pGameLevel->pHUD->Render_Last						();

		// Calculate miscelaneous stuff
		L_Shadows->calculate								();
		L_Projector->calculate								();
		*/
	}

	// End calc
	Device.Statistic->RenderCALC.End();
}

void CRender::rmNear()
{
	IRender_Target* T = getTarget();
	D3DVIEWPORT9 VP = {0, 0, T->get_width(), T->get_height(), 0, 0.02f};
	CHK_DX(HW.pDevice->SetViewport(&VP));
}
void CRender::rmFar()
{
	IRender_Target* T = getTarget();
	D3DVIEWPORT9 VP = {0, 0, T->get_width(), T->get_height(), 0.99999f, 1.f};
	CHK_DX(HW.pDevice->SetViewport(&VP));
}
void CRender::rmNormal()
{
	IRender_Target* T = getTarget();
	D3DVIEWPORT9 VP = {0, 0, T->get_width(), T->get_height(), 0, 1.f};
	CHK_DX(HW.pDevice->SetViewport(&VP));
}

extern u32 g_r;
void CRender::Render()
{
	g_r = 1;
	Device.Statistic->RenderDUMP.Begin();
	// Begin
	Target->Begin();
	o.vis_intersect = FALSE;
	phase = PHASE_NORMAL;

	g_pGamePersistent->Environment().RenderSky();	 // sky / sun
	g_pGamePersistent->Environment().RenderClouds(); // clouds

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	r_dsgraph_render_hud();	   // hud
	Target->EnableAlphaMSAA();
	r_dsgraph_render_graph(0); // normal level
	if (Details)
		Details->Render();				// grass / details
	r_dsgraph_render_lods(true, false); // lods - FB
	Target->DisableAlphaMSAA();

	r_pmask(true, false); // disable priority "1"
	o.vis_intersect = TRUE;
	HOM.Disable();
	L_Dynamic->render(); // addititional light sources
	if (Wallmarks)
	{
		//g_r = 0;
		Wallmarks->Render(); // wallmarks has priority as normal geometry
	}
	HOM.Enable();
	o.vis_intersect = FALSE;
	phase = PHASE_NORMAL;
	r_pmask(true, true); // enable priority "0" and "1"
	if (L_Shadows)
		L_Shadows->render();			// ... and shadows
	r_dsgraph_render_lods(false, true); // lods - FB
	r_dsgraph_render_graph(1);			// normal level, secondary priority
	PortalTraverser.fade_render();		// faded-portals
	r_dsgraph_render_sorted();			// strict-sorted geoms
	if (L_Glows)
		L_Glows->Render(); // glows
	if (ps_render_flags.test(RFLAG_LENS_FLARES))
		g_pGamePersistent->Environment().RenderFlares(); // lens-flares
	g_pGamePersistent->Environment().RenderLast();		 // rain/thunder-bolts

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	// Postprocess, if necessary
	Target->End();
	if (L_Projector)
		L_Projector->finalize();

	// HUD
	Device.Statistic->RenderDUMP.End();
}

void CRender::ApplyBlur4(FVF::TL4uv* pv, u32 w, u32 h, float k)
{
	float _w = float(w);
	float _h = float(h);
	float kw = (1.f / _w) * k;
	float kh = (1.f / _h) * k;
	Fvector2 p0, p1;
	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h);
	u32 _c = 0xffffffff;

	// Fill vertex buffer
	pv->p.set(EPS, float(_h + EPS), EPS, 1.f);
	pv->color = _c;
	pv->uv[0].set(p0.x - kw, p1.y - kh);
	pv->uv[1].set(p0.x + kw, p1.y + kh);
	pv->uv[2].set(p0.x + kw, p1.y - kh);
	pv->uv[3].set(p0.x - kw, p1.y + kh);
	pv++;
	pv->p.set(EPS, EPS, EPS, 1.f);
	pv->color = _c;
	pv->uv[0].set(p0.x - kw, p0.y - kh);
	pv->uv[1].set(p0.x + kw, p0.y + kh);
	pv->uv[2].set(p0.x + kw, p0.y - kh);
	pv->uv[3].set(p0.x - kw, p0.y + kh);
	pv++;
	pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f);
	pv->color = _c;
	pv->uv[0].set(p1.x - kw, p1.y - kh);
	pv->uv[1].set(p1.x + kw, p1.y + kh);
	pv->uv[2].set(p1.x + kw, p1.y - kh);
	pv->uv[3].set(p1.x - kw, p1.y + kh);
	pv++;
	pv->p.set(float(_w + EPS), EPS, EPS, 1.f);
	pv->color = _c;
	pv->uv[0].set(p1.x - kw, p0.y - kh);
	pv->uv[1].set(p1.x + kw, p0.y + kh);
	pv->uv[2].set(p1.x + kw, p0.y - kh);
	pv->uv[3].set(p1.x - kw, p0.y + kh);
	pv++;
}

#include "..\xrEngine\GameFont.h"
void CRender::Statistics(CGameFont* _F)
{
	CGameFont& F = *_F;
	F.OutNext(" **** Occ-Q(%03.1f) **** ", 100.f * f32(stats.o_culled) / f32(stats.o_queries ? stats.o_queries : 1));
	F.OutNext(" total  : %2d", stats.o_queries);
	stats.o_queries = 0;
	F.OutNext(" culled : %2d", stats.o_culled);
	stats.o_culled = 0;
	F.OutSkip();
#ifdef DEBUG
	HOM.stats();
#endif
}

CShaderMacros CRender::FetchShaderMacros(void)
{
	CShaderMacros macros;

	// build id
	macros.add("BUILD_ID", c_build_id);

	// skinning
	macros.add(m_skinning < 0, "SKIN_NONE", "1");
	macros.add(0 == m_skinning, "SKIN_0", "1");
	macros.add(1 == m_skinning, "SKIN_1", "1");
	macros.add(2 == m_skinning, "SKIN_2", "1");
	macros.add(3 == m_skinning, "SKIN_3", "1");
	macros.add(4 == m_skinning, "SKIN_4", "1");

	macros.add(o.forceskinw, "SKIN_COLOR", "1");

	// options
	macros.add(ps_render_flags.test(RFLAG_SEPIA), "USE_SEPIA", "1");
	macros.add(ps_render_flags.test(RFLAG_CHROMATIC_ABBERATION), "USE_CHROMATIC_ABBERATION", "1");
	macros.add(ps_vignette_mode > 0, "VIGNETTE_MODE", c_vignette);

	return macros;
}