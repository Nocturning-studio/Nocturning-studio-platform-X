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

	// Globals
	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_ColorWriteEnable();

	// Main Render
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0, HW.pBaseZB);
	g_pGamePersistent->OnRenderPPUI_main(); // PP-UI

	// Prepare distortion mask
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Distortion_Mask, HW.pBaseZB);
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 0, 127), 1.0f, 0L));
	g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI

	// Apply distortion
	RenderBackend.set_Shader(RenderTarget->s_menu_distortion);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1, HW.pBaseZB);

	// Resolve gamma and actual display
	RenderBackend.set_Shader(RenderTarget->s_menu_gamma);
	RenderBackend.RenderViewportSurface(Device.dwWidth, Device.dwHeight, HW.pBaseRT, HW.pBaseZB);

	// Fucking frame limiter
	u32 FrameEndTime = Device.TimerGlobal.GetElapsed_ms();
	u32 FrameTime = (FrameEndTime - FrameStartTime);
	u32 UpdateDelta = 13.0f;
	if (FrameTime < UpdateDelta)
		Sleep(UpdateDelta - FrameTime);
}
////////////////////////////////////////////////////////////////////////////////
