///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_PASS_PREPARE,
	SE_PASS_PROCESS_BLOOM,
	SE_PASS_PROCESS_BLADES,
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_bloom : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: combine to bloom target";
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
		case SE_PASS_PREPARE:
			C.begin_Pass("screen_quad", "postprocess_stage_bloom_prepare");
			C.set_Sampler_gaussian("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_PASS_PROCESS_BLOOM:
			C.set_Define("USE_HORIZONTAL_FILTER", "1");
			C.begin_Pass("screen_quad", "postprocess_stage_bloom_filter");
			C.set_Sampler_gaussian("s_bloom", r_RT_bloom1);
			C.end_Pass();

			C.begin_Pass("screen_quad", "postprocess_stage_bloom_filter");
			C.set_Sampler_gaussian("s_bloom", r_RT_bloom2);
			C.end_Pass();
			break;
		case SE_PASS_PROCESS_BLADES:
			C.set_Define("FILTER_STEP", "1");
			C.begin_Pass("screen_quad", "postprocess_stage_bloom_blades_filter");
			C.set_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades1);
			C.end_Pass();

			C.set_Define("FILTER_STEP", "2");
			C.begin_Pass("screen_quad", "postprocess_stage_bloom_blades_filter");
			C.set_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades2);
			C.end_Pass();

			C.set_Define("FILTER_STEP", "3");
			C.begin_Pass("screen_quad", "postprocess_stage_bloom_blades_filter");
			C.set_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades1);
			C.end_Pass();
			break;
		}
	}

	CBlender_bloom() = default;
	virtual ~CBlender_bloom() = default;
};
///////////////////////////////////////////////////////////////////////////////////
