#include "stdafx.h"
#pragma hdrstop

#include "blender_frame_overlay.h"
#include "r_types.h"

CBlender_frame_overlay::CBlender_frame_overlay()
{
	description.CLS = 0;
}

CBlender_frame_overlay::~CBlender_frame_overlay()
{
}

void CBlender_frame_overlay::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("null", "overlay_stage_apply", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_Sampler_tex("s_photo_grid", "vfx\\vfx_photo_grid");
		C.r_Sampler_tex("s_watermark", "vfx\\vfx_watermark");
		C.r_Sampler_tex("s_cinema_borders", "vfx\\vfx_cinema_borders");
		C.r_End();
		break;
	}
}
