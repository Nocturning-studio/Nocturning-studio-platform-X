#include "stdafx.h"
#include "blender_minmax.h"

void CBlender_createminmax::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	C.r_Pass("stub_notransform_2uv", "shadow_minmax_create", FALSE, FALSE, FALSE, FALSE); 
	C.PassSET_ZB(FALSE, FALSE, FALSE);
	C.r_dx10Texture("s_smap", tex_rt_smap_depth);
	C.r_dx10Sampler("smp_nofilter");
	C.r_End();
}