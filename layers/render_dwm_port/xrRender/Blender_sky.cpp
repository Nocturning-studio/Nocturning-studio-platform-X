#include "stdafx.h"
#pragma hdrstop

#include "blender_sky.h"

void CBlender_sky::Compile(CBlender_Compile& C)
{
	description.CLS = 0;

	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_SKYBOX:
		C.r_Pass("sky", "sky", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_sky0", "$null");
		C.r_dx10Texture("s_sky1", "$null");
		C.r_dx10Texture("s_tonemap", tex_t_LUM_dest);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		break;

	case SE_CLOUDS:
		C.r_Pass("clouds", "clouds", FALSE, FALSE, FALSE, TRUE, D3D_BLEND_SRC_ALPHA, D3D_BLEND_INV_SRC_ALPHA, 3, TRUE);
		C.r_dx10Texture("s_clouds0", "$null");
		C.r_dx10Texture("s_clouds1", "$null");
		C.r_dx10Texture("s_tonemap", tex_t_LUM_dest);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_base");
		break;

	case SE_SKYBOX1:
		C.r_Pass("planar_sky", "sky", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_sky0", "$null");
		C.r_dx10Texture("s_sky1", "$null");
		C.r_dx10Texture("s_tonemap", tex_t_LUM_dest);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_rtlinear");
		break;

	case SE_CLOUDS1:
		C.r_Pass("planar_clouds", "clouds", FALSE, FALSE, FALSE, TRUE, D3D_BLEND_SRC_ALPHA, D3D_BLEND_INV_SRC_ALPHA, 3, TRUE);
		C.r_dx10Texture("s_clouds0", "$null");
		C.r_dx10Texture("s_clouds1", "$null");
		C.r_dx10Texture("s_tonemap", tex_t_LUM_dest);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_base");
		break;
	}

	C.r_End();
}