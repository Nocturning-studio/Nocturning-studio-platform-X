#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\fbasicvisual.h"
#include "..\xrEngine\customhud.h"
#include "..\xrEngine\xr_object.h"

IC bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
	float d1 = _1->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	float d2 = _2->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	return d1 < d2;
}

void CRender::render_main(Fmatrix& m_ViewProjection, bool _fportals)
{
	OPTICK_EVENT("CRender::render_main");

	marker++;

	// Calculate sector(s) and their objects
	if (pLastSector)
	{
		//!!!
		//!!! BECAUSE OF PARALLEL HOM RENDERING TRY TO DELAY ACCESS TO HOM AS MUCH AS POSSIBLE
		//!!!
		{
			// Traverse object database
			g_SpatialSpace->q_frustum(lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE + STYPE_LIGHTSOURCE,
									  ViewBase);

			// (almost) Exact sorting order (front-to-back)
			std::sort(lstRenderables.begin(), lstRenderables.end(), pred_sp_sort);

			// Determine visibility for dynamic part of scene
			set_Object(0);
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

				// update light-vis for selected entity
				// track lighting environment
				if (lstRenderables.size())
				{
					IRenderable* renderable = lstRenderables[uID_LTRACK]->dcast_Renderable();
					if (renderable)
					{
						CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();
						if (T)
							T->update(renderable);
					}
				}
			}
		}

		// Traverse sector/portal structure
		PortalTraverser.traverse(
			pLastSector, ViewBase, Device.vCameraPosition, m_ViewProjection,
			CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE
			//. disabled scissoring (HW.Caps.bScissor?CPortalTraverser::VQ_SCISSOR:0)	// generate scissoring info
		);

		// Determine visibility for static geometry hierrarhy
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

		// Traverse frustums
		for (u32 o_it = 0; o_it < lstRenderables.size(); o_it++)
		{
			ISpatial* spatial = lstRenderables[o_it];
			spatial->spatial_updatesector();
			CSector* sector = (CSector*)spatial->spatial.sector;
			if (0 == sector)
				continue; // disassociated from S/P structure

			if (spatial->spatial.type & STYPE_LIGHTSOURCE)
			{
				// lightsource
				light* L = (light*)(spatial->dcast_Light());
				VERIFY(L);
				float lod = L->get_LOD();
				if (lod > EPS_L)
				{
					vis_data& vis = L->get_homdata();
					if (HOM.visible(vis))
						Lights.add_light(L);
				}
				continue;
			}

			if (PortalTraverser.i_marker != sector->r_marker)
				continue; // inactive (untouched) sector
			for (u32 v_it = 0; v_it < sector->r_frustums.size(); v_it++)
			{
				CFrustum& view = sector->r_frustums[v_it];
				if (!view.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
					continue;

				if (spatial->spatial.type & STYPE_RENDERABLE)
				{
					// renderable
					IRenderable* renderable = spatial->dcast_Renderable();
					VERIFY(renderable);

					// Occlusion
					vis_data& v_orig = renderable->renderable.visual->vis;
					vis_data v_copy = v_orig;
					v_copy.box.xform(renderable->renderable.xform);
					BOOL bVisible = HOM.visible(v_copy);
					v_orig.marker = v_copy.marker;
					v_orig.accept_frame = v_copy.accept_frame;
					v_orig.hom_frame = v_copy.hom_frame;
					v_orig.hom_tested = v_copy.hom_tested;
					if (!bVisible)
						break; // exit loop on frustums

					// Rendering
					set_Object(renderable);
					renderable->renderable_Render();
					set_Object(0);
				}
				break; // exit loop on frustums
			}
		}
		if (g_pGameLevel && (phase == PHASE_NORMAL || phase == PHASE_DEPTH_PREPASS))
			g_pGameLevel->pHUD->Render_Last(); // HUD
	}
	else
	{
		set_Object(0);
		if (g_pGameLevel && (phase == PHASE_NORMAL || phase == PHASE_DEPTH_PREPASS))
			g_pGameLevel->pHUD->Render_Last(); // HUD
	}
}

