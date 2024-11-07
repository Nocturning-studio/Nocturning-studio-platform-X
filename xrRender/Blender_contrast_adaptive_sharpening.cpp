///////////////////////////////////////////////////////////////////////////////////
//	Created		: 18.12.2023
//	Author		: Deathman
//  Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_contrast_adaptive_sharpening.h"
#include "r_types.h"
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
	case 0:
		C.r_Pass("null", "postprocess_stage_contrast_adaptive_sharpening", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_clf("s_autoexposure", r_RT_autoexposure_cur);
		C.r_Sampler_clf("s_bloom", r_RT_bloom1);
		C.r_Sampler_clf("s_bloom_blades", r_RT_bloom_blades1);
		C.r_Sampler_clf("s_light_accumulator", r_RT_Light_Accumulator);
		jitter(C);
		C.r_End();
	}
}
///////////////////////////////////////////////////////////////////////////////////
