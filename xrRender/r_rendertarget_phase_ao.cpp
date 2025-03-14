///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
#include "Blender_ambient_occlusion.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::draw_ao(int pass, u32 Offset)
{
	OPTICK_EVENT("CRenderTarget::draw_ao");

	for (u32 i = 0; i < s_ambient_occlusion->E[pass]->passes.size(); i++)
	{
		RCache.set_Element(s_ambient_occlusion->E[pass], i);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}

void CRenderTarget::phase_ao()
{
	OPTICK_EVENT("CRenderTarget::phase_ao");

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Set geometry
	set_viewport_vertex_buffer(w, h, Offset);

	u_setrt(rt_ao, NULL, NULL, NULL, NULL);

	switch (ps_r_ao_quality)
	{
	case 1:
		draw_ao(SE_AO_SSAO, Offset);
		break;
	case 2:
		draw_ao(SE_AO_HBAO_PLUS, Offset);
		break;
	case 3:
		draw_ao(SE_AO_GTAO, Offset);
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
