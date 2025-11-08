///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN,
	SE_PASS_AUTOEXPOSURE_PREPARE_LUMINANCE,
	SE_PASS_AUTOEXPOSURE_SAVE_LUMINANCE,
	SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE,
	SE_PASS_AUTOEXPOSURE_COPY_RENDERTARGET
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_autoexposure : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: autoexposure estimate";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN:
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_initial");
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.end_Pass();

			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_generate_mip_chain");
			C.set_Sampler("s_mip_chain", r_RT_autoexposure_mip_chain, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, false);
			C.end_Pass();
			break;

		case SE_PASS_AUTOEXPOSURE_PREPARE_LUMINANCE:
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_prepare_luminance");
			C.set_Sampler_gaussian("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler("s_luminance", r_RT_autoexposure_mip_chain, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, false);
			C.end_Pass();
			break;

		case SE_PASS_AUTOEXPOSURE_SAVE_LUMINANCE:
			C.begin_Pass("screen_quad", "simple_image");
			C.set_Sampler_gaussian("s_image", r_RT_autoexposure_luminance);
			C.end_Pass();
			break;

		case SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE:
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_apply");
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_gaussian("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler_gaussian("s_luminance", r_RT_autoexposure_luminance);
			C.end_Pass();
			break;

		case SE_PASS_AUTOEXPOSURE_COPY_RENDERTARGET:
			C.begin_Pass("screen_quad", "simple_image");
			C.set_Sampler_linear("s_image", r_RT_generic0);
			C.end_Pass();
			break;
		}
	}

	CBlender_autoexposure() = default;
	~CBlender_autoexposure() = default;
};
///////////////////////////////////////////////////////////////////////////////////
