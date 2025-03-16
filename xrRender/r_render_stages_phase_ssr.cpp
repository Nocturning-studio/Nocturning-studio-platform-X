///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::calc_screen_space_reflections()
{
	set_Render_Target_Surface(rt_Reflections);
	set_Depth_Buffer(NULL);

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_geometry(w, h, Offset);

	for (u32 i = 0; i < s_reflections->E[0]->passes.size(); i++)
	{
		RenderBackend.set_Element(s_reflections->E[0], i);
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
///////////////////////////////////////////////////////////////////////////////////
