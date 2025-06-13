///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_antialiasing.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
void CBlender_antialiasing::Compile(CBlender_Compile& C)
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
///////////////////////////////////////////////////////////////////////////////////
