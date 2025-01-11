///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_barrel_blur()
{
	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	set_viewport_vertex_buffer(w, h, Offset);

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_barrel_blur->E[0]);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_barrel_blur->E[1]);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}
///////////////////////////////////////////////////////////////////////////////////
