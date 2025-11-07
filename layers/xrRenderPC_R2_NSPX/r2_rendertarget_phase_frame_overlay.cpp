///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r2_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
#pragma message(Reminder("Fix overlay"))
/* void CRenderTarget::photo_grid()
{
	u_setrt(rt_Generic_0, NULL, NULL, NULL);

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
	RCache.set_Element(s_frame_overlay->E[0]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::cinema_borders()
{
	u_setrt(rt_Generic_0, NULL, NULL, NULL);

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
	RCache.set_Element(s_frame_overlay->E[1]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::watermark()
{
	u_setrt(rt_Generic_0, NULL, NULL, NULL);

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
	RCache.set_Element(s_frame_overlay->E[2]);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::draw_overlays()
{
	if (ps_r2_overlay_flags.test(R2FLAG_PHOTO_GRID))
		photo_grid();

#ifndef DEMO_BUILD
	if (ps_r2_overlay_flags.test(R2FLAG_CINEMA_BORDERS))
#endif
		cinema_borders();

#ifndef DEMO_BUILD
	if (ps_r2_overlay_flags.test(R2FLAG_WATERMARK))
#endif
		watermark();
}*/
///////////////////////////////////////////////////////////////////////////////////