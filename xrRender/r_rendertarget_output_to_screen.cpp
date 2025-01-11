#include "stdafx.h"
#include "r_rendertarget.h"

void CRenderTarget::phase_output_to_screen()
{
	OPTICK_EVENT("CRenderTarget::phase_output_to_screen");

	u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, NULL, HW.pBaseZB);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	set_viewport_vertex_buffer(w, h, Offset);

	// Set pass
	RCache.set_Element(s_output_to_screen->E[0]);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}