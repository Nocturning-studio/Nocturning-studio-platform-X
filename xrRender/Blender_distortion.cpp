///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_distortion.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_distortion::CBlender_distortion()
{
	description.CLS = 0;
}

CBlender_distortion::~CBlender_distortion()
{
}

void CBlender_distortion::Compile(CBlender_Compile& C)
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
///////////////////////////////////////////////////////////////////////////////////
