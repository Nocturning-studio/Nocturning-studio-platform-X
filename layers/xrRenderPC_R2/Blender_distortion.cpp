///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_distortion.h"
#include "r2_types.h"
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
		C.r_Pass("null", "postprocess_stage_distortion", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_gbuffer_position", r2_RT_GBuffer_Position);
		C.r_Sampler_rtf("s_gbuffer_normal", r2_RT_GBuffer_Normal);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		C.r_Sampler_clf("s_distort", r2_RT_distortion_mask);
		jitter(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////