///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_reflections.h"
///////////////////////////////////////////////////////////////////////////////////
bool ReflectionsTexIsCleared = false;
///////////////////////////////////////////////////////////////////////////////////
void CRender::clear_reflections()
{
	ReflectionsTexIsCleared = true;
	RenderBackend.ClearTexture(RenderTarget->rt_Reflections, color_argb(NULL, NULL, NULL, NULL));
}

void CRender::render_reflections()
{
	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	RenderBackend.set_Element(RenderTarget->s_reflections->E[SE_RENDER_PASS]);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Reflections);

	//RenderBackend.set_Element(RenderTarget->s_reflections->E[SE_BLUR_PASS]);
	//RenderBackend.RenderViewportSurface(RenderTarget->rt_Reflections);
}

void CRender::render_screen_space_reflections()
{
	OPTICK_EVENT("CRender::render_screen_space_reflections");

	if (ps_r_postprocess_flags.test(RFLAG_REFLECTIONS) && (ps_r_shading_mode != 0))
	{
		ReflectionsTexIsCleared = false;
		render_reflections();
	}
	else
	{
		if (!ReflectionsTexIsCleared)
			clear_reflections();
	}
}
///////////////////////////////////////////////////////////////////////////////////
