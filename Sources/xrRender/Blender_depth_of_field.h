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
	SE_PASS_PROCESS_BOKEH_HQ,
	SE_PASS_PROCESS_BOKEH_LQ,
	SE_PASS_DOF_PREPARE_BUFFER,
	SE_PASS_DOF_DUMMY
};
///////////////////////////////////////////////////////////////////////////////////
class CBlender_depth_of_field : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Depth of field";
	}

	virtual void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case SE_PASS_DOF_PREPARE_BUFFER:
			C.begin_Pass("screen_quad", "postprocess_stage_depth_of_field_pass_prepare_buffer", FALSE, FALSE, FALSE);
			C.set_Sampler_point("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_PASS_PROCESS_BOKEH_HQ:
			C.set_Define("HQ_FILTER", "1");
			C.begin_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
			C.set_Sampler_point("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
			C.set_Sampler_point("s_image", r_RT_generic0);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_PASS_PROCESS_BOKEH_LQ:
			C.set_Define("LQ_FILTER", "1");
			C.begin_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
			C.set_Sampler_point("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "postprocess_stage_depth_of_field", FALSE, FALSE, FALSE);
			C.set_Sampler_point("s_image", r_RT_generic0);
			gbuffer(C);
			C.end_Pass();
			break;
		case SE_PASS_DOF_DUMMY:
			C.begin_Pass("screen_quad", "simple_image", FALSE, FALSE, FALSE);
			C.set_Sampler_point("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", "simple_image", FALSE, FALSE, FALSE);
			C.set_Sampler_point("s_image", r_RT_generic0);
			gbuffer(C);
			C.end_Pass();
			break;
		}
	};

	CBlender_depth_of_field() = default;
	virtual ~CBlender_depth_of_field() = default;
};
///////////////////////////////////////////////////////////////////////////////////
