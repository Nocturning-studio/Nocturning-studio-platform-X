///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "Blender_effectors.h"
///////////////////////////////////////////////////////////////////////////////////
void CBlender_effectors::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_PASS_COMBINE:
		C.r_Pass("null", "postprocess_stage_pass_combine_effectors", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_clf("s_grad0", r_colormap0);
		C.r_Sampler_clf("s_grad1", r_colormap1);
		C.r_Sampler_clf("s_noise0", r_RT_radiation_noise0);
		C.r_Sampler_clf("s_noise1", r_RT_radiation_noise1);
		C.r_Sampler_clf("s_noise2", r_RT_radiation_noise2);
		C.r_End();
		break;
	case SE_PASS_RADIATION:
		C.r_Pass("screen_quad", "postprocess_stage_pass_radiation", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_End();
		break;
	case SE_PASS_RESOLVE_GAMMA:
		C.r_Pass("screen_quad", "postprocess_stage_pass_resolve_gamma", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		C.r_End();
		break;
	case SE_PASS_LUT:
		C.r_Pass("screen_quad", "postprocess_stage_pass_lut", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_clf("s_env_lut0", r_T_LUTs0);
		C.r_Sampler_clf("s_env_lut1", r_T_LUTs1);
		C.r_End();

		C.r_Pass("screen_quad", "simple_image", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		C.r_End();
		break;
	case SE_PASS_COLOR_BLIND_FILTER:
		C.r_Pass("screen_quad", "postprocess_stage_pass_color_blind_filter", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		jitter(C);
		C.r_End();

		C.r_Pass("null", "simple_image", FALSE, FALSE, FALSE, FALSE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////

