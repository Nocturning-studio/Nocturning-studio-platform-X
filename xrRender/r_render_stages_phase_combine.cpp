///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include <blender_combine.h>
///////////////////////////////////////////////////////////////////////////////////
void CRender::combine_additional_postprocess()
{
	OPTICK_EVENT("CRender::combine_additional_postprocess");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_combine->E[SE_COMBINE_POSTPROCESS]);
	RenderBackend.set_Constant("cas_params", ps_cas_contrast, ps_cas_sharpening, 0, 0);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::combine_sun_shafts()
{
	OPTICK_EVENT("CRender::combine_sun_shafts");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_combine->E[SE_COMBINE_VOLUMETRIC]);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_skybox()
{
	OPTICK_EVENT("CRender::render_skybox");

	RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Generic_1);
	RenderBackend.set_Depth_Buffer(NULL);
	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	// Draw full-screen quad textured with our scene image draw skybox
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, FALSE));
	g_pGamePersistent->Environment().RenderSky();
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::precombine_scene()
{
	OPTICK_EVENT("CRender::combine_additional_postprocess");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	float additional_ambient = 0.0f;

	if (g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		additional_ambient = 0.5f;

	RenderBackend.set_Element(RenderTarget->s_combine->E[SE_PRECOMBINE_SCENE]);
	RenderBackend.set_Constant("additional_ambient", additional_ambient);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::combine_scene_lighting()
{
	float additional_ambient = 0.0f;

	if (g_pGamePersistent && g_pGamePersistent->GetNightVisionState())
		additional_ambient = 0.5f;

	CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;

	const float minamb = 0.001f;
	Fvector4 ambclr = {	_max(sRgbToLinear(envdesc->ambient.x), minamb),
						_max(sRgbToLinear(envdesc->ambient.y), minamb),
						_max(sRgbToLinear(envdesc->ambient.z), minamb), 
						ps_r_ao_brightness};

	Fvector4 envclr = {	sRgbToLinear(envdesc->hemi_color.x), 
						sRgbToLinear(envdesc->hemi_color.y),
						sRgbToLinear(envdesc->hemi_color.z), 
						envdesc->weight};

	IDirect3DBaseTexture9* e0 = envdesc->sky_r_textures_env[0].second->surface_get();
	RenderTarget->t_envmap_0->surface_set(e0);
	_RELEASE(e0);
	IDirect3DBaseTexture9* e1 = envdesc->sky_r_textures_env[1].second->surface_get();
	RenderTarget->t_envmap_1->surface_set(e1);
	_RELEASE(e1);

	RenderBackend.set_Element(RenderTarget->s_combine->E[SE_COMBINE_SCENE]);
	RenderBackend.set_Constant("additional_ambient", additional_ambient);
	RenderBackend.set_Constant("debug_mode", ps_r_debug_render);
	RenderBackend.set_Constant("ambient_color", ambclr);
	RenderBackend.set_Constant("env_color", envclr);
	RenderBackend.set_CullMode(CULL_NONE);
	// stencil should be >= 1, we don't touch sky pixels
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1, HW.pBaseZB);

#ifdef DEBUG
	RenderBackend.set_CullMode(CULL_CCW);
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
			RenderBackend.dbg_DrawTRI(Fidentity, p0, p1, p2, 0xffffffff);
			RenderBackend.dbg_DrawTRI(Fidentity, p2, p3, p0, 0xffffffff);
		}

	static xr_vector<dbg_line_t> saved_dbg_lines;
	if (bDebug)
		saved_dbg_lines = dbg_lines;
	else
		dbg_lines = saved_dbg_lines;
	if (1)
		for (u32 it = 0; it < dbg_lines.size(); it++)
		{
			RenderBackend.dbg_DrawLINE(Fidentity, dbg_lines[it].P0, dbg_lines[it].P1, dbg_lines[it].color);
		}

	dbg_spheres.clear();
	dbg_lines.clear();
	dbg_planes.clear();
#endif
}
///////////////////////////////////////////////////////////////////////////////////
