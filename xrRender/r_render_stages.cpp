////////////////////////////////////////////////////////////////////////////////
// Created: 19.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_render_stages.h"
////////////////////////////////////////////////////////////////////////////////
IC bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
	float d1 = _1->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	float d2 = _2->spatial.sphere.P.distance_to_sqr(Device.vCameraPosition);
	return d1 < d2;
}

bool CRender::need_render_sun()
{
	Fcolor sun_color = ((light*)Lights.sun_adapted._get())->color;
	return ps_r_lighting_flags.test(RFLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);
}

bool CRender::is_dynamic_sun_enabled()
{
	return ps_r_lighting_flags.test(RFLAG_SUN);
}

void CRender::check_distort()
{
	if (!(mapDistort.size() == 0))
	{
		Msg("! mapDistort isn't deleted correctly!");
	}
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
			concurrency::parallel_sort(lstRenderables.begin(), lstRenderables.end(), pred_sp_sort);

			// Determine visibility for dynamic part of scene
			set_Object(0);
			u32 uID_LTRACK = 0xffffffff;
			if (active_phase() == PHASE_NORMAL)
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
		PortalTraverser.traverse(pLastSector, ViewBase, Device.vCameraPosition, m_ViewProjection,
								 CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE);

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

		if (g_pGameLevel && !(active_phase() == PHASE_SHADOW_DEPTH))
			g_pGameLevel->pHUD->Render_Last(); // HUD
	}
	else
	{
		set_Object(0);

		if (g_pGameLevel && !(active_phase() == PHASE_SHADOW_DEPTH))
			g_pGameLevel->pHUD->Render_Last(); // HUD
	}
}

void CRender::query_wait()
{
	OPTICK_EVENT("CRender::Render - Sync point");

	Device.Statistic->RenderDUMP_Wait_S.Begin();

	CTimer Timer;
	Timer.Start();

	BOOL result = FALSE;
	HRESULT hr = S_FALSE;

	while ((hr = q_sync_point[q_sync_count]->GetData(&result, sizeof(result), D3DGETDATA_FLUSH)) == S_FALSE)
	{
		if (!SwitchToThread())
			Sleep(ps_r_thread_wait_sleep);

		if (Timer.GetElapsed_ms() > 500)
		{
			result = FALSE;
			break;
		}
	}

	Device.Statistic->RenderDUMP_Wait_S.End();

	q_sync_count = (q_sync_count + 1) % HW.Caps.iGPUNum;
	CHK_DX(q_sync_point[q_sync_count]->Issue(D3DISSUE_END));
}

void CRender::update_shadow_map_visibility()
{
	OPTICK_EVENT("CRender::update_shadow_map_visibility()");

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

void CRender::render_depth_prepass()
{
	OPTICK_EVENT("CRender::render_depth_prepass()");

	r_pmask(true, false); // enable priority "0"

	set_Recorder(NULL);

	set_active_phase(PHASE_DEPTH_PREPASS);

	render_main(Device.mFullTransform, false);

	RenderBackend.set_ColorWriteEnable(FALSE);
	RenderBackend.set_ZWriteEnable(TRUE);

	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL));

	RenderBackend.enable_anisotropy_filtering();

	//if (Details)
	//	Details->Render();

	r_dsgraph_render_hud();

	r_dsgraph_render_graph(0);

	//r_dsgraph_render_lods(true, true);

	RenderBackend.disable_anisotropy_filtering();

	RenderBackend.set_ColorWriteEnable(TRUE);
	RenderBackend.set_ZWriteEnable(FALSE);
}

void CRender::render_gbuffer_primary()
{
	OPTICK_EVENT("CRender::render_gbuffer_primary()");

	r_pmask(true, false, true); // enable priority "0",+ capture wmarks

	if (m_need_render_sun)
		set_Recorder(&main_coarse_structure);
	else
		set_Recorder(NULL);

	set_active_phase(PHASE_NORMAL);
	render_main(Device.mFullTransform, true);
	set_Recorder(NULL);
	r_pmask(true, false); // disable priority "1"

	Device.Statistic->RenderCALC_GBuffer.Begin();
	RenderBackend.enable_anisotropy_filtering();

	set_gbuffer();

	if (ps_r_ls_flags.test(RFLAG_Z_PREPASS))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL));

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	r_dsgraph_render_graph(0);

	if (Details)
	{
		// Details->UpdateVisibleM();
		Details->Render();
	}

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	RenderBackend.disable_anisotropy_filtering();
	Device.Statistic->RenderCALC_GBuffer.End();
}

void CRender::render_gbuffer_secondary()
{
	OPTICK_EVENT("CRender::render_gbuffer_secondary()");

	PortalTraverser.fade_render();

	RenderBackend.enable_anisotropy_filtering();

	set_gbuffer();

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL));

	RenderBackend.set_ZWriteEnable(FALSE);

	r_dsgraph_render_lods(true, true);

	set_active_phase(PHASE_HUD);
	r_dsgraph_render_hud();

	if (psDeviceFlags.test(rsWireframe))
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

	RenderBackend.disable_anisotropy_filtering();
}

