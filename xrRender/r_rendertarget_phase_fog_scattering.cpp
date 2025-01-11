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
	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_vertex_buffer(w, h, Offset);

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);

	for (int BlurIterations = 0; BlurIterations < 4; BlurIterations++)
	for (u32 i = 0; i < s_fog_scattering->E[0]->passes.size(); i++)
	{
		RCache.set_Element(s_fog_scattering->E[0], i);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
///////////////////////////////////////////////////////////////////////////////////
