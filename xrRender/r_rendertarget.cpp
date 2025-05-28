///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
#include "r_rendertarget.h"
#include "render.h"
#include "..\xrEngine\resourcemanager.h"
#include "blender_ambient_occlusion.h"
#include "blender_bloom.h"
#include "blender_antialiasing.h"
#include "blender_combine.h"
#include "blender_distortion.h"
#include "blender_depth_of_field.h"
#include "blender_reflections.h"
#include "blender_motion_blur.h"
#include "blender_frame_overlay.h"
#include "blender_light_direct_cascade.h"
#include "blender_light_mask.h"
#include "blender_light_occq.h"
#include "blender_light_point.h"
#include "blender_light_spot.h"
#include "blender_autoexposure.h"
#include "blender_effectors.h"
#include "blender_output_to_screen.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::create_textures()
{
	Msg("Creating render target textures");

	// SCREENSHOT
	{
		R_CHK(HW.pDevice->CreateOffscreenPlainSurface(dwWidth, dwHeight, HW.Caps.fTarget, D3DPOOL_SYSTEMMEM, &surf_screenshot_normal, NULL));
		R_CHK(HW.pDevice->CreateTexture(128, 128, 1, NULL, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex_screenshot_gamesave, NULL));
		R_CHK(tex_screenshot_gamesave->GetSurfaceLevel(0, &surf_screenshot_gamesave));
	}

	// G-Buffer
	{
		if (ps_r_shading_flags.test(RFLAG_FLAT_SHADING))
		{
			rt_GBuffer_1.create(r_RT_GBuffer_1, dwWidth, dwHeight, D3DFMT_A8R8G8B8);
			rt_GBuffer_2.create(r_RT_GBuffer_2, dwWidth, dwHeight, D3DFMT_A8R8G8B8);
			rt_GBuffer_3.create(r_RT_GBuffer_3, dwWidth, dwHeight, D3DFMT_R16F);
		}
		else
		{
			rt_GBuffer_1.create(r_RT_GBuffer_1, dwWidth, dwHeight, D3DFMT_A8R8G8B8);
			rt_GBuffer_2.create(r_RT_GBuffer_2, dwWidth, dwHeight, D3DFMT_A8R8G8B8);
			rt_GBuffer_3.create(r_RT_GBuffer_3, dwWidth, dwHeight, D3DFMT_A8R8G8B8);
			rt_GBuffer_4.create(r_RT_GBuffer_4, dwWidth, dwHeight, D3DFMT_A16B16G16R16F);
		}
	}

	rt_Light_Accumulator.create(r_RT_Light_Accumulator, dwWidth, dwHeight, D3DFMT_A16B16G16R16F);
	rt_Shadow_Accumulator.create(r_RT_Shadow_Accumulator, dwWidth, dwHeight, D3DFMT_L8);

	rt_Distortion_Mask.create(r_RT_distortion_mask, dwWidth, dwHeight, D3DFMT_G16R16F);

	rt_Generic_0.create(r_RT_generic0, dwWidth, dwHeight, D3DFMT_A16B16G16R16F);
	rt_Generic_1.create(r_RT_generic1, dwWidth, dwHeight, D3DFMT_A16B16G16R16F);
	
	//rt_Generic_Prev.create(r_RT_generic_prev, dwWidth, dwHeight, D3DFMT_A16B16G16R16F);

	//rt_Motion_Blur_Previous_Frame_Depth.create(r_RT_mblur_previous_frame_depth, dwWidth, dwHeight, D3DFMT_R16F);
	//rt_Motion_Blur_Dilation_Map_0.create(r_RT_mblur_dilation_map_0, u32(dwWidth * 0.5f), u32(dwHeight * 0.5f), D3DFMT_G16R16F);
	//rt_Motion_Blur_Dilation_Map_1.create(r_RT_mblur_dilation_map_1, u32(dwWidth * 0.5f), u32(dwHeight * 0.5f), D3DFMT_G16R16F);

	rt_ReflectionsRaw.create(r_RT_reflections_raw, u32(dwWidth), u32(dwHeight), D3DFMT_A16B16G16R16F);
	rt_Reflections.create(r_RT_reflections, u32(dwWidth), u32(dwHeight), D3DFMT_A16B16G16R16F);

	rt_Radiation_Noise0.create(r_RT_radiation_noise0, dwWidth, dwHeight, D3DFMT_L8);
	rt_Radiation_Noise1.create(r_RT_radiation_noise1, u32(dwWidth * 0.5f), u32(dwHeight * 0.5f), D3DFMT_L8);
	rt_Radiation_Noise2.create(r_RT_radiation_noise2, u32(dwWidth * 0.25f), u32(dwHeight * 0.25f), D3DFMT_L8);

	rt_ao.create(r_RT_ao, dwWidth, dwHeight, D3DFMT_L8);

	t_envmap_0.create(r_T_envs0);
	t_envmap_1.create(r_T_envs1);

	t_LUT_0.create(r_T_LUTs0);
	t_LUT_1.create(r_T_LUTs1);

	// BLOOM
	float BloomResolutionMultiplier = 0.2f;
	u32 w = u32(dwWidth * BloomResolutionMultiplier), h = u32(dwHeight * BloomResolutionMultiplier);
	rt_Bloom_1.create(r_RT_bloom1, w, h, D3DFMT_A16B16G16R16F);
	rt_Bloom_2.create(r_RT_bloom2, w, h, D3DFMT_A16B16G16R16F);
	rt_Bloom_Blades_1.create(r_RT_bloom_blades1, w, h, D3DFMT_A16B16G16R16F);
	rt_Bloom_Blades_2.create(r_RT_bloom_blades2, w, h, D3DFMT_A16B16G16R16F);

	// autoexposure
	rt_LUM_512.create(r_RT_autoexposure_t512, 512, 512, D3DFMT_R16F);
	rt_LUM_256.create(r_RT_autoexposure_t256, 256, 256, D3DFMT_R16F);
	rt_LUM_128.create(r_RT_autoexposure_t128, 128, 128, D3DFMT_R16F);
	rt_LUM_64.create(r_RT_autoexposure_t64, 64, 64, D3DFMT_R16F);
	rt_LUM_8.create(r_RT_autoexposure_t8, 8, 8, D3DFMT_R16F);

	rt_SceneLuminance.create(r_RT_autoexposure_luminance, 8, 8, D3DFMT_R16F);
	rt_SceneLuminancePrevious.create(r_RT_autoexposure_luminance_previous, 8, 8, D3DFMT_R16F);

	t_LUM_src.create(r_RT_autoexposure_src);
	t_LUM_dest.create(r_RT_autoexposure_cur);

	// create pool
	for (u32 it = 0; it < 4; it++)
	{
		string256 name;
		sprintf(name, "%s_%d", r_RT_autoexposure_pool, it);
		rt_LUM_pool[it].create(name, 1, 1, D3DFMT_L8);
		RenderBackend.set_Render_Target_Surface(rt_LUM_pool[it]);
		RenderBackend.set_Depth_Buffer(NULL);
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, 0x7f7f7f7f, 1.0f, 0L));
	}
}

