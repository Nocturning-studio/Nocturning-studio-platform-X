///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r2_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_create_distortion_mask()
{
	//u_setrt(rt_Distortion_Mask, 0, 0, rt_ZB->pRT); // Now RT is a distortion mask
	SetRT(rt_Distortion_Mask, 0, 0, 0, rt_ZB->pZRT, true, false);
	RCache.set_CullMode(CULL_CCW);
	RCache.set_Stencil(FALSE);
	RCache.set_ColorWriteEnable();
	//CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 0, 127), 1.0f, 0L));
	RImplementation.r_dsgraph_render_distort();
}

void CRenderTarget::phase_distortion()
{
	//u_setrt(rt_Generic_0, NULL, NULL, NULL);
	SetRT(rt_Generic_0);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	// Set pass
	RCache.set_Element(s_distortion->E[0]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}