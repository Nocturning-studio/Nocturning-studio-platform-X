///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_barrel_blur()
{
	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	set_viewport_geometry(w, h, Offset);

	set_Render_Target_Surface(rt_Generic_0);
	set_Depth_Buffer(NULL);
	RenderBackend.set_Element(s_fog_scattering->E[0]);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	set_Render_Target_Surface(rt_Generic_1);
	set_Depth_Buffer(NULL);
	RenderBackend.set_Element(s_fog_scattering->E[1]);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}
///////////////////////////////////////////////////////////////////////////////////
