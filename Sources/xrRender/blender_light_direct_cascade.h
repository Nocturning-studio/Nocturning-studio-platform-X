///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_accum_direct_cascade : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: accumulate direct light";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		Msg("=== BLENDER START: CBlender_accum_direct_cascade, element %d ===", C.iElement);

		switch (C.iElement)
		{
		case SE_SUN_NEAR: // near pass - enable Z-test to perform depth-clipping
			C.set_Define("NEAR_CASCADE");
		case SE_SUN_MIDDLE:
			C.set_Define("MIDDLE_CASCADE");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
			C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer
			C.set_Sampler("s_lmap", r_sunmask);
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_SUN_FAR: // far pass, only stencil clipping performed
			C.set_Define("USE_SMOOTH_FADING");
			C.set_Define("FAR_CASCADE");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
			C.set_Sampler("s_lmap", r_sunmask);
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			{
				u32 s = C.i_Sampler("s_smap");
				C.i_Address(s, D3DTADDRESS_BORDER);
				C.i_BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
			}
			C.end_Pass();
			break;
		case SE_SUN_VOL_NEAR: // near pass - enable Z-test to perform depth-clipping
			C.set_Define("NEAR_CASCADE");
		case SE_SUN_VOL_MIDDLE:
			C.set_Define("MIDDLE_CASCADE");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct_volumetric", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
			C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer
			C.set_Sampler("s_lmap", r_sunmask);
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_SUN_VOL_FAR: // far pass, only stencil clipping performed
			C.set_Define("USE_SMOOTH_FADING");
			C.set_Define("FAR_CASCADE");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_direct_volumetric", false, TRUE, FALSE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
			C.set_Sampler("s_lmap", r_sunmask);
			C.set_Sampler_point("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			{
				u32 s = C.i_Sampler("s_smap");
				C.i_Address(s, D3DTADDRESS_BORDER);
				C.i_BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
			}
			C.end_Pass();
			break;
		}
	}

	CBlender_accum_direct_cascade() = default;
	virtual ~CBlender_accum_direct_cascade() = default;
};
///////////////////////////////////////////////////////////////////////////////////