///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_fog_scattering()
{
	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_geometry(w, h, Offset);

	set_Render_Target_Surface(rt_Generic_0);
	set_Depth_Buffer(NULL);

	for (int BlurIterations = 0; BlurIterations < 4; BlurIterations++)
	for (u32 i = 0; i < s_fog_scattering->E[0]->passes.size(); i++)
	{
		RenderBackend.set_Element(s_fog_scattering->E[0], i);
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
///////////////////////////////////////////////////////////////////////////////////
