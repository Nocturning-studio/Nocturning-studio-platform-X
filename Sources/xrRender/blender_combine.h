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
			C.set_Define("USE_FOR_PRECOMBINE", 1);
			C.set_Define("DISABLE_REFLECTIONS", 1);

			C.begin_Pass("screen_quad", "scene_combine_stage", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA);

			C.set_Sampler_point("s_light_accumulator", r_RT_Light_Accumulator);
			C.set_Sampler_point("s_ao", r_RT_ao);
			C.set_Sampler("env_s0", r_T_irradiance0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("env_s1", r_T_irradiance1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("sky_s0", r_T_sky0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("sky_s1", r_T_sky1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler_linear("s_brdf_lut", "vfx\\vfx_brdf_lut");

			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);

			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_COMBINE_SCENE:
			C.begin_Pass("screen_quad", "scene_combine_stage", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA);

			C.set_Sampler_point("s_light_accumulator", r_RT_Light_Accumulator);
			C.set_Sampler_point("s_ao", r_RT_ao);
			C.set_Sampler("env_s0", r_T_irradiance0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("env_s1", r_T_irradiance1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("sky_s0", r_T_sky0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
			C.set_Sampler("sky_s1", r_T_sky1, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);

			C.set_Sampler_linear("s_brdf_lut", "vfx\\vfx_brdf_lut");

			C.set_Sampler_point("s_image", r_RT_generic0);

			C.set_Sampler_linear("s_reflections", r_RT_reflections);

			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);

			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_COMBINE_POSTPROCESS:
			C.begin_Pass("screen_quad", "scene_combine_stage_pass_postprocess", FALSE, FALSE, FALSE);

			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_linear("s_autoexposure", r_RT_autoexposure_cur);
			C.set_Sampler_linear("s_bloom", r_RT_bloom1);
			C.set_Sampler_linear("s_bloom_blades", r_RT_bloom_blades1);
			C.set_Sampler_point("s_light_accumulator", r_RT_Light_Accumulator);

			jitter(C);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_COMBINE_VOLUMETRIC:
			C.begin_Pass("screen_quad", "scene_combine_stage_apply_volumetric", FALSE, FALSE, FALSE);

			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_linear("s_volumetric_accumulator", r_RT_Volumetric_Sun);

			jitter(C);
			gbuffer(C);
			C.end_Pass();
			break;
		}
	}

	CBlender_combine() = default;
	virtual ~CBlender_combine() = default;
};
///////////////////////////////////////////////////////////////////////////////////
