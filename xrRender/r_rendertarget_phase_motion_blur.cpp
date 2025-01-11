///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::motion_blur_phase_prepare_dilation_map()
{
	OPTICK_EVENT("CRenderTarget::motion_blur_phase_prepare_dilation_map");

	// (new-camera) -> (world) -> (old_viewproj)
	Fmatrix m_previous, m_current, m_invview;
	m_invview.invert(Device.mView);
	m_previous.mul(RImplementation.m_saved_viewproj, m_invview);
	m_current.set(Device.mProject);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth * 0.05f);
	float h = float(Device.dwHeight * 0.05f);

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

	// Set geometry
	RCache.set_Geometry(g_combine);

	u_setrt(rt_Motion_Blur_Dilation_Map_0, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_motion_blur->E[0]);
	RCache.set_c("m_blur_power", ps_r_mblur, 0, 0, 0);
	RCache.set_c("m_current", m_current);
	RCache.set_c("m_previous", m_previous);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	if(0)//for (int i = 0; i < 1; i++)
	{
		u_setrt(rt_Motion_Blur_Dilation_Map_1, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_motion_blur->E[1]);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Motion_Blur_Dilation_Map_0, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_motion_blur->E[2]);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}

void CRenderTarget::phase_motion_blur_pass_blur()
{
	OPTICK_EVENT("CRenderTarget::phase_motion_blur_pass_blur");

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

	// Set geometry
	RCache.set_Geometry(g_combine);

	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_motion_blur->E[4]);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	u_setrt(rt_Motion_Blur_Saved_Frame, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_motion_blur->E[3]);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::motion_blur_phase_combine()
{
	// calc m-blur matrices
	Fmatrix m_previous, m_current;
	// (new-camera) -> (world) -> (old_viewproj)
	Fmatrix m_invview;
	m_invview.invert(Device.mView);
	m_previous.mul(RImplementation.m_saved_viewproj, m_invview);
	m_current.set(Device.mProject);

	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);

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
	RCache.set_Element(s_motion_blur->E[3]);

	RCache.set_c("m_blur_power", ps_r_mblur, 0, 0, 0);
	RCache.set_c("m_current", m_current);
	RCache.set_c("m_previous", m_previous);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::motion_blur_phase_save_frame()
{
	OPTICK_EVENT("CRenderTarget::motion_blur_phase_save_frame");

	u_setrt(rt_Motion_Blur_Saved_Frame, NULL, NULL, NULL, NULL);

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
	RCache.set_Element(s_motion_blur->E[5]);

	RCache.set_c("m_blur_power", ps_r_mblur, 0, 0, 0);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_motion_blur()
{
	OPTICK_EVENT("CRenderTarget::phase_motion_blur");
	//motion_blur_phase_combine();
	motion_blur_phase_prepare_dilation_map();
	//motion_blur_phase_blur_dilation_map();
	phase_motion_blur_pass_blur();
	//motion_blur_phase_save_frame();
}
///////////////////////////////////////////////////////////////////////////////////
