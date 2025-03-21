#include "stdafx.h"

void CRender::output_frame_to_screen()
{
	OPTICK_EVENT("CRender::output_frame_to_screen");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);
	RenderBackend.set_Element(RenderTarget->s_output_to_screen->E[0]);
	RenderTargetBackend->RenderViewportSurface(Device.dwWidth, Device.dwHeight, HW.pBaseRT, HW.pBaseZB);
}