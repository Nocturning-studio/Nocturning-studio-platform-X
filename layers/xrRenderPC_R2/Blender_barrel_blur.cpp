///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_barrel_blur.h"
#include "r2_types.h"
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
		C.r_Pass("null", "postprocess_stage_barrrel_blur", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_GBuffer_Albedo);
		C.r_Sampler_clf("s_vignette", "vfx\\vfx_vignette");
		jitter(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////