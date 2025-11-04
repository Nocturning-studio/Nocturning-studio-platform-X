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

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_PASS_DOWNSAMPLE_PART_1:
			// pass 0 - начальное сильное размытие
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling_initial");
			C.set_Sampler_gaussian("s_image", r_RT_generic1);
			C.end_Pass();

			// pass 1
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling_heavy");
			C.set_Sampler_gaussian("s_image", r_RT_autoexposure_64);
			C.end_Pass();

			// pass 2
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling_heavy");
			C.set_Sampler_gaussian("s_image", r_RT_autoexposure_32);
			C.end_Pass();
			break;

		case SE_PASS_DOWNSAMPLE_PART_2:
			// pass 0
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling_heavy");
			C.set_Sampler_gaussian("s_image", r_RT_autoexposure_16);
			C.end_Pass();

			// pass 1
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling_heavy");
			C.set_Sampler_gaussian("s_image", r_RT_autoexposure_8);
			C.end_Pass();

			// pass 2
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_downsampling_final");
			C.set_Sampler_gaussian("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler_gaussian("s_image", r_RT_autoexposure_4);
			C.end_Pass();
			break;

		case SE_PASS_SAVE_LUMINANCE:
			// pass 0
			C.begin_Pass("screen_quad", "simple_image");
			C.set_Sampler_gaussian("s_image", r_RT_autoexposure_luminance);
			C.end_Pass();
			break;

		case SE_PASS_APPLY_EXPOSURE:
			// pass 0
			C.begin_Pass("screen_quad", "postprocess_stage_autoexposure_pass_apply");
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_gaussian("s_luminance_previous", r_RT_autoexposure_luminance_previous);
			C.set_Sampler_gaussian("s_luminance", r_RT_autoexposure_luminance);
			C.set_Sampler_gaussian("s_luminance0", r_RT_autoexposure_64);
			C.set_Sampler_gaussian("s_luminance1", r_RT_autoexposure_32);
			C.set_Sampler_gaussian("s_luminance2", r_RT_autoexposure_16);
			C.set_Sampler_gaussian("s_luminance3", r_RT_autoexposure_8);
			C.set_Sampler_gaussian("s_luminance4", r_RT_autoexposure_4);
			C.end_Pass();
			break;

		case SE_PASS_DUMMY:
			// pass 0
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