void CRender::render_stage_forward()
{
	OPTICK_EVENT("CRender::render_stage_forward()");

	VERIFY(0 == mapDistort.size());

	//******* Main render - second order geometry (the one, that doesn't support deffering)
	{
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Generic_1);
		RenderBackend.set_Depth_Buffer(HW.pBaseZB);

		RenderBackend.set_CullMode(CULL_CCW);
		RenderBackend.set_Stencil(FALSE);
		RenderBackend.set_ColorWriteEnable();

		// if (g_pGamePersistent)
		//	g_pGamePersistent->Environment().RenderClouds();

		// level
		r_pmask(false, true); // enable priority "1"
		set_active_phase(PHASE_NORMAL);
		render_main(Device.mFullTransform, false); //

		RenderBackend.enable_anisotropy_filtering();

		if (psDeviceFlags.test(rsWireframe))
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));

		r_dsgraph_render_graph(1); // normal level, secondary priority
		r_dsgraph_render_sorted(); // strict-sorted geoms

		if (psDeviceFlags.test(rsWireframe))
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));

		RenderBackend.disable_anisotropy_filtering();

		g_pGamePersistent->Environment().RenderThunderbolt();
		g_pGamePersistent->Environment().RenderRain();
	}
}

void CRender::render_hom()
{
	OPTICK_EVENT("CRender::render_hom");

	ViewBase.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	View = 0;

	if (!ps_render_flags.test(RFLAG_EXP_MT_CALC))
	{
		HOM.Enable();
		HOM.Render(ViewBase);
	}
}

void CRender::render_stage_occlusion_culling()
{
	OPTICK_EVENT("CRender::OcclusionCulling");

	phase_occq();

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
}

void CRender::render_sun()
{
	OPTICK_EVENT("CRender::render_sun");

	Device.Statistic->RenderCALC_SUN.Begin();

	RenderImplementation.stats.l_visible++;
	render_sun_cascades();
	dwLightMarkerID += 2;

	Device.Statistic->RenderCALC_SUN.End();
}

void CRender::render_lights()
{
	OPTICK_EVENT("CRender::render_lights()");

	Device.Statistic->RenderCALC_LIGHTS.Begin();

	//******* Occlusion testing of volume-limited light-sources
	render_stage_occlusion_culling();

	// Update incremental shadowmap-visibility solver
	update_shadow_map_visibility();

	// Lighting, non dependant on OCCQ
	set_light_accumulator();

	render_lights(LP_normal);

	// Lighting, dependant on OCCQ
	render_lights(LP_pending);

	Device.Statistic->RenderCALC_LIGHTS.End();
}

void CRender::combine_scene()
{
	precombine_scene();

	render_screen_space_reflections();

	render_skybox();

	combine_scene_lighting();

	render_stage_forward();

	combine_sun_shafts();
}

void CRender::render_postprocess()
{
	OPTICK_EVENT("CRender::render_postprocess");

	Device.Statistic->RenderCALC_POSTPROCESS.Begin();

	create_distortion_mask();

	render_distortion();

	render_bloom();

	// Generic1 -> Generic0 -> Generic1
	if (ps_r_postprocess_flags.test(RFLAG_ANTI_ALIASING))
		render_antialiasing();

	// Generic1 -> Generic0 -> Generic1
	if (ps_r_postprocess_flags.test(RFLAG_DOF))
		render_depth_of_field();

	if (ps_render_flags.test(RFLAG_LENS_FLARES))
		g_pGamePersistent->Environment().RenderFlares();

	// Generic1 -> Generic0
	combine_additional_postprocess();

	// Generic0 -> Generic0
	//if (ps_r_postprocess_flags.test(RFLAG_AUTOEXPOSURE))
	//	render_autoexposure();

	//Generic_0 -> Generic_0
	if (g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		render_effectors_pass_night_vision();

	//Radiation
	render_effectors_pass_generate_radiation_noise();

	//"Postprocess" params and colormapping (Generic_1 -> Generic_0)
	render_effectors_pass_combine();

	// Ceneric0 -> Generic0
	if (ps_r_postprocess_flags.test(RFLAG_MBLUR))
		render_motion_blur();

	//Generic_0 -> Generic_1
	render_effectors_pass_resolve_gamma();

	//Generic_1 -> Generic_0
	render_effectors_pass_lut();

	// Ceneric0 -> Generic0
	if (ps_r_color_blind_mode)
		render_effectors_pass_color_blind_filter();

	// Generic0 -> Generic1
	render_screen_overlays();

	if (g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_PP();

	Device.Statistic->RenderCALC_POSTPROCESS.End();
}
////////////////////////////////////////////////////////////////////////////////
