///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_accum_spot : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: accumulate spot light";
	}

	~CBlender_accum_spot() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_L_FILL: // masking
			C.begin_Pass("null", "copy", false, FALSE, FALSE);
			C.set_Sampler("s_base", C.L_textures[0]);
			C.end_Pass();
			break;
		case SE_L_UNSHADOWED: // unshadowed
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.set_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
			C.end_Pass();
			break;
		case SE_L_NORMAL: // normal
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAP_XFORM", "1");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.set_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_FULLSIZE: // normal-fullsize
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
			gbuffer(C);
			C.set_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_TRANSLUENT: // shadowed + transluency
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.begin_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
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
