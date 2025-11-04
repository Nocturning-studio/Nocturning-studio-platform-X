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

	~CBlender_accum_point() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_L_FILL: // fill projective
			C.begin_Pass("null", "copy", false, FALSE, FALSE);
			C.set_Sampler("s_base", C.L_textures[0]);
			C.end_Pass();
			break;
		case SE_L_UNSHADOWED: // unshadowed
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.set_Sampler_linear("s_lmap", *C.L_textures[0]);
			C.end_Pass();
			break;
		case SE_L_NORMAL: // normal
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.set_Sampler("s_lmap", C.L_textures[0]);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_FULLSIZE: // normal-fullsize
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.set_Sampler("s_lmap", C.L_textures[0]);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_TRANSLUENT: // shadowed + transluency
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_point", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.set_Sampler_linear("s_lmap", r_RT_smap_surf); // diff here
			C.set_Sampler("s_smap", r_RT_smap_depth);
			jitter(C);
			C.end_Pass();
			break;
		}
	}

};
///////////////////////////////////////////////////////////////////////////////////
