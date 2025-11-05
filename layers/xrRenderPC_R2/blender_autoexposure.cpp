#include "stdafx.h"
#pragma hdrstop

#include "Blender_autoexposure.h"

CBlender_autoexposure::CBlender_autoexposure()
{
	description.CLS = 0;
}
CBlender_autoexposure::~CBlender_autoexposure()
{
}

void CBlender_autoexposure::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	/* case 0:
		C.r_Pass("null", "postprocess_stage_autoexposure_pass_downsampling_step_1", false, FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_image", r2_RT_generic0);
		C.r_End();
		break;
	case 1:
		C.r_Pass("null", "postprocess_stage_autoexposure_pass_downsampling_step_2", false, FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_image", r2_RT_autoexposure_t64);
		C.r_End();
		break;
	case 2:
		C.r_Pass("null", "postprocess_stage_autoexposure_pass_downsampling_step_3", false, FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_image", r2_RT_autoexposure_t8);
		C.r_Sampler_clf("s_autoexposure", r2_RT_autoexposure_src);
		C.r_End();
		break;
	case 3:
		C.r_Pass("null", "postprocess_stage_autoexposure_pass_combine", false, FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		C.r_Sampler_clf("s_autoexposure", r2_RT_autoexposure_cur);
		C.r_End();
		break;*/
	}
}
