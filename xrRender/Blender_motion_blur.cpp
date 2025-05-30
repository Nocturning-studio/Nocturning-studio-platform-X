///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_motion_blur.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
void CBlender_motion_blur::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_PASS_PREPARE_DILATION_MAP:
		C.r_Pass("screen_quad", "postprocess_stage_motion_blur_pass_create_dilation_map", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_rtf("s_previous_depth", r_RT_mblur_previous_frame_depth);
		gbuffer(C);
		C.r_End();
		break;
	case SE_PASS_BLUR_DILATION_MAP:
		C.r_Pass("screen_quad", "postprocess_stage_motion_blur_pass_blur_dilation_map_vertical", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_dilation_map", r_RT_mblur_dilation_map_0);
		C.r_End();

		C.r_Pass("screen_quad", "postprocess_stage_motion_blur_pass_blur_dilation_map_vertical", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_dilation_map", r_RT_mblur_dilation_map_1);
		C.r_End();
		break;
	case SE_PASS_BLUR_FRAME:
		C.r_Pass("screen_quad", "postprocess_stage_motion_blur_pass_blur_horizontal", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_rtf("s_dilation_map", r_RT_mblur_dilation_map_0);
		gbuffer(C);
		C.r_End();

		C.r_Pass("screen_quad", "postprocess_stage_motion_blur_pass_blur_vertical", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		C.r_Sampler_rtf("s_dilation_map", r_RT_mblur_dilation_map_0);
		gbuffer(C);
		C.r_End();
		break;
	case SE_PASS_SAVE_DEPTH_BUFFER:
		C.r_Pass("screen_quad", "postprocess_stage_motion_blur_pass_save_depth", FALSE, FALSE, FALSE);
		gbuffer(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