void CRenderTarget::create_blenders()
{
	Msg("Creating blenders");

	b_occq = xr_new<CBlender_light_occq>();
	s_occq.create(b_occq, "r\\occq");

	b_accum_mask = xr_new<CBlender_accum_direct_mask>();
	s_accum_mask.create(b_accum_mask, "r\\accum_mask");

	b_accum_direct_cascade = xr_new<CBlender_accum_direct_cascade>();
	s_accum_direct_cascade.create(b_accum_direct_cascade, "r\\accumulate_sun");

	b_accum_point = xr_new<CBlender_accum_point>();
	s_accum_point.create(b_accum_point, "r\\accum_point_s");

	b_accum_spot = xr_new<CBlender_accum_spot>();
	s_accum_spot.create(b_accum_spot, "r\\accum_spot_s", "lights\\lights_spot01");

	b_effectors = xr_new<CBlender_effectors>();
	s_effectors.create(b_effectors, "postprocess_stage_pass_effectors");

	b_output_to_screen = xr_new<CBlender_output_to_screen>();
	s_output_to_screen.create(b_output_to_screen, "output_to_screen_stage");

	s_menu.create("distort");

	b_ambient_occlusion = xr_new<CBlender_ambient_occlusion>();
	s_ambient_occlusion.create(b_ambient_occlusion, "r\\ambient_occlusion");

	b_bloom = xr_new<CBlender_bloom>();
	s_bloom.create(b_bloom, "r\\bloom");

	b_autoexposure = xr_new<CBlender_autoexposure>();
	s_autoexposure.create(b_autoexposure, "r\\autoexposure");

	b_combine = xr_new<CBlender_combine>();
	s_combine.create(b_combine, "r\\combine");

	b_antialiasing = xr_new<CBlender_antialiasing>();
	s_antialiasing.create(b_antialiasing, "r\\antialiasing");

	b_distortion = xr_new<CBlender_distortion>();
	s_distortion.create(b_distortion, "r\\distortion");

	b_reflections = xr_new<CBlender_reflections>();
	s_reflections.create(b_reflections, "r\\reflections");

	b_dof = xr_new<CBlender_depth_of_field>();
	s_dof.create(b_dof, "r\\dof");

	b_motion_blur = xr_new<CBlender_motion_blur>();
	s_motion_blur.create(b_motion_blur, "r\\motion_blur");

	b_frame_overlay = xr_new<CBlender_frame_overlay>();
	s_frame_overlay.create(b_frame_overlay, "r\\frame_overlay");
}

