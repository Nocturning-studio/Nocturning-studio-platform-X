///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r2_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
#pragma message(Reminder("Fix antialiasing"))
/* void CRenderTarget::phase_antialiasing()
{
	u32 Offset = 0;

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, 0, 0, 0, p0.x, p1.y);
	pv++;
	pv->set(0, 0, 0, 0, 0, p0.x, p0.y);
	pv++;
	pv->set(w, h, 0, 0, 0, p1.x, p1.y);
	pv++;
	pv->set(w, 0, 0, 0, 0, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	RCache.set_Geometry(g_combine);

	u_setrt(rt_Generic_0, NULL, NULL, NULL);

	switch (ps_r2_aa)
	{
	case 1:
		RCache.set_Element(s_antialiasing->E[0]);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		break;
	case 2:
		RCache.set_Element(s_antialiasing->E[1]);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		break;
	case 3:
		RCache.set_Element(s_antialiasing->E[2]);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		break;
	}
}*/
///////////////////////////////////////////////////////////////////////////////////
