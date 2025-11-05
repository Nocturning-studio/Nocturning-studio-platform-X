#pragma once
#include "stdafx.h"
#include "blender_add_effects.h"

// SSAO
void CBlender_ssao::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("simple_quad", "ssao_perform", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_ssao_prev", tex_rt_SSAO_prev);
		C.r_ColorWriteEnable(true, false, false, false);
		C.Jitter();
		break;

	case 1:
		C.r_Pass("simple_quad", "ssao_filter", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_ssao", tex_rt_Generic);
		C.r_dx10Texture("s_ssao_prev", tex_rt_SSAO_prev);
		C.r_ColorWriteEnable(true, false, false, false);
		break;

	case 2:
		C.r_Pass("simple_quad", "hbao_plus_prepare_normal", FALSE, FALSE, FALSE);
		C.r_ColorWriteEnable(true, true, true, false);
		break;

	case 3:
		C.r_Pass("simple_quad", "ssao_new_perform", FALSE, FALSE, FALSE);
		C.r_ColorWriteEnable(true, false, false, false);
		C.Jitter();
		break;

	case 4:
		C.r_Pass("simple_quad", "ssao_new_filter", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_ssao", tex_rt_SSAO_temp);
		C.r_dx10Texture("s_ssao_small", tex_rt_SSAO_small);
		C.r_dx10Texture("s_ssao_prev", tex_rt_SSAO_prev);
		C.r_ColorWriteEnable(true, false, false, false);
		break;

	/*case 5:
		C.r_Pass("simple_quad", "ssao_new_blur", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_ssao", tex_rt_SSAO);
		C.r_ColorWriteEnable(true, false, false, false);
		break;

	case 6:
		C.r_Pass("simple_quad", "ssao_new_blur", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_ssao", tex_rt_SSAO_temp);
		C.r_ColorWriteEnable(true, false, false, false);
		break;*/
	}

	C.r_dx10Texture("s_position", tex_rt_Position);
	C.r_dx10Texture("s_position_prev", tex_rt_Position_prev);
	C.r_dx10Texture("s_depth", tex_t_depth);
	C.r_dx10Texture("s_depthms", tex_t_msaa_depth);

	C.r_dx10Sampler("smp_rtlinear");
	C.r_dx10Sampler("smp_linear");
	C.r_dx10Sampler("smp_nofilter");

	C.r_End();
}

// Screen Space Sunshafts
void CBlender_sunshafts::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("simple_quad", "sunshafts_dwm_perform", FALSE, FALSE, FALSE);
		C.r_ColorWriteEnable(true, false, false, false);
		break;

	case 1:
		C.r_Pass("simple_quad", "sunshafts_dwm_denoise_apply", FALSE, FALSE, FALSE, TRUE, D3D_BLEND_ONE, D3D_BLEND_ONE);
		C.r_dx10Texture("s_sunshafts", tex_rt_PPTemp);
		C.r_ColorWriteEnable(true, true, true, false);
		break;
	}

	C.r_dx10Texture("s_position", tex_rt_Position);
	C.r_dx10Texture("s_depth", tex_t_depth);
	C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
	C.r_dx10Sampler("smp_rtlinear");
	C.r_dx10Sampler("smp_nofilter");

	C.r_End();
}

// Screen Space Planar Reflections
void CBlender_SSPR::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_ComputePass("sspr_hash_buffer");
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_depth1", tex_rt_Depth_1);
		C.r_dx10Sampler("smp_nofilter");
		break;
	}

	C.r_End();
}

