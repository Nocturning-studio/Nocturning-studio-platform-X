///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_frame_overlay : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: Frame overlay";
	}

	CBlender_frame_overlay()
	{
		description.CLS = 0;
	}

	~CBlender_frame_overlay() = default;

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		switch (C.iElement)
		{
		case 0:
			C.begin_Pass("screen_quad", "overlay_stage_apply");
			C.set_Sampler_point("s_image", r_RT_generic1);
			C.set_Sampler_point_wrap("s_watermark", "vfx\\vfx_watermark");
			C.end_Pass();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
