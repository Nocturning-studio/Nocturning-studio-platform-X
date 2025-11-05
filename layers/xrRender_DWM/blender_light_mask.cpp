#include "stdafx.h"
#pragma hdrstop

#include "blender_light_mask.h"

CBlender_accum_direct_mask::CBlender_accum_direct_mask() { description.CLS = 0; }

void CBlender_accum_direct_mask::Compile(CBlender_Compile& C)
{
	IBlender::Compile		(C);

	switch (C.iElement)
	{
	case SE_MASK_SPOT: // spot or omni-part
		C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
		break;
	case SE_MASK_POINT: // point
		C.r_Pass("accum_mask", "dumb", false, TRUE, FALSE);
		break;
	case SE_MASK_DIRECT: // stencil mask for directional light
		C.r_Pass("stub_notransform_t", "accum_sun_mask", false, FALSE, FALSE, TRUE, D3D_BLEND_ZERO, D3D_BLEND_ONE, TRUE, 1);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Sampler("smp_nofilter");
		break;
	}

	C.r_ColorWriteEnable(false, false, false, false);
	C.r_End();
}