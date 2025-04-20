///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_reflections.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_reflections::CBlender_reflections()
{
	description.CLS = 0;
}

CBlender_reflections::~CBlender_reflections()
{
}

void CBlender_reflections::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_RENDER_PASS:
		C.r_Pass("screen_quad", "postprocess_stage_reflections_pass_render", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;
	case SE_BLUR_PASS:
		C.r_Pass("screen_quad", "postprocess_stage_reflections_pass_blur", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_reflections", r_RT_reflections_raw);
		gbuffer(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
