///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_depth_of_field.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_depth_of_field::CBlender_depth_of_field()
{
	description.CLS = 0;
}

CBlender_depth_of_field::~CBlender_depth_of_field()
{
}

void CBlender_depth_of_field::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("null", "postprocess_stage_depth_of_field_pass_poisson_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;
	case 1:
		C.r_Pass("null", "postprocess_stage_depth_of_field_pass_vertical_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;

	case 2:
		C.r_Pass("null", "postprocess_stage_depth_of_field_pass_horizontal_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		gbuffer(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////