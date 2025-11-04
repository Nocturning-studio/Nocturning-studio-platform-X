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
	SE_AO_MXAO,
	SE_AO_SSAO,
	SE_AO_HBAO_PLUS,
	SE_AO_SSAO_PATH_TRACE,
	SE_AO_GTAO,
	SE_AO_DENOISE,
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_ambient_occlusion : public IBlender
{
  public:
	virtual LPCSTR getComment() { return "INTERNAL: Ambient occlusion"; }

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_AO_SSAO:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_AO_MXAO:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_mxao");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_AO_HBAO_PLUS:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_hbao_plus");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_AO_SSAO_PATH_TRACE:
			C.begin_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao_pt");
			C.set_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.end_Pass();
			break;
		case SE_AO_DENOISE:
			C.begin_Pass("screen_quad", "ambient_occlusion_blurring_stage_pass_bilinear_filter");
			C.set_Sampler("s_ao", r_RT_ao);
			C.end_Pass();
			break;
		}
	}

	CBlender_ambient_occlusion() = default;
	virtual ~CBlender_ambient_occlusion() = default;
};
///////////////////////////////////////////////////////////////////////////////////
