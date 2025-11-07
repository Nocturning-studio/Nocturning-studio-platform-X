///////////////////////////////////////////////////////////////////////////////////
// Created: 19.11.2023
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r2_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_wallmarks()
{
	// Targets
	RCache.set_RT(NULL, 2);
	RCache.set_RT(NULL, 1);
	//u_setrt(rt_GBuffer_Albedo, NULL, NULL, rt_ZB->pRT);
	SetRT(rt_GBuffer_Albedo, 0, 0, 0, rt_ZB->pZRT, false, false);

	// Stencil	- draw only where stencil >= 0x1
	RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RCache.set_CullMode(CULL_CCW);
	RCache.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
}
///////////////////////////////////////////////////////////////////////////////////
