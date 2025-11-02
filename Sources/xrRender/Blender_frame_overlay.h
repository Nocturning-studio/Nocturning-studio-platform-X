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
			C.r_Pass("screen_quad", "overlay_stage_apply", FALSE, FALSE, FALSE);
			C.r_Sampler_rtf("s_image", r_RT_generic1);
			C.r_Sampler_tex("s_watermark", "vfx\\vfx_watermark");
			C.r_End();
			break;
		}
	}
};
///////////////////////////////////////////////////////////////////////////////////
