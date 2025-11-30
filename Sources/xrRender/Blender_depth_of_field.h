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

		LPCSTR sh_name = "postprocess_stage_depth_of_field";

		switch (C.iElement)
		{
		case SE_PASS_DOF_PREPARE_BUFFER:
			// 1. Prepare (Calc CoC)
			C.begin_Pass("screen_quad", sh_name, "main", "PrepareBuffer");
			C.set_Sampler_point("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();
			break;

		case SE_PASS_PROCESS_BOKEH_HQ:
			// 2. HQ Bokeh
			C.set_Define("HQ_FILTER", "1");
			C.begin_Pass("screen_quad", sh_name, "main", "Bokeh");
			C.set_Sampler_linear("s_image", r_RT_generic1); // Linear для сглаживания сэмплов
			gbuffer(C);
			C.end_Pass();

			// Второй проход (если нужен, но для дискового боке обычно хватает одного хорошего)
			// В коде рендера у вас вызывается два раза (ping-pong), можно оставить.
			C.begin_Pass("screen_quad", sh_name, "main", "Bokeh");
			C.set_Sampler_linear("s_image", r_RT_generic0);
			gbuffer(C);
			C.end_Pass();
			break;

		case SE_PASS_PROCESS_BOKEH_LQ:
			// 3. LQ Bokeh
			// Макрос HQ_FILTER не задан -> будет LQ
			C.begin_Pass("screen_quad", sh_name, "main", "Bokeh");
			C.set_Sampler_linear("s_image", r_RT_generic1);
			gbuffer(C);
			C.end_Pass();

			C.begin_Pass("screen_quad", sh_name, "main", "Bokeh");
			C.set_Sampler_linear("s_image", r_RT_generic0);
			gbuffer(C);
			C.end_Pass();
			break;
		}
	};

	CBlender_depth_of_field() = default;
	virtual ~CBlender_depth_of_field() = default;
};
///////////////////////////////////////////////////////////////////////////////////
