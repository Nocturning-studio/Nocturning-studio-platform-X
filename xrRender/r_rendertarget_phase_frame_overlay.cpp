///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::draw_overlays()
{
	OPTICK_EVENT("CRenderTarget::draw_overlays");

	int GridEnabled = 0;
	int CinemaBordersEnabled = 0;
	int WatermarkEnabled = 0;

	if (ps_r_overlay_flags.test(RFLAG_PHOTO_GRID))
		GridEnabled = 1;

	if (ps_r_overlay_flags.test(RFLAG_CINEMA_BORDERS))
		CinemaBordersEnabled = 1;

	if (ps_r_overlay_flags.test(RFLAG_WATERMARK))
		WatermarkEnabled = 1;

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_vertex_buffer(w, h, Offset);

	// Set pass
	RCache.set_Element(s_frame_overlay->E[0]);

	// Set constants
	RCache.set_c("enabled_overlays", (float)GridEnabled, (float)CinemaBordersEnabled, (float)WatermarkEnabled, 0);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}
///////////////////////////////////////////////////////////////////////////////////