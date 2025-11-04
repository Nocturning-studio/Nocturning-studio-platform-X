///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PASS_DOWNSAMPLE_PART_1,
	SE_PASS_DOWNSAMPLE_PART_2,
	SE_PASS_SAVE_LUMINANCE,
	SE_PASS_APPLY_EXPOSURE,
	SE_PASS_DUMMY
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_autoexposure : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: autoexposure estimate";
	}
	virtual BOOL canBeDetailed()
	{
		return FALSE;
	}
	virtual BOOL canBeLMAPped()
	{
		return FALSE;
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_PASS_DOWNSAMPLE_PART_1:
			// pass 0
			C.r_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_generic0);
			C.r_End();

			// pass 1
			C.r_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_autoexposure_t512);
			C.r_End();

			// pass 2
			C.r_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_autoexposure_t256);
			C.r_End();
			break;

		case SE_PASS_DOWNSAMPLE_PART_2:
			// pass 0
			C.r_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_autoexposure_t128);
			C.r_End();

			// pass 1
			C.r_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_autoexposure_t64);
			C.r_End();

			// pass 2
			C.r_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_autoexposure_t8);
			C.r_End();
			break;

		case SE_PASS_SAVE_LUMINANCE:
			// pass 0
			C.r_Pass("screen_quad", "simple_image", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_autoexposure_luminance);
			C.r_End();
			break;

		case SE_PASS_APPLY_EXPOSURE:
			// pass 0
			C.r_Pass("screen_quad", "postprocess_stage_autoexposure_pass_apply", false, FALSE, FALSE, FALSE);
			C.r_Sampler_point("s_image", r_RT_generic0);
			C.r_Sampler_point("s_exposure_previous", r_RT_autoexposure_luminance_previous);
			C.r_Sampler_point("s_exposure", r_RT_autoexposure_luminance);
			C.r_End();
			break;

		case SE_PASS_DUMMY:
			// pass 0
			C.r_Pass("screen_quad", "simple_image", false, FALSE, FALSE, FALSE);
			C.r_Sampler_linear("s_image", r_RT_generic1);
			C.r_End();
			break;
		}
	}

	CBlender_autoexposure() = default;
	~CBlender_autoexposure() = default;
};
///////////////////////////////////////////////////////////////////////////////////
