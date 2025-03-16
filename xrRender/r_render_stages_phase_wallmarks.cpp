///////////////////////////////////////////////////////////////////////////////////
// Created: 19.11.2023
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_wallmarks()
{
	OPTICK_EVENT("CRenderTarget::phase_wallmarks");

	// Targets
	RenderBackend.setRenderTarget(NULL, 2);
	RenderBackend.setRenderTarget(NULL, 1);
	set_Render_Target_Surface(rt_GBuffer_1);
	set_Depth_Buffer(HW.pBaseZB);

	// Stencil	- draw only where stencil >= 0x1
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
}
///////////////////////////////////////////////////////////////////////////////////
