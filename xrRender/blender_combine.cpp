#include "stdafx.h"
#pragma hdrstop

#include "Blender_combine.h"

CBlender_combine::CBlender_combine()
{
	description.CLS = 0;
}
CBlender_combine::~CBlender_combine()
{
}

void CBlender_combine::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0: //pre combine
		C.r_Pass("scene_combine_stage", "scene_combine_stage_pass_precombine", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA,
				 D3DBLEND_SRCALPHA); //. MRT-blend?
		gbuffer(C);
		C.r_Sampler_rtf("s_light_accumulator", r_RT_Light_Accumulator);
		C.r_Sampler_rtf("s_ao", r_RT_ao);
		C.r_Sampler("env_s0", r_T_envs0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);
		C.r_Sampler("env_s1", r_T_envs1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);
		C.r_Sampler("sky_s0", r_T_sky0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);
		C.r_Sampler("sky_s1", r_T_sky1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);

		C.r_Sampler_clf("s_brdf_lut", "vfx\\vfx_brdf_lut");

		jitter(C);
		C.r_End();
		break;
	case 1: // combine
		C.r_Pass("scene_combine_stage", "scene_combine_stage", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA); //. MRT-blend?
		gbuffer(C);
		C.r_Sampler_rtf("s_light_accumulator", r_RT_Light_Accumulator);
		C.r_Sampler_gaussian("s_ao", r_RT_ao);
		C.r_Sampler("env_s0", r_T_envs0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);
		C.r_Sampler("env_s1", r_T_envs1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);
		C.r_Sampler("sky_s0", r_T_sky0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);
		C.r_Sampler("sky_s1", r_T_sky1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);

		C.r_Sampler_clf("s_brdf_lut", "vfx\\vfx_brdf_lut");

		C.r_Sampler_rtf("s_image", r_RT_generic0);

		C.r_Sampler_clf("s_reflections", r_RT_reflections);

		C.r_Sampler_clf("s_autoexposure", r_RT_autoexposure_cur);

		jitter(C);
		C.r_End();
		break;
	}
}
