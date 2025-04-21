///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_light_direct_cascade.h"
///////////////////////////////////////////////////////////////////////////////////
void CBlender_accum_direct_cascade::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_SUN_NEAR: // near pass - enable Z-test to perform depth-clipping
		C.sh_macro("NEAR_CASCADE", "1");
	case SE_SUN_MIDDLE:
		C.sh_macro(ps_r_lighting_flags.test(RFLAG_SUN_SHAFTS), "SUN_SHAFTS_ENABLED", "1");
		string32 c_sun_shafts_quality;
		sprintf(c_sun_shafts_quality, "%d", ps_r_sun_shafts_quality);
		C.sh_macro("SUN_SHAFTS_QUALITY", c_sun_shafts_quality);
		C.sh_macro("MIDDLE_CASCADE", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
		C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer
		C.r_Sampler("s_lmap", r_sunmask);
		C.r_Sampler_gaussian("s_smap", r_RT_smap_depth);
		gbuffer(C);
		jitter(C);
		C.r_End();
		break;
	case SE_SUN_FAR: // far pass, only stencil clipping performed
		C.sh_macro("USE_SMOOTH_FADING", "1");
		C.sh_macro("FAR_CASCADE", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
		C.r_Sampler("s_lmap", r_sunmask);
		C.r_Sampler_gaussian("s_smap", r_RT_smap_depth);
		gbuffer(C);
		jitter(C);
		{
			u32 s = C.i_Sampler("s_smap");
			C.i_Address(s, D3DTADDRESS_BORDER);
			C.i_BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
		}
		C.r_End();
		break;
	case SE_SUN_STATIC:
		C.r_Pass("screen_quad", "accumulating_light_stage_direct_static", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_light_accumulator", r_RT_Light_Accumulator);
		gbuffer(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