void CRenderTarget::delete_blenders()
{
	Msg("Deleting blenders");

	xr_delete(b_frame_overlay);
	xr_delete(b_motion_blur);
	xr_delete(b_dof);
	xr_delete(b_reflections);
	xr_delete(b_distortion);
	xr_delete(b_antialiasing);
	xr_delete(b_combine);
	xr_delete(b_autoexposure);
	xr_delete(b_bloom);
	xr_delete(b_ambient_occlusion);
	xr_delete(b_output_to_screen);
	xr_delete(b_effectors);
	xr_delete(b_accum_spot);
	xr_delete(b_accum_point);
	xr_delete(b_accum_direct_cascade);
	xr_delete(b_accum_mask);
	xr_delete(b_occq);
}

void CRenderTarget::initialize_postprocess_params()
{
	param_blur = 0.f;
	param_gray = 0.f;
	param_noise = 0.f;
	param_duality_h = 0.f;
	param_duality_v = 0.f;
	param_noise_fps = 25.f;
	param_noise_scale = 1.f;

	im_noise_time = 1 / 100;
	im_noise_shift_w = 0;
	im_noise_shift_h = 0;

	param_color_base = color_rgba(127, 127, 127, 0);
	param_color_gray = color_rgba(85, 85, 85, 0);
	param_color_add = color_rgba(0, 0, 0, 0);

	param_color_map_influence = 0.0f;
	param_color_map_interpolate = 0.0f;

	param_radiation_intensity = 0.0f;
}

CRenderTarget::CRenderTarget()
{
	Msg("Creating render target class...");

	dwWidth = Device.dwWidth;
	dwHeight = Device.dwHeight;

	Device.Resources->Evict();

	initialize_postprocess_params();

	static CTimer phase_timer;
	phase_timer.Start();
	create_textures();
	Msg("- All textures successfully created");
	phase_timer.Dump();

	phase_timer.Start();
	create_blenders();
	Msg("- All blenders successfully created");
	phase_timer.Dump();

	// DIRECT (spot)
	u32 size = RenderImplementation.o.smapsize;
	rt_smap_depth.create(r_RT_smap_depth, size, size, D3DFMT_D24X8);
	rt_smap_surf.create(r_RT_smap_surf, size, size, (D3DFORMAT)MAKEFOURCC('N', 'U', 'L', 'L'));
	rt_smap_ZB = NULL;

	// POINT
	accum_point_geom_create();
	g_accum_point.create(D3DFVF_XYZ, g_accum_point_vb, g_accum_point_ib);
	accum_omnip_geom_create();
	g_accum_omnipart.create(D3DFVF_XYZ, g_accum_omnip_vb, g_accum_omnip_ib);

	// SPOT
	accum_spot_geom_create();
	g_accum_spot.create(D3DFVF_XYZ, g_accum_spot_vb, g_accum_spot_ib);

	// PP
	g_effectors.create(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX3, RenderBackend.Vertex.Buffer(), RenderBackend.QuadIB);

	g_cuboid.create(FVF::F_L, RenderBackend.Vertex.Buffer(), RenderBackend.Index.Buffer());
}

CRenderTarget::~CRenderTarget()
{
	t_LUM_src->surface_set(NULL);
	t_LUM_dest->surface_set(NULL);

#ifdef DEBUG
	_SHOW_REF("t_envmap_0 - #small", t_envmap_0->pSurface);
	_SHOW_REF("t_envmap_1 - #small", t_envmap_1->pSurface);

	_SHOW_REF("t_LUT_0", t_LUT_0->pSurface);
	_SHOW_REF("t_LUT_1", t_LUT_1->pSurface);
#endif // DEBUG

	t_envmap_0->surface_set(NULL);
	t_envmap_1->surface_set(NULL);
	t_envmap_0.destroy();
	t_envmap_1.destroy();

	t_LUT_0->surface_set(NULL);
	t_LUT_1->surface_set(NULL);
	t_LUT_0.destroy();
	t_LUT_1.destroy();

	_RELEASE(rt_smap_ZB);

	_RELEASE(surf_screenshot_normal);
	_RELEASE(surf_screenshot_gamesave);
	_RELEASE(tex_screenshot_gamesave);

	//
	accum_spot_geom_destroy();
	accum_omnip_geom_destroy();
	accum_point_geom_destroy();

	delete_blenders();
}
///////////////////////////////////////////////////////////////////////////////////
