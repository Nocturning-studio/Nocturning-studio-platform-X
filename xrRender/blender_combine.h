///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PRECOMBINE_SCENE,
	SE_COMBINE_SCENE,
	SE_COMBINE_POSTPROCESS,
	SE_COMBINE_VOLUMETRIC
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_combine : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: combiner";
	}

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_PRECOMBINE_SCENE:
			C.r_Define("USE_FOR_PRECOMBINE", 1);
			C.r_Define("DISABLE_REFLECTIONS", 1);

			C.r_Pass("screen_quad", "scene_combine_stage", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA);

			C.r_Sampler_rtf("s_light_accumulator", r_RT_Light_Accumulator);
			C.r_Sampler_rtf("s_ao", r_RT_ao);
			C.r_Sampler("env_s0", r_T_envs0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.r_Sampler("env_s1", r_T_envs1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.r_Sampler("sky_s0", r_T_sky0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.r_Sampler("sky_s1", r_T_sky1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.r_Sampler_rtf("s_brdf_lut", "vfx\\vfx_brdf_lut");

			C.r_Sampler_rtf("s_bent_normals", r_RT_Bent_Normals);

			gbuffer(C);
			jitter(C);
			C.r_End();
			break;
		case SE_COMBINE_SCENE:
			C.r_Pass("screen_quad", "scene_combine_stage", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA);

			C.r_Sampler_rtf("s_light_accumulator", r_RT_Light_Accumulator);
			C.r_Sampler_rtf("s_ao", r_RT_ao);
			C.r_Sampler("env_s0", r_T_envs0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.r_Sampler("env_s1", r_T_envs1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.r_Sampler("sky_s0", r_T_sky0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.r_Sampler("sky_s1", r_T_sky1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);

			C.r_Sampler_rtf("s_brdf_lut", "vfx\\vfx_brdf_lut");

			C.r_Sampler_rtf("s_image", r_RT_generic0);

			C.r_Sampler_clf("s_reflections", r_RT_reflections);

			C.r_Sampler_rtf("s_bent_normals", r_RT_Bent_Normals);

			gbuffer(C);
			jitter(C);
			C.r_End();
			break;
		case SE_COMBINE_POSTPROCESS:
			C.r_Pass("screen_quad", "scene_combine_stage_pass_postprocess", FALSE, FALSE, FALSE);

			C.r_Sampler_rtf("s_image", r_RT_generic1);
			C.r_Sampler_clf("s_autoexposure", r_RT_autoexposure_cur);
			C.r_Sampler_clf("s_bloom", r_RT_bloom1);
			C.r_Sampler_clf("s_bloom_blades", r_RT_bloom_blades1);
			C.r_Sampler_rtf("s_light_accumulator", r_RT_Light_Accumulator);

			jitter(C);
			gbuffer(C);
			C.r_End();
			break;
		case SE_COMBINE_VOLUMETRIC:
			C.r_Pass("screen_quad", "scene_combine_stage_apply_volumetric", FALSE, FALSE, FALSE);

			C.r_Sampler_rtf("s_image", r_RT_generic1);
			C.r_Sampler_clf("s_volumetric_accumulator", r_RT_Volumetric_Sun);

			jitter(C);
			gbuffer(C);
			C.r_End();
			break;
		}
	}

	CBlender_combine() = default;
	virtual ~CBlender_combine() = default;
};
///////////////////////////////////////////////////////////////////////////////////
