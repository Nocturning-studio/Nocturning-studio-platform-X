///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_ambient_occlusion.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
void CBlender_ambient_occlusion::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_AO_SSAO:
		C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao", FALSE, FALSE, FALSE);
		gbuffer(C);
		jitter(C);
		C.r_End();
		break;
	case SE_AO_MXAO:
		C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_mxao", FALSE, FALSE, FALSE);
		gbuffer(C);
		jitter(C);
		C.r_End();
		break;
	case SE_AO_HBAO_PLUS:
		C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_hbao_plus", FALSE, FALSE, FALSE);
		gbuffer(C);
		jitter(C);
		C.r_End();
		break;	
	case SE_AO_SSAO_PATH_TRACE:
		C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao_pt", FALSE, FALSE, FALSE);
		gbuffer(C);
		jitter(C);
		C.r_End();
		break;
	case SE_AO_DENOISE:
		C.r_Pass("screen_quad", "ambient_occlusion_blurring_stage_pass_bilinear_filter", FALSE, FALSE, FALSE);
		C.r_Sampler("s_ao", r_RT_ao);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
