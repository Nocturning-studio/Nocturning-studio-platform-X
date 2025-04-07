////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "r_render_pipeline.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::RenderMenu()
{
	OPTICK_EVENT("CRender::RenderMenu");

	u32 FrameStartTime = Device.TimerGlobal.GetElapsed_ms();

	//	Globals
	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	// Main Render
	{
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Generic_0);
		RenderBackend.set_Depth_Buffer(HW.pBaseZB);
		g_pGamePersistent->OnRenderPPUI_main(); // PP-UI
	}

	// Distort
	{
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Distortion_Mask);
		RenderBackend.set_Depth_Buffer(HW.pBaseZB);
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 0, 127), 1.0f, 0L));
		g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI
	}

	// Actual Display
	RenderBackend.set_Render_Target_Surface(Device.dwWidth, Device.dwHeight, HW.pBaseRT);
	RenderBackend.set_Depth_Buffer(HW.pBaseZB);
	RenderBackend.set_Shader(RenderTarget->s_menu);
	RenderBackend.set_Geometry(RenderBackend.g_viewport);

	Fvector2 p0, p1;
	u32 Offset;
	u32 C = color_rgba(255, 255, 255, 255);
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	float d_Z = EPS_S;
	float d_W = 1.f;
	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h);

	FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, RenderBackend.g_viewport->vb_stride, Offset);
	pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RenderBackend.Vertex.Unlock(4, RenderBackend.g_viewport->vb_stride);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	u32 FrameEndTime = Device.TimerGlobal.GetElapsed_ms();
	u32 FrameTime = (FrameEndTime - FrameStartTime);
	u32 DSUpdateDelta = 18.6f;
	if (FrameTime < DSUpdateDelta)
	{
		Sleep(DSUpdateDelta - FrameTime);
	}
}
////////////////////////////////////////////////////////////////////////////////
