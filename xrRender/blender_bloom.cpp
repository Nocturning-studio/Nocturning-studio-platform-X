///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "Blender_bloom.h"
///////////////////////////////////////////////////////////////////////////////////
void CBlender_bloom::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_PASS_PREPARE:
		C.r_Pass("screen_quad", "postprocess_stage_bloom_prepare", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_gaussian("s_image", r_RT_generic1);
		gbuffer(C);
		C.r_End();
		break;
	case SE_PASS_PROCESS_BLOOM:
		C.r_Pass("screen_quad", "postprocess_stage_bloom_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom", r_RT_bloom1);
		C.r_End();

		C.r_Pass("screen_quad", "postprocess_stage_bloom_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom", r_RT_bloom2);
		C.r_End();
		break;
	case SE_PASS_PROCESS_BLADES:
		C.r_Define("FILTER_STEP", "1");
		C.r_Pass("screen_quad", "postprocess_stage_bloom_blades_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades1);
		C.r_End();

		C.r_Define("FILTER_STEP", "2");
		C.r_Pass("screen_quad", "postprocess_stage_bloom_blades_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades2);
		C.r_End();

		C.r_Define("FILTER_STEP", "3");
		C.r_Pass("screen_quad", "postprocess_stage_bloom_blades_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_gaussian("s_bloom_blades", r_RT_bloom_blades1);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
