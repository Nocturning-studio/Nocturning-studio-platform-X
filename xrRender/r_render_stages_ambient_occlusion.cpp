///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_ambient_occlusion.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_ambient_occlusion()
{
	OPTICK_EVENT("CRender::render_ambient_occlusion");

	Device.Statistic->RenderCALC_AO.Begin();

	RenderBackend.set_ColorWriteEnable();
	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	RenderBackend.set_Element(RenderTarget->s_ambient_occlusion->E[SE_AO_SSAO]);
	RenderBackend.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
	RenderBackend.set_Constant("ao_params", ps_r_ao_bias, ps_r_ao_radius);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_ao);

	RenderBackend.set_Element(RenderTarget->s_ambient_occlusion->E[SE_AO_DENOISE]);
	RenderBackend.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_ao);

	Device.Statistic->RenderCALC_AO.End();
}
///////////////////////////////////////////////////////////////////////////////////
