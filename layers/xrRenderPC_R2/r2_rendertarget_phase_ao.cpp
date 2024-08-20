///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r2_rendertarget.h"
#include "Blender_ambient_occlusion.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_ao()
{
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	RCache.set_Geometry(g_combine);

	u_setrt(rt_ao, NULL, NULL, NULL);

	if (ps_r2_ao_quality == 1)
	{
		for (u32 i = 0; i < s_ambient_occlusion->E[SE_AO_SSAO]->passes.size(); i++)
		{
			RCache.set_Element(s_ambient_occlusion->E[SE_AO_SSAO], i);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}
	}
	else if (ps_r2_ao_quality == 2)
	{
		for (u32 i = 0; i < s_ambient_occlusion->E[SE_AO_HBAO_PLUS]->passes.size(); i++)
		{
			RCache.set_Element(s_ambient_occlusion->E[SE_AO_HBAO_PLUS], i);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}
	}
	else if (ps_r2_ao_quality == 3)
	{
		for (u32 i = 0; i < s_ambient_occlusion->E[SE_AO_GTAO]->passes.size(); i++)
		{
			RCache.set_Element(s_ambient_occlusion->E[SE_AO_GTAO], i);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////
