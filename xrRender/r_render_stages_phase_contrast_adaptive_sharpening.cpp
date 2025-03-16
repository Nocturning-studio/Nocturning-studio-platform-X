///////////////////////////////////////////////////////////////////////////////////
//	Created		: 18.12.2023
//	Author		: Deathman
//  Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_contrast_adaptive_sharpening()
{
	set_Render_Target_Surface(rt_Generic_0);
	set_Depth_Buffer(NULL);

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_geometry(w, h, Offset);

	// Set constants
	RenderBackend.set_Constant("cas_params", ps_cas_contrast, ps_cas_sharpening, 0, 0);

	// Draw
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}
///////////////////////////////////////////////////////////////////////////////////
