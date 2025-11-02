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
	SE_RENDER_PASS,
	SE_BLUR_PASS
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_reflections : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Reflections";
	}

	virtual BOOL canBeDetailed()
	{
		return FALSE;
	}

	CBlender_reflections()
	{
		description.CLS = 0;
	}

	~CBlender_reflections() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_RENDER_PASS:
			C.r_Pass("screen_quad", "postprocess_stage_reflections_pass_render", FALSE, FALSE, FALSE);
			C.r_Sampler_gaussian("s_image", r_RT_generic0);
			C.r_Sampler_rtf("s_bent_normals", r_RT_Bent_Normals);
			gbuffer(C);
			C.r_End();
			break;
		case SE_BLUR_PASS:
			C.r_Pass("screen_quad", "postprocess_stage_reflections_pass_blur", FALSE, FALSE, FALSE);
			C.r_Sampler_rtf("s_reflections", r_RT_reflections_raw);
			gbuffer(C);
			C.r_End();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
