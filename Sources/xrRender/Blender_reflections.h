///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
enum
{
	SE_SSR_GENERATE_MIP_CHAIN_PASS,
	SE_SSR_RENDER_PASS,
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_reflections : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Reflections";
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_SSR_GENERATE_MIP_CHAIN_PASS:
			C.begin_Pass("screen_quad", "simple_image");
			C.set_Sampler("s_image", r_RT_generic0);
			gbuffer(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "postprocess_stage_reflections_pass_generate_mip_chain");
			C.set_Sampler("s_mip_chain", r_RT_backbuffer_mip, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, false);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_SSR_RENDER_PASS:
			C.begin_Pass("screen_quad", "postprocess_stage_reflections_pass_render");
			C.set_Sampler("s_image", r_RT_backbuffer_mip, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, D3DTEXF_GAUSSIANQUAD, false);
			gbuffer(C);
			C.end_Pass();
			break;
		}
	}

	~CBlender_reflections() = default;
};
///////////////////////////////////////////////////////////////////////////////////
