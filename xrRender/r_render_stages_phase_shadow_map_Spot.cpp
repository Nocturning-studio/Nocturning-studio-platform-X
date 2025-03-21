#include "stdafx.h"

void CRender::clear_shadow_map_spot()
{
	/*
	if (RenderImplementation.b_HW_smap)		set_Render_Target_Surface	(rt_smap_surf, NULL, NULL, NULL, rt_smap_d_depth->pRT);
	else								set_Render_Target_Surface	(rt_smap_surf, NULL, NULL, NULL, rt_smap_d_ZB);
	CHK_DX								(HW.pDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER,	0xffffffff,	1.0f, 0L));
	*/
}

void CRender::render_shadow_map_spot(light* L)
{
	// Targets + viewport
	RenderTargetBackend->set_Render_Target_Surface(RenderTarget->rt_smap_surf);
	RenderTargetBackend->set_Depth_Buffer(RenderTarget->rt_smap_depth->pRT);

	D3DVIEWPORT9 VP = {L->X.S.posX, L->X.S.posY, L->X.S.size, L->X.S.size, 0, 1};
	CHK_DX(HW.pDevice->SetViewport(&VP));

	// Misc	- draw only front-faces
	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_Stencil(FALSE);
	// no transparency
#pragma todo("can optimize for multi-lights covering more than say 50%...")

	RenderBackend.set_ColorWriteEnable(FALSE);
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0L));
}

void CRender::render_shadow_map_spot_transluent(light* L)
{
	VERIFY(RenderImplementation.o.Tshadows);
	RenderBackend.set_ColorWriteEnable();
	if (IRender_Light::OMNIPART == L->flags.type)
	{
		// omni-part
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, 0xffffffff, 1.0f, 0L));
	}
	else
	{
		// real-spot
		// Select color-mask
		ref_shader shader = L->s_spot;
		if (!shader)
			shader = RenderTarget->s_accum_spot;
		RenderBackend.set_Element(shader->E[SE_L_FILL]);

		// Fill vertex buffer
		Fvector2 p0, p1;
		u32 Offset;
		u32 C = color_rgba(255, 255, 255, 255);
		float _w = float(L->X.S.size);
		float _h = float(L->X.S.size);
		float d_Z = EPS_S;
		float d_W = 1.f;
		p0.set(.5f / _w, .5f / _h);
		p1.set((_w + .5f) / _w, (_h + .5f) / _h);

		FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, RenderTargetBackend->g_viewport->vb_stride, Offset);
		pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
		pv++;
		pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
		pv++;
		pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
		pv++;
		pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
		pv++;
		RenderBackend.Vertex.Unlock(4, RenderTargetBackend->g_viewport->vb_stride);
		RenderBackend.set_Geometry(RenderTargetBackend->g_viewport);

		// draw
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
