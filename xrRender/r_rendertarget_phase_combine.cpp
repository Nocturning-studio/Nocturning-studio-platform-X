#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"

void CRenderTarget::phase_combine_postprocess()
{
	OPTICK_EVENT("CRenderTarget::phase_combine_postprocess");

	u_setrt(rt_Generic_0, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	// Set pass
	RCache.set_Element(s_combine->E[2]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Set constants
	RCache.set_c("cas_params", ps_cas_contrast, ps_cas_sharpening, 0, 0);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_apply_volumetric()
{
	OPTICK_EVENT("CRenderTarget::phase_apply_volumetric");

	u_setrt(rt_Generic_0, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	// Set pass
	RCache.set_Element(s_combine->E[3]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_combine()
{
	OPTICK_EVENT("CRenderTarget::phase_combine");

	u32 Offset = 0;
	Fvector2 p0, p1;

	// low/hi RTs
	u_setrt(rt_Generic_1, 0, 0, HW.pBaseZB);
	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Draw full-screen quad textured with our scene image
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
	Fvector4 ambclr = {_max(sRgbToLinear(envdesc->ambient.x), minamb), _max(sRgbToLinear(envdesc->ambient.y), minamb),
					   _max(sRgbToLinear(envdesc->ambient.z), minamb), ps_r_ao_brightness};
	Fvector4 envclr = {sRgbToLinear(envdesc->hemi_color.x), sRgbToLinear(envdesc->hemi_color.y),
					   sRgbToLinear(envdesc->hemi_color.z), envdesc->weight};

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
	IDirect3DBaseTexture9* e0 = envdesc->sky_r_textures_env[0].second->surface_get();
	t_envmap_0->surface_set(e0);
	_RELEASE(e0);

	IDirect3DBaseTexture9* e1 = envdesc->sky_r_textures_env[1].second->surface_get();
	t_envmap_1->surface_set(e1);
	_RELEASE(e1);

	// Draw
	RCache.set_Element(s_combine->E[1]);

	RCache.set_Geometry(g_combine_VP);

	Fvector4 debug_mode = {(float)ps_r_debug_render, 0, 0, 0};
	RCache.set_c("debug_mode", debug_mode);

	RCache.set_c("ambient_color", ambclr);

	RCache.set_c("env_color", envclr);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	// Forward rendering
#ifndef MASTER_GOLD
	if (ps_r_debug_render == 0)
#endif
	{
		u_setrt(rt_Generic_1, rt_GBuffer_2, NULL, HW.pBaseZB); // LDR RT

		RCache.set_CullMode(CULL_CCW);
		RCache.set_Stencil(FALSE);
		RCache.set_ColorWriteEnable();
		//if (g_pGamePersistent)
		//	g_pGamePersistent->Environment().RenderClouds();
		RImplementation.render_forward();

		if (g_pGamePersistent)
			g_pGamePersistent->OnRenderPPUI_main(); // PP-UI
	}

	phase_apply_volumetric();

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
