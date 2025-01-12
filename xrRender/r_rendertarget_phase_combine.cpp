#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"

void CRenderTarget::phase_combine_postprocess()
{
	OPTICK_EVENT("CRenderTarget::phase_combine_postprocess");

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set Geometry
	set_viewport_vertex_buffer(w, h, Offset);

	// Set pass
	RCache.set_Element(s_combine->E[2]);

	// Set constants
	RCache.set_c("cas_params", ps_cas_contrast, ps_cas_sharpening, 0, 0);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_apply_volumetric()
{
	OPTICK_EVENT("CRenderTarget::phase_apply_volumetric");

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_vertex_buffer(w, h, Offset);

	// Set pass
	RCache.set_Element(s_combine->E[3]);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_combine()
{
	OPTICK_EVENT("CRenderTarget::phase_combine");

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);
	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;

	// Set output texture
	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);

	// Fill vertex buffer and set geometry
	set_viewport_vertex_buffer(w, h, Offset);

	// Setup textures
	IDirect3DBaseTexture9* e0 = envdesc->sky_r_textures_env[0].second->surface_get();
	t_envmap_0->surface_set(e0);
	_RELEASE(e0);

	IDirect3DBaseTexture9* e1 = envdesc->sky_r_textures_env[1].second->surface_get();
	t_envmap_1->surface_set(e1);
	_RELEASE(e1);

	// Set shader pass
	RCache.set_Element(s_combine->E[1]);

	// Set shader constants
	const float minamb = 0.001f;
	Fvector4 ambclr = 
	{
		_max(sRgbToLinear(envdesc->ambient.x), minamb), 
		_max(sRgbToLinear(envdesc->ambient.y), minamb),
		_max(sRgbToLinear(envdesc->ambient.z), minamb), 
		ps_r_ao_brightness
	};
	RCache.set_c("ambient_color", ambclr);

	Fvector4 envclr = 
	{
		sRgbToLinear(envdesc->hemi_color.x), 
		sRgbToLinear(envdesc->hemi_color.y),
		sRgbToLinear(envdesc->hemi_color.z), 
		envdesc->weight
	};
	RCache.set_c("env_color", envclr);

	RCache.set_c("debug_mode", (float)ps_r_debug_render, 0, 0, 0);

	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

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
