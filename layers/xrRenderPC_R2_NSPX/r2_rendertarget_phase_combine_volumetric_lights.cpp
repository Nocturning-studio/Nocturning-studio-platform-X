///////////////////////////////////////////////////////////////////////////////////
// Created: 19.11.2023
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r2_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
/* void CRenderTarget::phase_combine_volumetric()
{
	u32 Offset = 0;
	Fvector2 p0, p1;

	// u_setrt(rt_Generic_0,0,0,rt_ZB->pRT );			// LDR RT
	u_setrt(rt_Generic_0, 0, 0, rt_ZB->pRT);
	//	Sets limits to both render targets
	RCache.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	{
		// Compute params
		CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
		const float minamb = 0.001f;
		Fvector4 ambclr = {_max(envdesc->ambient.x * 2, minamb), _max(envdesc->ambient.y * 2, minamb),
						   _max(envdesc->ambient.z * 2, minamb), 0};
		ambclr.mul(ps_r2_sun_lumscale_amb);

		//.		Fvector4	envclr			= { envdesc.sky_color.x*2+EPS,	envdesc.sky_color.y*2+EPS,
		// envdesc.sky_color.z*2+EPS,	envdesc.weight					};
		Fvector4 envclr = {envdesc->hemi_color.x * 2 + EPS, envdesc->hemi_color.y * 2 + EPS,
						   envdesc->hemi_color.z * 2 + EPS, envdesc->weight};
		envclr.x *= 2 * ps_r2_sun_lumscale_hemi;
		envclr.y *= 2 * ps_r2_sun_lumscale_hemi;
		envclr.z *= 2 * ps_r2_sun_lumscale_hemi;
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

		// Fill VB
		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		p0.set(.5f / _w, .5f / _h);
		p1.set((_w + .5f) / _w, (_h + .5f) / _h);

		// Fill vertex buffer
		// Fvector4* pv				= (Fvector4*)	RCache.Vertex.Lock	(4,g_combine_VP->vb_stride,Offset);
		// pv->set						(hclip(EPS,		_w),	hclip(_h+EPS,	_h),	p0.x, p1.y);	pv++;
		// pv->set						(hclip(EPS,		_w),	hclip(EPS,		_h),	p0.x, p0.y);	pv++;
		// pv->set						(hclip(_w+EPS,	_w),	hclip(_h+EPS,	_h),	p1.x, p1.y);	pv++;
		// pv->set						(hclip(_w+EPS,	_w),	hclip(EPS,		_h),	p1.x, p0.y);	pv++;
		// RCache.Vertex.Unlock		(4,g_combine_VP->vb_stride);

		// Fill VB
		float scale_X = float(Device.dwWidth) / float(TEX_jitter);
		float scale_Y = float(Device.dwHeight) / float(TEX_jitter);

		// Fill vertex buffer
		FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine_VP->vb_stride, Offset);
		pv->set(hclip(EPS, _w), hclip(_h + EPS, _h), p0.x, p1.y, 0, 0, scale_Y);
		pv++;
		pv->set(hclip(EPS, _w), hclip(EPS, _h), p0.x, p0.y, 0, 0, 0);
		pv++;
		pv->set(hclip(_w + EPS, _w), hclip(_h + EPS, _h), p1.x, p1.y, 0, scale_X, scale_Y);
		pv++;
		pv->set(hclip(_w + EPS, _w), hclip(EPS, _h), p1.x, p0.y, 0, scale_X, 0);
		pv++;
		RCache.Vertex.Unlock(4, g_combine_VP->vb_stride);

		// Draw
		RCache.set_Element(s_combine_volumetric->E[0]);
		RCache.set_Geometry(g_combine_VP);

		RCache.set_c("L_ambient", ambclr);

		RCache.set_c("Ldynamic_color", sunclr);
		RCache.set_c("Ldynamic_dir", sundir);

		RCache.set_c("env_color", envclr);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
	RCache.set_ColorWriteEnable();
}*/
///////////////////////////////////////////////////////////////////////////////////
