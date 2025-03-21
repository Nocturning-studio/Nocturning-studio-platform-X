///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::create_distortion_mask()
{
	OPTICK_EVENT("CRender::create_distortion_mask");

	RenderTarget->set_Render_Target_Surface(RenderTarget->rt_Distortion_Mask);
	RenderTarget->set_Depth_Buffer(HW.pBaseZB);

	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 0, 127), 1.0f, 0L));

	RenderImplementation.r_dsgraph_render_distort();
}

void CRender::render_distortion()
{
	OPTICK_EVENT("CRender::render_distortion");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_distortion->E[0]);

	RenderTarget->RenderViewportSurface(RenderTarget->rt_Generic_1);
}