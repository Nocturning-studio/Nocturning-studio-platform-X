#pragma once
#include "stdafx.h"
#include "blender_combine.h"

void CBlender_combine::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("simple_quad_tm", "combine_1", FALSE, FALSE, FALSE, TRUE, D3D_BLEND_INV_SRC_ALPHA, D3D_BLEND_SRC_ALPHA);
		C.r_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, 0xff, 0x00);
		C.r_StencilRef(0x01);
		C.r_dx10Texture("s_diffuse", tex_rt_Color);
		C.r_dx10Texture("s_diffuse_ms", tex_rt_Color_ms);
		C.r_dx10Texture("s_accumulator", tex_rt_Accumulator);
		C.r_dx10Texture("s_accumulator_1", tex_rt_Accumulator_1);
		C.r_dx10Texture("s_tonemap", tex_t_LUM_dest);
		C.r_dx10Texture("s_image", tex_rt_Generic);
		C.r_dx10Texture("s_material", tex_t_material);
		C.r_dx10Texture("env_s0", tex_t_envmap_0);
		C.r_dx10Texture("env_s1", tex_t_envmap_1);
		C.r_dx10Texture("s_ssao", tex_rt_PPTemp);
		C.r_dx10Texture("s_ssao_pt", tex_rt_SSAO);
		C.r_dx10Texture("s_rsm", tex_rt_RSM);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_material");
		break;

	case 1:
		C.r_Pass("simple_quad", "combine_2_d", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Generic_0);
		C.r_dx10Texture("s_diffuse_ms", tex_rt_Color_ms);
		C.r_dx10Texture("s_bloom", tex_rt_Bloom_1);
		C.r_dx10Texture("s_distort", tex_rt_Generic_1);
		C.r_dx10Texture("s_distortms", tex_rt_Generic_1_ms);
		C.r_dx10Texture("s_sspr_hash", tex_rt_SSPR);
		C.r_dx10Texture("s_ssao", tex_rt_PPTemp);
		C.r_dx10Texture("s_ssao_pt", tex_rt_SSAO);
		C.r_dx10Texture("s_depth1", tex_rt_Depth_1);
		C.r_dx10Texture("s_rsm", tex_rt_RSM);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.Jitter();
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		break;

	case 2:
		C.r_Pass("simple_quad", "combine_2", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Generic_0);
		C.r_dx10Texture("s_diffuse_ms", tex_rt_Color_ms);
		C.r_dx10Texture("s_bloom", tex_rt_Bloom_1);
		C.r_dx10Texture("s_distort", tex_rt_Generic_1);
		C.r_dx10Texture("s_distortms", tex_rt_Generic_1_ms);
		C.r_dx10Texture("s_sspr_hash", tex_rt_SSPR);
		C.r_dx10Texture("s_ssao", tex_rt_PPTemp);
		C.r_dx10Texture("s_ssao_pt", tex_rt_SSAO);
		C.r_dx10Texture("s_depth1", tex_rt_Depth_1);
		C.r_dx10Texture("s_rsm", tex_rt_RSM);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.Jitter();
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		break;

	case 3:
		C.r_Pass("simple_quad_tm", "combine_volumetric", FALSE, FALSE, FALSE, TRUE, D3D_BLEND_ONE, D3D_BLEND_ONE, 2, FALSE);
		C.r_dx10Texture("s_volumetric", tex_rt_Generic_2);
		C.r_dx10Texture("s_accumulator", tex_rt_Accumulator);
		C.r_dx10Texture("s_accumulator_1", tex_rt_Accumulator_1);
		C.r_dx10Texture("s_tonemap", tex_t_LUM_dest);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_ColorWriteEnable(true, true, true, false);
		break;

	case 4:
		C.r_Pass("simple_quad", "cas", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Color);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_ColorWriteEnable(true, true, true, false);
		C.r_End();
		break;
	}

	C.r_End();
}