void CRender::render_menu()
{
	OPTICK_EVENT("CRender::render_menu");

	//	Globals
	RCache.set_CullMode(CULL_CCW);
	RCache.set_Stencil(FALSE);
	RCache.set_ColorWriteEnable();

	// Main Render
	{
		Target->u_setrt(Target->rt_Generic_0, NULL, NULL, NULL, HW.pBaseZB); // LDR RT
		g_pGamePersistent->OnRenderPPUI_main();					 // PP-UI
	}

	// Distort
	{
		Target->u_setrt(Target->rt_Distortion_Mask, NULL, NULL, NULL, HW.pBaseZB); // Now RT is a distortion mask
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 0, 127), 1.0f, 0L));
		g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI
	}

	// Actual Display
	Target->u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, NULL, HW.pBaseZB);
	RCache.set_Shader(Target->s_menu);
	RCache.set_Geometry(Target->g_menu);

	Fvector2 p0, p1;
	u32 Offset;
	u32 C = color_rgba(255, 255, 255, 255);
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	float d_Z = EPS_S;
	float d_W = 1.f;
	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h);

	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, Target->g_menu->vb_stride, Offset);
	pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, Target->g_menu->vb_stride);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRender::render_forward()
{
	OPTICK_EVENT("CRender::render_forward");

	VERIFY(0 == mapDistort.size());

	//******* Main render - second order geometry (the one, that doesn't support deffering)
	//.todo: should be done inside "combine" with estimation of of autoexposure, tone-mapping, etc.
	{
		// level
		r_pmask(false, true); // enable priority "1"
		phase = PHASE_NORMAL;
		render_main(Device.mFullTransform, false); //

		Target->enable_anisotropy_filtering();

		if (psDeviceFlags.test(rsWireframe))
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

		r_dsgraph_render_graph(1); // normal level, secondary priority
		r_dsgraph_render_sorted(); // strict-sorted geoms

		if (psDeviceFlags.test(rsWireframe))
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

		Target->disable_anisotropy_filtering();

		g_pGamePersistent->Environment().RenderThunderbolt();
		g_pGamePersistent->Environment().RenderRain();
	}
}

