#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"

#define STENCIL_CULL 0

void CRenderTarget::phase_combine()
{
	bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	u32 Offset = 0;
	Fvector2 p0, p1;

	//*** exposure-pipeline
	u32 gpu_id = Device.dwFrame % 2;
	{
		t_LUM_src->surface_set(rt_LUM_pool[gpu_id * 2 + 0]->pSurface);
		t_LUM_dest->surface_set(rt_LUM_pool[gpu_id * 2 + 1]->pSurface);
	}

	// low/hi RTs
	u_setrt(rt_Generic_0, rt_Generic_1, 0, rt_ZB->pRT);
	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Draw full-screen quad textured with our scene image
	if (!_menu_pp)
	{
		// draw skybox
		RCache.set_ColorWriteEnable();
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, FALSE));
		g_pGamePersistent->Environment().RenderSky();
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));

		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00); // stencil should be >= 1
		if (RImplementation.o.nvstencil)
		{
			u_stencil_optimize(FALSE);
			RCache.set_ColorWriteEnable();
		}

		// Compute params
		CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
		const float minamb = 0.001f;
		Fvector4 ambclr = {_max(envdesc->ambient.x, minamb), _max(envdesc->ambient.y, minamb),
						   _max(envdesc->ambient.z, minamb), 0};
		// ambclr.mul(ps_r_sun_lumscale_amb);
		Fvector4 envclr = {envdesc->hemi_color.x, envdesc->hemi_color.y, envdesc->hemi_color.z, envdesc->weight};
		// envclr.x *= 2 * ps_r_sun_lumscale_hemi;
		// envclr.y *= 2 * ps_r_sun_lumscale_hemi;
		// envclr.z *= 2 * ps_r_sun_lumscale_hemi;
		Fvector4 sunclr, sundir;

		// sun-params
		{
			light* sun = (light*)RImplementation.Lights.sun_adapted._get();
			Fvector L_dir, L_clr;
			float L_spec;
			L_clr.set(sun->color.r, sun->color.g, sun->color.b);
			L_spec = u_diffuse2s(L_clr);
			Device.mView.transform_dir(L_dir, sun->direction);
			L_dir.normalize();

			sunclr.set(L_clr.x, L_clr.y, L_clr.z, L_spec);
			sundir.set(L_dir.x, L_dir.y, L_dir.z, 0);
		}

		//----------------------
		// Start combine stage
		//----------------------

		// Fill VB
		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		p0.set(.5f / _w, .5f / _h);
		p1.set((_w + .5f) / _w, (_h + .5f) / _h);

		// Fill vertex buffer
		Fvector4* pv = (Fvector4*)RCache.Vertex.Lock(4, g_combine_VP->vb_stride, Offset);
		pv->set(hclip(EPS, _w), hclip(_h + EPS, _h), p0.x, p1.y);
		pv++;
		pv->set(hclip(EPS, _w), hclip(EPS, _h), p0.x, p0.y);
		pv++;
		pv->set(hclip(_w + EPS, _w), hclip(_h + EPS, _h), p1.x, p1.y);
		pv++;
		pv->set(hclip(_w + EPS, _w), hclip(EPS, _h), p1.x, p0.y);
		pv++;
		RCache.Vertex.Unlock(4, g_combine_VP->vb_stride);

		// Setup textures
		IDirect3DBaseTexture9* e0 = _menu_pp ? 0 : envdesc->sky_r_textures_env[0].second->surface_get();
		IDirect3DBaseTexture9* e1 = _menu_pp ? 0 : envdesc->sky_r_textures_env[1].second->surface_get();
		t_envmap_0->surface_set(e0);
		_RELEASE(e0);
		t_envmap_1->surface_set(e1);
		_RELEASE(e1);

		// Draw
		RCache.set_Element(s_combine->E[1]);

		RCache.set_Geometry(g_combine_VP);

		Fvector4 debug_mode = {ps_r_debug_render, 0, 0, 0};
		RCache.set_c("debug_mode", debug_mode);

		RCache.set_c("L_ambient", ambclr);

		RCache.set_c("Ldynamic_color", sunclr);
		RCache.set_c("Ldynamic_dir", sundir);

		RCache.set_c("env_color", envclr);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// Forward rendering
#ifndef MASTER_GOLD
	if (ps_r_debug_render == 0)
