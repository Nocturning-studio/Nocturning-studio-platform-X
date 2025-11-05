#include "stdafx.h"
#pragma hdrstop

#include "blender_luminance.h"

void CBlender_luminance::Compile(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	switch (C.iElement)
	{
	case 0:				
		// 256x256 => 64x64
		C.r_Pass("stub_notransform_build", "bloom_luminance_1", false, FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_Bloom_1);
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 1:				
		// 64x64 => 8x8
		C.r_Pass("stub_notransform_filter", "bloom_luminance_2", false, FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_LUM_64);
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 2:				
		// 8x8 => 1x1, blending with old result
		C.r_Pass("stub_notransform_filter", "bloom_luminance_3", false, FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", tex_rt_LUM_8);
		C.r_dx10Texture("s_tonemap", tex_t_LUM_src);
		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_nofilter");
		C.r_End();
		break;
	}
}
