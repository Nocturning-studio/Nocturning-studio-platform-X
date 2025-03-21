#include "stdafx.h"
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

void CRender::render_effectors_pass_generate_radiation_noise()
{
	OPTICK_EVENT("CRenderTarget::render_effectors_pass_generate_radiation_noise");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[1]);
	RenderBackend.set_Constant("noise_intesity", RenderTarget->param_radiation_intensity, 1);
	RenderTarget->RenderViewportSurface(RenderTarget->rt_Radiation_Noise0);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[1]);
	RenderBackend.set_Constant("noise_intesity", RenderTarget->param_radiation_intensity, 0.66f);
	RenderTarget->RenderViewportSurface(w * 0.5f, h * 0.5f, RenderTarget->rt_Radiation_Noise1);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[1]);
	RenderBackend.set_Constant("noise_intesity", RenderTarget->param_radiation_intensity, 0.33f);
	RenderTarget->RenderViewportSurface(w * 0.25f, h * 0.25f, RenderTarget->rt_Radiation_Noise2);
}

void CRender::render_effectors_pass_night_vision()
{
	OPTICK_EVENT("CRenderTarget::render_effectors_pass_night_vision");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[2]);

	RenderTarget->RenderViewportSurface(RenderTarget->rt_Generic_0);
}

void CRender::render_effectors_pass_combine()
{
	OPTICK_EVENT("CRenderTarget::render_effectors_pass_combine");

	// combination/postprocess
	RenderTarget->set_Render_Target_Surface(RenderTarget->rt_Generic_1);
	RenderTarget->set_Depth_Buffer(NULL);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[0]);

	int gblend = clampr(iFloor((1 - RenderTarget->param_gray) * 255.f), 0, 255);
	int nblend = clampr(iFloor((1 - RenderTarget->param_noise) * 255.f), 0, 255);
	u32 p_color = subst_alpha(RenderTarget->param_color_base, nblend);
	u32 p_gray = subst_alpha(RenderTarget->param_color_gray, gblend);
	u32 p_brightness = RenderTarget->param_color_add;

	// Draw full-screen quad textured with our scene image
	u32 Offset;
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);

	Fvector2 r0, r1, l0, l1;
	RenderTarget->u_calc_tc_duality_ss(r0, r1, l0, l1);

	// Fill vertex buffer
	float du = ps_pps_u, dv = ps_pps_v;
	TL_2c3uv* pv = (TL_2c3uv*)RenderBackend.Vertex.Lock(4, RenderTarget->g_effectors.stride(), Offset);
	pv->set(du + 0, dv + float(_h), p_color, p_gray, r0.x, r1.y, l0.x, l1.y, 0, 0);
	pv++;
	pv->set(du + 0, dv + 0, p_color, p_gray, r0.x, r0.y, l0.x, l0.y, 0, 0);
	pv++;
	pv->set(du + float(_w), dv + float(_h), p_color, p_gray, r1.x, r1.y, l1.x, l1.y, 0, 0);
	pv++;
	pv->set(du + float(_w), dv + 0, p_color, p_gray, r1.x, r0.y, l1.x, l0.y, 0, 0);
	pv++;
	RenderBackend.Vertex.Unlock(4, RenderTarget->g_effectors.stride());

	// Actual rendering
	RenderBackend.set_Constant(	"c_colormap", 
								RenderTarget->param_color_map_influence,
								RenderTarget->param_color_map_interpolate, 
								0,
								0	);
	RenderBackend.set_Constant(	"c_brightness", 
								color_get_R(p_brightness) / 255.f, 
								color_get_G(p_brightness) / 255.f, 
								color_get_B(p_brightness) / 255.f, 
								RenderTarget->param_noise);

	RenderBackend.set_Geometry(RenderTarget->g_effectors);

	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRender::render_effectors_pass_resolve_gamma()
{
	OPTICK_EVENT("CRenderTarget::render_effectors_pass_resolve_gamma");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
	IDirect3DBaseTexture9* e0 = envdesc->lut_r_textures[0].second->surface_get();
	RenderTarget->t_LUT_0->surface_set(e0);
	_RELEASE(e0);

	IDirect3DBaseTexture9* e1 = envdesc->lut_r_textures[1].second->surface_get();
	RenderTarget->t_LUT_1->surface_set(e1);
	_RELEASE(e1);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[3]);

	RenderTarget->RenderViewportSurface(RenderTarget->rt_Generic_0);
}

void CRender::render_effectors_pass_lut()
{
	OPTICK_EVENT("CRenderTarget::render_effectors_pass_lut");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
	IDirect3DBaseTexture9* e0 = envdesc->lut_r_textures[0].second->surface_get();
	RenderTarget->t_LUT_0->surface_set(e0);
	_RELEASE(e0);

	IDirect3DBaseTexture9* e1 = envdesc->lut_r_textures[1].second->surface_get();
	RenderTarget->t_LUT_1->surface_set(e1);
	_RELEASE(e1);

	RenderBackend.set_Element(RenderTarget->s_effectors->E[4]);

	RenderBackend.set_Constant("c_lut_params", envdesc->weight, 0, 0, 0);

	RenderTarget->RenderViewportSurface(RenderTarget->rt_Generic_1);
}

void CRender::render_effectors()
{
	OPTICK_EVENT("CRenderTarget::render_effectors");

	//Generic_0 -> Generic_1
	if (g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		render_effectors_pass_night_vision();

	//Radiation
	render_effectors_pass_generate_radiation_noise();

	//"Postprocess" params and colormapping (Generic_0 -> Generic_1)
	render_effectors_pass_combine();

	//Generic_1 -> Generic_0
	render_effectors_pass_resolve_gamma();

	//Generic_0 -> Generic_1
	render_effectors_pass_lut();
}
