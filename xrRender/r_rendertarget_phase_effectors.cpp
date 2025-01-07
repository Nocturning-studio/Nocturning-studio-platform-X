#include "stdafx.h"
#include "r_rendertarget.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"

void CRenderTarget::u_calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1)
{
	// Calculate ordinaty TCs from blur and SS
	float tw = float(dwWidth);
	float th = float(dwHeight);
	if (dwHeight != Device.dwHeight)
		param_blur = 1.f;
	Fvector2 shift, p0, p1;
	shift.set(.5f / tw, .5f / th);
	shift.mul(param_blur);
	p0.set(.5f / tw, .5f / th).add(shift);
	p1.set((tw + .5f) / tw, (th + .5f) / th).add(shift);

	// Calculate Duality TC
	float shift_u = param_duality_h * .5f;
	float shift_v = param_duality_v * .5f;

	r0.set(p0.x, p0.y);
	r1.set(p1.x - shift_u, p1.y - shift_v);
	l0.set(p0.x + shift_u, p0.y + shift_v);
	l1.set(p1.x, p1.y);
}

struct TL_2c3uv
{
	Fvector4 p;
	u32 color0;
	u32 color1;
	Fvector2 uv[3];
	IC void set(float x, float y, u32 c0, u32 c1, float u0, float v0, float u1, float v1, float u2, float v2)
	{
		p.set(x, y, EPS_S, 1.f);
		color0 = c0;
		color1 = c1;
		uv[0].set(u0, v0);
		uv[1].set(u1, v1);
		uv[2].set(u2, v2);
	}
};

void CRenderTarget::phase_effectors_pass_generate_noise0()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors_pass_generate_noise0");

	u_setrt(rt_Radiation_Noise0, NULL, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth) * 0.25f;
	float h = float(Device.dwHeight) * 0.25f;

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
	RCache.set_Element(s_effectors->E[1]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Set constants
	RCache.set_c("noise_intesity", param_radiation_intensity, 0.33f, 0, 0);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}


void CRenderTarget::phase_effectors_pass_generate_noise1()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors_pass_generate_noise1");

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth) * 0.5f;
	float h = float(Device.dwHeight) * 0.5f;

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
	RCache.set_Element(s_effectors->E[1]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Set constants
	RCache.set_c("noise_intesity", param_radiation_intensity, 0.66f, 0, 0);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_effectors_pass_generate_noise2()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors_pass_generate_noise2");

	u_setrt(rt_Radiation_Noise2, NULL, NULL, NULL, NULL);

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
	RCache.set_Element(s_effectors->E[1]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Set constants
	RCache.set_c("noise_intesity", param_radiation_intensity, 1, 0, 0);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_effectors_pass_night_vision()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors_pass_night_vision");

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);

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
	RCache.set_Element(s_effectors->E[2]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_effectors_pass_combine()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors_pass_combine");

	// combination/postprocess
	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);

	RCache.set_Element(s_effectors->E[0]);

	int gblend = clampr(iFloor((1 - param_gray) * 255.f), 0, 255);
	int nblend = clampr(iFloor((1 - param_noise) * 255.f), 0, 255);
	u32 p_color = subst_alpha(param_color_base, nblend);
	u32 p_gray = subst_alpha(param_color_gray, gblend);
	u32 p_brightness = param_color_add;

	// Draw full-screen quad textured with our scene image
	u32 Offset;
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);

	Fvector2 r0, r1, l0, l1;
	u_calc_tc_duality_ss(r0, r1, l0, l1);

	// Fill vertex buffer
	float du = ps_pps_u, dv = ps_pps_v;
	TL_2c3uv* pv = (TL_2c3uv*)RCache.Vertex.Lock(4, g_effectors.stride(), Offset);
	pv->set(du + 0, dv + float(_h), p_color, p_gray, r0.x, r1.y, l0.x, l1.y, 0, 0);
	pv++;
	pv->set(du + 0, dv + 0, p_color, p_gray, r0.x, r0.y, l0.x, l0.y, 0, 0);
	pv++;
	pv->set(du + float(_w), dv + float(_h), p_color, p_gray, r1.x, r1.y, l1.x, l1.y, 0, 0);
	pv++;
	pv->set(du + float(_w), dv + 0, p_color, p_gray, r1.x, r0.y, l1.x, l0.y, 0, 0);
	pv++;
	RCache.Vertex.Unlock(4, g_effectors.stride());

	// Actual rendering
	RCache.set_c("c_colormap", param_color_map_influence, param_color_map_interpolate, 0, 0);
	RCache.set_c("c_brightness", color_get_R(p_brightness) / 255.f, color_get_G(p_brightness) / 255.f,
				 color_get_B(p_brightness) / 255.f, param_noise);

	RCache.set_Geometry(g_effectors);

	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_effectors_pass_resolve_gamma()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors_pass_resolve_gamma");

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);

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

	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
	IDirect3DBaseTexture9* e0 = envdesc->lut_r_textures[0].second->surface_get();
	t_LUT_0->surface_set(e0);
	_RELEASE(e0);

	IDirect3DBaseTexture9* e1 = envdesc->lut_r_textures[1].second->surface_get();
	t_LUT_1->surface_set(e1);
	_RELEASE(e1);

	// Set pass
	RCache.set_Element(s_effectors->E[3]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_effectors_pass_lut()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors_pass_lut");

	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);

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

	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
	IDirect3DBaseTexture9* e0 = envdesc->lut_r_textures[0].second->surface_get();
	t_LUT_0->surface_set(e0);
	_RELEASE(e0);

	IDirect3DBaseTexture9* e1 = envdesc->lut_r_textures[1].second->surface_get();
	t_LUT_1->surface_set(e1);
	_RELEASE(e1);

	// Set pass
	RCache.set_Element(s_effectors->E[4]);

	RCache.set_c("c_lut_params", envdesc->weight, 0, 0, 0);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_effectors()
{
	OPTICK_EVENT("CRenderTarget::phase_effectors");

	//Generic_0 -> Generic_0!
	if (g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		phase_effectors_pass_night_vision();

	//Radiation
	phase_effectors_pass_generate_noise0();
	phase_effectors_pass_generate_noise1();
	phase_effectors_pass_generate_noise2();

	//"Postprocess" params and colormapping (Generic_0 -> Generic_1)
	phase_effectors_pass_combine();

	//Generic_1 -> Generic_0
	phase_effectors_pass_resolve_gamma();

	//Generic_0 -> Generic_1
	phase_effectors_pass_lut();
}
