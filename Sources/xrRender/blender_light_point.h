///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_accum_point : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: accumulate point light";
	}

	CBlender_accum_point()
	{
		description.CLS = 0;
	}

	~CBlender_accum_point() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_L_FILL: // fill projective
			C.r_Pass("null", "copy", false, FALSE, FALSE);
			C.r_Sampler("s_base", C.L_textures[0]);
			C.r_End();
			break;
		case SE_L_UNSHADOWED: // unshadowed
			C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.r_Sampler_clf("s_lmap", *C.L_textures[0]);
			C.r_End();
			break;
		case SE_L_NORMAL: // normal
			C.r_Define("USE_SHADOW_MAPPING", "1");
			C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.r_Sampler("s_lmap", C.L_textures[0]);
			C.r_Sampler_gaussian("s_smap", r_RT_smap_depth);
			jitter(C);
			C.r_End();
			break;
		case SE_L_FULLSIZE: // normal-fullsize
			C.r_Define("USE_SHADOW_MAPPING", "1");
			C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.r_Sampler("s_lmap", C.L_textures[0]);
			C.r_Sampler_gaussian("s_smap", r_RT_smap_depth);
			jitter(C);
			C.r_End();
			break;
		case SE_L_TRANSLUENT: // shadowed + transluency
			C.r_Define("USE_SHADOW_MAPPING", "1");
			C.r_Define("USE_LIGHT_MAPPING", "1");
			C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.r_Sampler_clf("s_lmap", r_RT_smap_surf); // diff here
			C.r_Sampler_gaussian("s_smap", r_RT_smap_depth);
			jitter(C);
			C.r_End();
			break;
		}
	}

};
///////////////////////////////////////////////////////////////////////////////////
