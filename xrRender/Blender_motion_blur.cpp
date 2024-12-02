///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_motion_blur.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_motion_blur::CBlender_motion_blur()
{
	description.CLS = 0;
}

CBlender_motion_blur::~CBlender_motion_blur()
{
}

void CBlender_motion_blur::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("null", "postprocess_stage_motion_blur_pass_combine", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		C.r_Sampler_rtf("s_previous_image", r_RT_mblur_saved_frame);
		jitter(C);
		C.r_End();
		break;
	case 1:
		C.r_Pass("null", "postprocess_stage_motion_blur_pass_save_frame", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		jitter(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
