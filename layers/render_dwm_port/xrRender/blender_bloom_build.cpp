#include "stdafx.h"
#include "blender_bloom_build.h"

void CBlender_bloom_build::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		// transfer into bloom-target
		C.r_Pass("stub_notransform_build", "bloom_build", FALSE, FALSE, FALSE, FALSE, D3D_BLEND_SRC_ALPHA, D3D_BLEND_INV_SRC_ALPHA);
		C.r_dx10Texture("s_image", tex_rt_Generic_1);
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 1:
		// X-filter
		C.r_Pass("stub_notransform_filter", "bloom_filter", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_bloom", tex_rt_Bloom_1);
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 2:
		// Y-filter
		C.r_Pass("stub_notransform_filter", "bloom_filter", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_bloom", tex_rt_Bloom_2);
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}