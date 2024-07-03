///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_reflections.h"
#include "r2_types.h"
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
	case 0:
		//C.r_Pass("null", "postprocess_stage_reflections_pass_vertical_filter", FALSE, FALSE, FALSE);
		//C.r_Sampler_rtf("s_reflections", r2_RT_reflections);
		//C.r_Sampler_rtf("s_image", r2_RT_generic0);
		//gbuffer(C);
		//C.r_End();

		//C.r_Pass("null", "postprocess_stage_reflections_pass_horizontal_filter", FALSE, FALSE, FALSE);
		//C.r_Sampler_rtf("s_reflections", r2_RT_reflections);
		//C.r_Sampler_rtf("s_image", r2_RT_generic0);
		//gbuffer(C);
		//C.r_End();

		C.r_Pass("null", "postprocess_stage_reflections_pass_render", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_reflections", r2_RT_reflections);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
