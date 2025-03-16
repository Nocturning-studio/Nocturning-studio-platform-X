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

	set_Render_Target_Surface(rt_Distortion_Mask);
	set_Depth_Buffer(HW.pBaseZB);

	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 0, 127), 1.0f, 0L));

	RenderImplementation.r_dsgraph_render_distort();
}

void CRenderTarget::phase_distortion()
{
	OPTICK_EVENT("CRenderTarget::phase_distortion");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(s_distortion->E[0]);

	RenderViewportSurface(rt_Generic_1);
}