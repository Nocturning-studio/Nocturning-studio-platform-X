#include "stdafx.h"
#pragma hdrstop

#include "blender_frame_overlay.h"
#include "r2_types.h"

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
	/* case 0:
		C.r_Pass("null", "overlay_stage_photo_grid", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		C.r_Sampler("s_photo_grid", "vfx\\vfx_photo_grid", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_End();
		break;
	case 1:
		C.r_Pass("null", "overlay_stage_cinema_borders", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		C.r_Sampler("s_cinema_borders", "vfx\\vfx_cinema_borders", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_End();
		break;
	case 2:
		C.r_Pass("null", "overlay_stage_watermark", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		C.r_Sampler("s_watermark", "vfx\\vfx_watermark", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_End();
		break;*/
	}
}
