///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_distortion : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Distortion";
	}

	CBlender_distortion()
	{
		description.CLS = 0;
	}

	~CBlender_distortion()
	{
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case 0:
			C.r_Pass("screen_quad", "postprocess_stage_distortion", FALSE, FALSE, FALSE);
			gbuffer(C);
			C.r_Sampler_rtf("s_image", r_RT_generic1);
			C.r_Sampler_clf("s_distort", r_RT_distortion_mask);
			jitter(C);
			C.r_End();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////