// Antialiasing
void  CBlender_TAA::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_TAA:
		C.r_Pass("temporal_reprojection", "temporal_reprojection", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_history", tex_rt_Generic_0_feedback);
		C.r_dx10Texture("s_motion", tex_rt_Motion);
		break;

	case SE_TXAA_MOTION:
		C.r_Pass("simple_quad", "txaa_motion_vectors", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		break;

	case SE_TAA_V2_ANTIALIASING:
		C.r_Pass("temporal_reprojection", "temporal_reprojection_hq", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		extern rt_TAA_params_type rt_TAA_params[TAA_FEEDBACK_SIZE];
		for (u32 i = 0; i < TAA_FEEDBACK_SIZE; i++) 
			C.r_dx10Texture(rt_TAA_params[i].shader_name, rt_TAA_params[i].texture_name);
		break;
		
	case SE_TAA_V2_COPY_FRAME:
		C.r_Pass("simple_quad", "temporal_frame_copy", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		break;
	}

	C.r_dx10Sampler("smp_rtlinear");
	C.r_dx10Sampler("smp_nofilter");

	C.r_End();
}

void  CBlender_AA::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_FXAA:
		C.r_Pass("simple_quad", "fxaa_main", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		break;
		
	case SE_FXAA2:
		C.r_Pass("simple_quad", "fxaa_dual", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image_0", tex_rt_Color);
		C.r_dx10Texture("s_image_1", tex_rt_PPTemp);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		break;
		
	case SE_MSAA_RESOLVE:
		C.r_Pass("simple_quad", "msaa_2", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_input", tex_rt_Generic_0_ms);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_image", tex_rt_Color);
		break;

	case SE_MLAA_0:
		C.r_Pass("simple_quad", "mlaa_seperating_lines", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_ColorWriteEnable(true, false, false, false);
		break;

	case SE_MLAA_1:
		C.r_Pass("simple_quad", "mlaa_compute_length", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_edge_mask", tex_rt_MLAA_0);
		C.r_ColorWriteEnable(true, true, false, false);
		break;

	case SE_MLAA_2:
		C.r_Pass("simple_quad", "mlaa_aa_and_blend", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Texture("s_edge_count", tex_rt_MLAA_1);
		break;
	}

	C.r_dx10Sampler("smp_rtlinear");
	C.r_dx10Sampler("smp_nofilter");

	C.r_End();
}

void  CBlender_RSM::PerformRSM(CBlender_Compile& C, LPCSTR ps, LPCSTR vs)
{
	C.r_Pass(ps, vs, FALSE, FALSE, FALSE, TRUE, D3D_BLEND_ONE, D3D_BLEND_ONE);

	C.r_dx10Texture("s_position", tex_rt_Position);
	C.r_dx10Texture("s_depth", tex_t_depth);
	C.r_dx10Texture("s_depthms", tex_t_msaa_depth);

	C.r_dx10Texture("s_smap", tex_rt_smap_depth);
	C.r_dx10Texture("s_smap_minmax", tex_rt_smap_depth_minmax);

	C.r_dx10Texture("s_positionil", tex_rt_Position_IL);
	C.r_dx10Texture("s_normalil", tex_rt_Normal_IL);
	C.r_dx10Texture("s_coloril", tex_rt_Color_IL);

	C.r_dx10Sampler("smp_smap");
	C.r_dx10Sampler("smp_rtlinear");
	C.r_dx10Sampler("smp_nofilter");

	C.r_End();
}

void  CBlender_RSM::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_RSM_SPOT:
		PerformRSM(C, "simple_quad", "accum_spot_gi");
		break;

	case SE_RSM_DIRECT:
		PerformRSM(C, "accum_v", "accum_sun_far_rsm");
		break;

	case SE_RSM_SPATIAL_FILTER:
		C.r_Pass("simple_quad", "rsm_spatial_filter", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_rsm", tex_rt_RSM);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;

	case SE_RSM_TEMPORAL_FILTER:
		C.r_Pass("simple_quad", "rsm_temporal_reprojection", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_rsm", tex_rt_RSM_copy);
		C.r_dx10Texture("s_rsm_prev", tex_rt_RSM_prev);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_position_prev", tex_rt_Position_prev);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}

// SSAA
void CBlender_ssaa::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_CAS:
		C.r_Pass("simple_quad", "cas", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_ColorWriteEnable(true, true, true, false);
		C.r_End();
		break;
		
	case SE_CAS_PORT:
		C.r_Pass("simple_quad", "cas_port", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_SSAA_distort);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_ColorWriteEnable(true, true, true, false);
		C.r_End();
		break;

	case SE_FSR_PORT_EASU:
		C.r_Pass("simple_quad", "fsr_port_easu", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
		
	case SE_FSR_PORT_EASU_16:
		C.r_Pass("simple_quad", "fsr_port_easu_16", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
		
	case SE_FSR_PORT_RCAS:
		C.r_Pass("simple_quad", "fsr_port_rcas", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_SSAA_distort);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;

	case SE_FSR_PORT_RCAS_16:
		C.r_Pass("simple_quad", "fsr_port_rcas_16", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_SSAA_distort);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}
