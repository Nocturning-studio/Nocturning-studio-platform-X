///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
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
	SE_PASS_FXAA,
	SE_PASS_NFAA,
	SE_PASS_AA_DUMMY
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_antialiasing : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Antialiasing";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_PASS_NFAA:
			C.r_Pass("screen_quad", "postprocess_stage_antialiasing_pass_nfaa", FALSE, FALSE, FALSE);
			C.r_Sampler_rtf("s_image", r_RT_generic0);
			C.r_End();
			break;
		case SE_PASS_AA_DUMMY:
			C.r_Pass("screen_quad", "simple_image", FALSE, FALSE, FALSE);
			C.r_Sampler_rtf("s_image", r_RT_generic1);
			C.r_End();
			break;
		}
	}

	CBlender_antialiasing() = default;
	virtual ~CBlender_antialiasing() = default;
};
///////////////////////////////////////////////////////////////////////////////////
