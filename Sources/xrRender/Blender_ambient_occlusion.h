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
			C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao", FALSE, FALSE, FALSE);
			C.r_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.r_End();
			break;
		case SE_AO_MXAO:
			C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_mxao", FALSE, FALSE, FALSE);
			C.r_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.r_End();
			break;
		case SE_AO_HBAO_PLUS:
			C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_hbao_plus", FALSE, FALSE, FALSE);
			C.r_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.r_End();
			break;
		case SE_AO_SSAO_PATH_TRACE:
			C.r_Pass("screen_quad", "ambient_occlusion_stage_pass_ssao_pt", FALSE, FALSE, FALSE);
			C.r_Sampler_point("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			jitter(C);
			C.r_End();
			break;
		case SE_AO_DENOISE:
			C.r_Pass("screen_quad", "ambient_occlusion_blurring_stage_pass_bilinear_filter", FALSE, FALSE, FALSE);
			C.r_Sampler("s_ao", r_RT_ao);
			C.r_End();
			break;
		}
	}

	CBlender_ambient_occlusion() = default;
	virtual ~CBlender_ambient_occlusion() = default;
};
///////////////////////////////////////////////////////////////////////////////////
