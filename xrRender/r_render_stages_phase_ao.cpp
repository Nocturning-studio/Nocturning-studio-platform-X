///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
#include "Blender_ambient_occlusion.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_ao()
{
	OPTICK_EVENT("CRenderTarget::phase_ao");

	int ao_type = SE_AO_SSAO;

	switch (ps_r_ao_quality)
	{
	case 1:
		ao_type = SE_AO_SSAO;
		break;
	case 2:
		ao_type = SE_AO_HBAO_PLUS;
		break;
	case 3:
		ao_type = SE_AO_GTAO;
		break;
	}

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	for (u32 i = 0; i < s_ambient_occlusion->E[ao_type]->passes.size(); i++)
	{
		RenderBackend.set_Element(s_ambient_occlusion->E[ao_type], i);
		RenderBackend.set_Constant("ao_params", ps_r_ao_radius, ps_r_ao_bias);
		RenderViewportSurface(rt_ao);
	}
}
///////////////////////////////////////////////////////////////////////////////////
