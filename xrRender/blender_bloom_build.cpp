#include "stdafx.h"
#pragma hdrstop

#include "Blender_bloom_build.h"

CBlender_bloom_build::CBlender_bloom_build()
{
	description.CLS = 0;
}
CBlender_bloom_build::~CBlender_bloom_build()
{
}

void CBlender_bloom_build::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0: // transfer into bloom-target
		C.r_Pass("null", "postprocess_stage_bloom_prepare", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_gaussian("s_image", r_RT_generic0);
		C.r_End();
		break;
	case 1:
		C.r_Pass("null", "postprocess_stage_bloom_filter_vertical", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom", r_RT_bloom1);
		C.r_End();
		break;
	case 2:
		C.r_Pass("null", "postprocess_stage_bloom_filter_horizontal", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom", r_RT_bloom2);
		C.r_End();
		break;
	case 3:
		C.r_Pass("null", "postprocess_stage_bloom_blades_filter_vertical", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades1);
		C.r_End();
		break;
	case 4:
		C.r_Pass("null", "postprocess_stage_bloom_blades_filter_horizontal", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades2);
		C.r_End();
		break;
		/*
	case 5: // Combine with frame
		C.r_Pass("null", "postprocess_stage_bloom_combine", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_clf("s_bloom", r_RT_bloom1);
		C.r_Sampler_clf("s_bloom_blades", r_RT_bloom_blades1);
		C.r_End();
		break;
		*/
	}
}