extern u32 g_r;
void CRender::Render()
{
	OPTICK_EVENT("CRender::Render");

	Device.Statistic->RenderCALC.Begin();
	g_r = 1;
	VERIFY(0 == mapDistort.size());

	bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;
	if (_menu_pp)
	{
		render_menu();
		return;
	};

	if (!(g_pGameLevel && g_pGameLevel->pHUD))
		return;

	if (m_bFirstFrameAfterReset)
	{
		m_bFirstFrameAfterReset = false;
		return;
	}

	// Configure
	Fcolor sun_color = ((light*)Lights.sun_adapted._get())->color;
	BOOL bSUN = ps_r_lighting_flags.test(RFLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);

	// HOM
	ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	View = 0;
	if (!ps_render_flags.test(RFLAG_EXP_MT_CALC))
	{
		HOM.Enable();
		HOM.Render(ViewBase);
	}

	//*******
	// Sync point
	Device.Statistic->RenderDUMP_Wait_S.Begin();
	if (1)
	{
		CTimer T;
		T.Start();
		BOOL result = FALSE;
		HRESULT hr = S_FALSE;
		while ((hr = q_sync_point[q_sync_count]->GetData(&result, sizeof(result), D3DGETDATA_FLUSH)) == S_FALSE)
		{
			if (!SwitchToThread())
				Sleep(ps_r_thread_wait_sleep);
			if (T.GetElapsed_ms() > 500)
			{
				result = FALSE;
				break;
			}
		}
	}
	Device.Statistic->RenderDUMP_Wait_S.End();
	q_sync_count = (q_sync_count + 1) % HW.Caps.iGPUNum;
	CHK_DX(q_sync_point[q_sync_count]->Issue(D3DISSUE_END));

	Target->clear_gbuffer();

	//******* Z-prefill calc - DEFERRER RENDERER
	if (ps_r_ls_flags.test(RFLAG_Z_PREPASS))
	{
		r_pmask(true, false); // enable priority "0"

		set_Recorder(NULL);

		phase = PHASE_DEPTH_PREPASS;

		render_main(Device.mFullTransform, false);

		RCache.set_ColorWriteEnable(FALSE);
		RCache.set_ZWriteEnable(TRUE);

		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL));

		Target->enable_anisotropy_filtering();

		r_dsgraph_render_graph(0);

		if (Details)
			Details->Render();

		r_dsgraph_render_hud();

		r_dsgraph_render_lods(true, true);

		Target->disable_anisotropy_filtering();

		RCache.set_ColorWriteEnable(TRUE);
		RCache.set_ZWriteEnable(FALSE);
	}

	//******* Main calc - DEFERRER RENDERER
	// Main calc
	r_pmask(true, false, true); // enable priority "0",+ capture wmarks

	if (bSUN)
		set_Recorder(&main_coarse_structure);
	else
		set_Recorder(NULL);

	phase = PHASE_NORMAL;
	render_main(Device.mFullTransform, true);
	set_Recorder(NULL);
	r_pmask(true, false); // disable priority "1"

	//******* Main render :: PART-0	-- first
	// level, SPLIT
	Device.Statistic->RenderCALC_GBuffer.Begin();
	Target->enable_anisotropy_filtering();

	Target->create_gbuffer();

	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL));

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	r_dsgraph_render_graph(0);

	if (Details)
	{
		Details->UpdateVisibleM();
		Details->Render();
	}

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	Target->disable_anisotropy_filtering();
	Device.Statistic->RenderCALC_GBuffer.End();

	//******* Occlusion testing of volume-limited light-sources
	Target->phase_occq();

	LP_normal.clear();
	LP_pending.clear();

	{
		// perform tests
		u32 count = 0;
		light_Package& LP = Lights.package;

		// stats
		stats.l_shadowed = LP.v_shadowed.size();
		stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
		stats.l_total = stats.l_shadowed + stats.l_unshadowed;

		// perform tests
		count = _max(count, LP.v_point.size());
		count = _max(count, LP.v_spot.size());
		count = _max(count, LP.v_shadowed.size());

		for (u32 it = 0; it < count; it++)
		{
			if (it < LP.v_point.size())
			{
				light* L = LP.v_point[it];
				L->vis_prepare();
				if (L->vis.pending)
					LP_pending.v_point.push_back(L);
				else
					LP_normal.v_point.push_back(L);
			}
			if (it < LP.v_spot.size())
			{
				light* L = LP.v_spot[it];
				L->vis_prepare();
				if (L->vis.pending)
					LP_pending.v_spot.push_back(L);
				else
					LP_normal.v_spot.push_back(L);
			}
			if (it < LP.v_shadowed.size())
			{
				light* L = LP.v_shadowed[it];
				L->vis_prepare();
				if (L->vis.pending)
					LP_pending.v_shadowed.push_back(L);
				else
					LP_normal.v_shadowed.push_back(L);
			}
		}
	}

	LP_normal.sort();
	LP_pending.sort();

	//******* Main render :: PART-1 (second)
	// level
	PortalTraverser.fade_render();

	Target->enable_anisotropy_filtering();

	Target->create_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL));

	RCache.set_ZWriteEnable(FALSE);

	r_dsgraph_render_hud();

	r_dsgraph_render_lods(true, true);

	RCache.set_ZWriteEnable(TRUE);

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	Target->disable_anisotropy_filtering();

	// Wall marks
	if (Wallmarks)
	{
		Target->phase_wallmarks();

		//g_r = 0;

		Wallmarks->Render(); // wallmarks has priority as normal geometry
	}

	// Update incremental shadowmap-visibility solver
	{
		u32 it = 0;
		for (it = 0; it < Lights_LastFrame.size(); it++)
		{
			if (0 == Lights_LastFrame[it])
				continue;
			try
			{
				Lights_LastFrame[it]->svis.flushoccq();
			}
			catch (...)
			{
				Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear();
	}

	// Directional light - sun
	if (bSUN)
	{
		Device.Statistic->RenderCALC_SUN.Begin();
		RImplementation.stats.l_visible++;
		render_sun_cascades();
		Target->dwLightMarkerID += 2;
		Device.Statistic->RenderCALC_SUN.End();
	}

	Device.Statistic->RenderCALC_LIGHTS.Begin();
	// Lighting, non dependant on OCCQ
	Target->phase_accumulator();

	render_lights(LP_normal);

	// Lighting, dependant on OCCQ
	render_lights(LP_pending);
	Device.Statistic->RenderCALC_LIGHTS.End();

	HOM.Disable();

	Device.Statistic->RenderCALC_AO.Begin();
	Target->phase_ao();
	Device.Statistic->RenderCALC_AO.End();

	// Postprocess
	Device.Statistic->RenderCALC_POSTPROCESS.Begin();

	Target->phase_autoexposure_pipeline_start();

	Target->phase_combine();

	// Generic0 -> Generic1
	Target->phase_antialiasing();

	Target->phase_create_distortion_mask();

	if (g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_PP();

	Target->phase_distortion();

	if (ps_r_postprocess_flags.test(RFLAG_BLOOM))
		Target->phase_bloom();

	if (ps_r_postprocess_flags.test(RFLAG_AUTOEXPOSURE))
		Target->phase_autoexposure();

	//if (ps_r_postprocess_flags.test(RFLAG_MBLUR))
	//	Target->motion_blur_phase_save_frame();

	// Generic1 -> Generic0 -> Generic1
	if (ps_r_postprocess_flags.test(RFLAG_DOF))
		Target->phase_depth_of_field();

	// Generic1 -> Generic0 -> Generic1
	//if (ps_r_postprocess_flags.test(RFLAG_BARREL_BLUR))
	//	Target->phase_barrel_blur();

	if (ps_render_flags.test(RFLAG_LENS_FLARES))
		g_pGamePersistent->Environment().RenderFlares();

	// Generic1 -> Generic0
	Target->phase_combine_postprocess();

	// Generic0 -> Generic1
	Target->phase_effectors();

	Device.Statistic->RenderCALC_POSTPROCESS.End();

	// Generic1 -> Generic0
	Target->draw_overlays();

	Target->phase_output_to_screen();

	Target->phase_autoexposure_pipeline_clear();

	VERIFY(0 == mapDistort.size());

	Device.Statistic->RenderCALC.End();
}

