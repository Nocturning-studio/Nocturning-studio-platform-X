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
		C.r_Pass("null", "postprocess_stage_bloom_build", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_clf("s_image", r2_RT_GBuffer_Albedo);
		C.r_End();
		break;
	case 1: // X-filter
		C.r_Pass("null", "postprocess_stage_bloom_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
		C.r_End();
		break;
	case 2: // Y-filter
		C.r_Pass("null", "postprocess_stage_bloom_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
		C.r_End();
		break;
	case 3: // FF-filter_P0
		C.r_Pass("null", "postprocess_stage_bloom_filter_vertical", FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
		C.r_End();
		break;
	case 4: // FF-filter_P1
		C.r_Pass("null", "postprocess_stage_bloom_filter_horizontal", FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_bloom", r2_RT_bloom2);
		C.r_End();
		break;
	case 5: // Combine with frame
		C.r_Pass("null", "postprocess_stage_bloom_combine", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_GBuffer_Albedo);
		C.r_Sampler_clf("s_bloom", r2_RT_bloom1);
		C.r_End();
		break;
	}
}
