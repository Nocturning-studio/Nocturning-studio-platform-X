#include "stdafx.h"
#pragma hdrstop

#include "blender_light_direct_cascade.h"

CBlender_accum_direct_cascade::CBlender_accum_direct_cascade()
{
	description.CLS = 0;
}
CBlender_accum_direct_cascade::~CBlender_accum_direct_cascade()
{
}

void CBlender_accum_direct_cascade::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_SUN_NEAR: // near pass - enable Z-test to perform depth-clipping
		C.sh_macro("NEAR_CASCADE", "1");
	case SE_SUN_MIDDLE:
		C.sh_macro("MIDDLE_CASCADE", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
		C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer
		C.r_Sampler("s_lmap", r2_sunmask);
		C.r_Sampler_clf("s_smap", r2_RT_smap_depth);
		gbuffer(C);
		jitter(C);
		C.r_End();
		break;
	case SE_SUN_FAR: // far pass, only stencil clipping performed
		C.sh_macro("USE_SMOOTH_FADING", "1");
		C.sh_macro("FAR_CASCADE", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
		C.r_Sampler("s_lmap", r2_sunmask);
		C.r_Sampler_clf("s_smap", r2_RT_smap_depth);
		gbuffer(C);
		jitter(C);
		{
			u32 s = C.i_Sampler("s_smap");
			C.i_Address(s, D3DTADDRESS_BORDER);
			C.i_BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
		}
		C.r_End();
		break;
	}
}
