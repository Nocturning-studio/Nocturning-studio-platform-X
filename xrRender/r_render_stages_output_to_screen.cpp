#include "stdafx.h"
#include "r_rendertarget.h"

void CRenderTarget::phase_output_to_screen()
{
	OPTICK_EVENT("CRenderTarget::phase_output_to_screen");

	set_Render_Target_Surface(Device.dwWidth, Device.dwHeight, HW.pBaseRT);
	set_Depth_Buffer(HW.pBaseZB);

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	set_viewport_geometry(w, h, Offset);

	// Set pass
	RenderBackend.set_Element(s_output_to_screen->E[0]);

	// Draw
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}