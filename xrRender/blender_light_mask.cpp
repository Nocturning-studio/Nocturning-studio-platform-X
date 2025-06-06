#include "stdafx.h"
#pragma hdrstop

#include "Blender_light_mask.h"

CBlender_accum_direct_mask::CBlender_accum_direct_mask()
{
	description.CLS = 0;
}
CBlender_accum_direct_mask::~CBlender_accum_direct_mask()
{
}

void CBlender_accum_direct_mask::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_MASK_SPOT: // spot or omni-part
		C.r_Pass("accumulating_light_stage_mask", "dumb", false, TRUE, FALSE);
		gbuffer(C);
		C.r_End();
		break;
	case SE_MASK_POINT: // point
		C.r_Pass("accumulating_light_stage_mask", "dumb", false, TRUE, FALSE);
		gbuffer(C);
		C.r_End();
		break;
	case SE_MASK_DIRECT: // stencil mask for directional light
		C.r_Pass("screen_quad", "accumulating_light_stage_sun_mask", false, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE, TRUE, 1);
		gbuffer(C);
		C.r_End();
		break;
	}
}
