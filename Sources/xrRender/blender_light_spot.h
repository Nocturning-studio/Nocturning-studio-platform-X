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

		CBlender_Compile::PassDesc PassDescription;
		PassDescription.VertexShader = "accumulating_light_stage_volume";
		PassDescription.PixelShader = "accumulating_light_stage_spot";
		PassDescription.EnableAlphaBlend = true;
		PassDescription.BlendSRC = D3DBLEND_ONE;
		PassDescription.BlendDST = D3DBLEND_ONE;

		switch (C.iElement)
		{
		case SE_L_FILL: // masking
			C.begin_Pass("null", "copy", false, FALSE, FALSE);
			C.set_Sampler("s_base", C.L_textures[0]);
			C.end_Pass();
			break;
		case SE_L_UNSHADOWED: // unshadowed
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.begin_Pass(PassDescription);
			C.set_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_L_NORMAL: // normal
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAP_XFORM", "1");
			C.begin_Pass(PassDescription);
			C.set_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_FULLSIZE: // normal-fullsize
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.begin_Pass(PassDescription);
			C.set_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_L_TRANSLUENT: // shadowed + transluency
			C.set_Define("USE_SHADOW_MAPPING", "1");
			C.set_Define("USE_LIGHT_MAPPING", "1");
			C.begin_Pass(PassDescription);
			C.set_Sampler_linear("s_lmap", r_RT_smap_surf);
			C.set_Sampler("s_smap", r_RT_smap_depth);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
