#include "stdafx.h"
#pragma hdrstop

#include "blender_light_reflected.h"

CBlender_accum_reflected::CBlender_accum_reflected() { description.CLS = 0; }
CBlender_accum_reflected::~CBlender_accum_reflected() {	}

void CBlender_accum_reflected::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	BOOL		blend = TRUE;
	D3D_BLEND	dest = blend ? D3D_BLEND_ONE : D3D_BLEND_ZERO;

	C.r_Pass("accum_v", "accum_indirect", false, FALSE, FALSE, blend, D3D_BLEND_ONE, dest);
	C.r_dx10Texture("s_diffuse", tex_rt_Color);
	C.r_dx10Texture("s_position", tex_rt_Position);
	C.r_dx10Texture("s_depth", tex_t_depth);
	C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
	C.r_dx10Sampler("smp_nofilter");
	C.Jitter();
	C.r_End();
}
