///////////////////////////////////////////////////////////////////////////////////
//	Created		: 18.12.2023
//	Author		: Deathman
//  Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_contrast_adaptive_sharpening.h"
#include "r2_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_contrast_adaptive_sharpening::CBlender_contrast_adaptive_sharpening()
{
	description.CLS = 0;
}

CBlender_contrast_adaptive_sharpening::~CBlender_contrast_adaptive_sharpening()
{
}

void CBlender_contrast_adaptive_sharpening::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	/* case 0:
		C.r_Pass("null", "postprocess_stage_contrast_adaptive_sharpening", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		jitter(C);
		C.r_End();*/
	}
}
///////////////////////////////////////////////////////////////////////////////////
