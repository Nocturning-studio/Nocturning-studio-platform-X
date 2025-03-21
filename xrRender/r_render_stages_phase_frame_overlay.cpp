///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_screen_overlays()
{
	OPTICK_EVENT("CRenderTarget::render_screen_overlays");

	int GridEnabled = 0;
	int CinemaBordersEnabled = 0;
	int WatermarkEnabled = 0;

	if (ps_r_overlay_flags.test(RFLAG_PHOTO_GRID))
		GridEnabled = 1;

	if (ps_r_overlay_flags.test(RFLAG_CINEMA_BORDERS))
		CinemaBordersEnabled = 1;

	if (ps_r_overlay_flags.test(RFLAG_WATERMARK))
		WatermarkEnabled = 1;

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_frame_overlay->E[0]);

	RenderBackend.set_Constant("enabled_overlays", (float)GridEnabled, (float)CinemaBordersEnabled, (float)WatermarkEnabled, 0);

	RenderTarget->RenderViewportSurface(RenderTarget->rt_Generic_0);
}
///////////////////////////////////////////////////////////////////////////////////