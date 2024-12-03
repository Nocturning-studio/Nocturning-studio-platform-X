///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_barrel_blur.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_barrel_blur::CBlender_barrel_blur()
{
	description.CLS = 0;
}

CBlender_barrel_blur::~CBlender_barrel_blur()
{
}

void CBlender_barrel_blur::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("null", "postprocess_stage_barrrel_blur_vertical_pass", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		C.r_Sampler_clf("s_vignette", "vfx\\vfx_vignette");
		jitter(C);
		C.r_End();
		break;
	case 1:
		C.r_Pass("null", "postprocess_stage_barrrel_blur_horizontal_pass", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_clf("s_vignette", "vfx\\vfx_vignette");
		jitter(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
