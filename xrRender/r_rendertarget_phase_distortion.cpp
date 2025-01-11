///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_create_distortion_mask()
{
	OPTICK_EVENT("CRenderTarget::phase_create_distortion_mask");

	u_setrt(rt_Distortion_Mask, NULL, NULL, NULL, HW.pBaseZB); // Now RT is a distortion mask
	RCache.set_CullMode(CULL_CCW);
	RCache.set_Stencil(FALSE);
	RCache.set_ColorWriteEnable();
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 0, 127), 1.0f, 0L));
	RImplementation.r_dsgraph_render_distort();
}

void CRenderTarget::phase_distortion()
{
	OPTICK_EVENT("CRenderTarget::phase_distortion");

	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_vertex_buffer(w, h, Offset);

	// Set pass
	RCache.set_Element(s_distortion->E[0]);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}