#endif
	{
		u_setrt(rt_Generic_0, rt_GBuffer_2, 0, rt_ZB->pRT); // LDR RT

		RCache.set_CullMode(CULL_CCW);
		RCache.set_Stencil(FALSE);
		RCache.set_ColorWriteEnable();
		if (g_pGamePersistent)
			g_pGamePersistent->Environment().RenderClouds();
		RImplementation.render_forward();

		if (g_pGamePersistent)
			g_pGamePersistent->OnRenderPPUI_main(); // PP-UI
	}

	#pragma todo("Deathman to All: Реализовать через текстуру в низком разрешении")
	//if (g_pGamePersistent && (g_pGamePersistent->Environment().CurrentEnv->fog_density > 0.5f))
		//phase_fog_scattering();

	// Perform blooming filter and distortion if needed
	RCache.set_Stencil(FALSE);

	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));

	// Distortion filter
	{
		phase_create_distortion_mask();

		if (g_pGamePersistent)
			g_pGamePersistent->OnRenderPPUI_PP();

		phase_distortion();
	}

	if (!_menu_pp)
	{
		if (!ps_render_flags.test(RFLAG_DISABLE_POSTPROCESS) && (ps_r_debug_render == 0))
		{
			if (ps_r_postprocess_flags.test(RFLAG_ANTI_ALIASING))
				phase_antialiasing();

			if (ps_r_postprocess_flags.test(RFLAG_BLOOM))
				phase_bloom();

			if (ps_r_postprocess_flags.test(RFLAG_AUTOEXPOSURE))
				phase_autoexposure();

			if (ps_r_postprocess_flags.test(RFLAG_CONTRAST_ADAPTIVE_SHARPENING))
				phase_contrast_adaptive_sharpening();

			if (ps_r_postprocess_flags.test(RFLAG_MBLUR))
				phase_motion_blur();

			if (ps_r_postprocess_flags.test(RFLAG_DOF))
				phase_depth_of_field();

			if (ps_r_postprocess_flags.test(RFLAG_BARREL_BLUR))
				phase_barrel_blur();

			if (ps_render_flags.test(RFLAG_LENS_FLARES))
				g_pGamePersistent->Environment().RenderFlares();
		}

		draw_overlays();

		phase_pp();
	}

	//	Re-adapt autoexposure
	RCache.set_Stencil(FALSE);

	//*** exposure-pipeline-clear
	{
		std::swap(rt_LUM_pool[gpu_id * 2 + 0], rt_LUM_pool[gpu_id * 2 + 1]);
		t_LUM_src->surface_set(NULL);
		t_LUM_dest->surface_set(NULL);
	}

#ifdef DEBUG
	RCache.set_CullMode(CULL_CCW);
	static xr_vector<Fplane> saved_dbg_planes;
	if (bDebug)
		saved_dbg_planes = dbg_planes;
	else
		dbg_planes = saved_dbg_planes;
	if (1)
		for (u32 it = 0; it < dbg_planes.size(); it++)
		{
			Fplane& P = dbg_planes[it];
			Fvector zero;
			zero.mul(P.n, P.d);

			Fvector L_dir, L_up = P.n, L_right;
			L_dir.set(0, 0, 1);
			if (_abs(L_up.dotproduct(L_dir)) > .99f)
				L_dir.set(1, 0, 0);
			L_right.crossproduct(L_up, L_dir);
			L_right.normalize();
			L_dir.crossproduct(L_right, L_up);
			L_dir.normalize();

			Fvector p0, p1, p2, p3;
			float sz = 100.f;
			p0.mad(zero, L_right, sz).mad(L_dir, sz);
			p1.mad(zero, L_right, sz).mad(L_dir, -sz);
			p2.mad(zero, L_right, -sz).mad(L_dir, -sz);
			p3.mad(zero, L_right, -sz).mad(L_dir, +sz);
			RCache.dbg_DrawTRI(Fidentity, p0, p1, p2, 0xffffffff);
			RCache.dbg_DrawTRI(Fidentity, p2, p3, p0, 0xffffffff);
		}

	static xr_vector<dbg_line_t> saved_dbg_lines;
	if (bDebug)
		saved_dbg_lines = dbg_lines;
	else
		dbg_lines = saved_dbg_lines;
	if (1)
		for (u32 it = 0; it < dbg_lines.size(); it++)
		{
			RCache.dbg_DrawLINE(Fidentity, dbg_lines[it].P0, dbg_lines[it].P1, dbg_lines[it].color);
		}

	dbg_spheres.clear();
	dbg_lines.clear();
	dbg_planes.clear();
#endif
}
