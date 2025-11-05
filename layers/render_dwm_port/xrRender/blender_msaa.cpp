#include "stdafx.h"
#include "blender_msaa.h"

void CBlender_msaa_mark_edges::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	C.r_Pass("stub_notransform_2uv", "msaa_mark_edges", FALSE, FALSE, FALSE, FALSE);
	C.PassSET_ZB(FALSE, FALSE, FALSE);
	C.r_dx10Texture("s_position", tex_rt_Position);
	C.r_dx10Texture("s_depth", tex_t_depth);
	C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
	C.r_dx10Sampler("smp_nofilter");
	C.r_End();
}