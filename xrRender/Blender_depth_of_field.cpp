///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_depth_of_field.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
void CBlender_depth_of_field::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_PASS_DOF_PREPARE_BUFFER:
		C.r_Pass("screen_quad", "postprocess_stage_depth_of_field_pass_prepare_buffer", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		gbuffer(C);
		C.r_End();
		break;	
	case SE_PASS_PROCESS_BOKEH_HQ:
		C.r_Define("HQ_FILTER", "1");
		C.r_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		gbuffer(C);
		C.r_End();

		C.r_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;
	case SE_PASS_PROCESS_BOKEH_LQ:
		C.r_Define("LQ_FILTER", "1");
		C.r_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		gbuffer(C);
		C.r_End();

		C.r_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;
	case SE_PASS_DOF_DUMMY:
		C.r_Pass("screen_quad", "simple_image", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic1);
		gbuffer(C);
		C.r_End();

		C.r_Pass("screen_quad", "simple_image", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
