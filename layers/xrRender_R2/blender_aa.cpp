#include "stdafx.h"
#pragma hdrstop

#include "blender_aa.h"

CBlender_TAA::CBlender_TAA() { description.CLS = 0; }
CBlender_TAA::~CBlender_TAA() {}
void CBlender_TAA::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_TAA_OPT:
		C.r_Pass("temporal_vert", "temporal_reprojection_opt", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_position", r2_RT_P);
		C.r_Sampler_rtf("s_image", r2_RT_albedo);
		C.r_Sampler_rtf("s_taa_0_mask", "$user$taa_0_mask");
		C.r_Sampler_rtf("s_taa_1", "$user$taa_1");
		C.r_Sampler_rtf("s_taa_2", "$user$taa_2");
		C.r_Sampler_rtf("s_taa_3", "$user$taa_3");
		C.r_Sampler_rtf("s_taa_4", "$user$taa_4");
		C.r_Sampler_rtf("s_taa_5", "$user$taa_5");
		C.r_Sampler_rtf("s_taa_6", "$user$taa_6");
		C.r_Sampler_rtf("s_taa_7", "$user$taa_7");
		C.r_Sampler_rtf("s_taa_8", "$user$taa_8");
		C.r_Sampler_rtf("s_taa_9", "$user$taa_9");
		C.r_Sampler_rtf("s_taa_10", "$user$taa_10");
		C.r_Sampler_rtf("s_taa_11", "$user$taa_11");
		C.r_Sampler_rtf("s_taa_12", "$user$taa_12");
		C.r_Sampler_rtf("s_taa_13", "$user$taa_13");
		C.r_Sampler_rtf("s_taa_14", "$user$taa_14");
		C.r_Sampler_rtf("s_taa_15", "$user$taa_15");
		break;

	case SE_TAA_FRAME_COPY_OPT: // Copy frame and mask
		C.r_Pass("temporal_vert", "temporal_frame_copy_opt", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_albedo);
		C.r_Sampler_rtf("s_taa_0_mask", "$user$taa_0_mask");
		break;

	case SE_TAA_IMG_COPY_OPT: // Copy antialiased image
		C.r_Pass("temporal_vert", "temporal_img_copy_opt", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", "$user$taa_temp");
		break;
	}

	C.r_End();
}