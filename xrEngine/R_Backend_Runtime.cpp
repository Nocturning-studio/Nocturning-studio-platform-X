#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(pop)

#include "frustum.h"

void CBackend::OnFrameEnd()
{
	OPTICK_EVENT("CBackend::OnFrameEnd");

#ifndef DEDICATED_SERVER
	for (u32 stage = 0; stage < HW.Caps.raster.dwStages; stage++)
		CHK_DX(HW.pDevice->SetTexture(0, 0));
	CHK_DX(HW.pDevice->SetStreamSource(0, 0, 0, 0));
	CHK_DX(HW.pDevice->SetIndices(0));
	CHK_DX(HW.pDevice->SetVertexShader(0));
	CHK_DX(HW.pDevice->SetPixelShader(0));
	Invalidate();
#endif
}

void CBackend::OnFrameBegin()
{
	OPTICK_EVENT("CBackend::OnFrameBegin");

#ifndef DEDICATED_SERVER
	Memory.mem_fill(&stat, 0, sizeof(stat));
	Vertex.Flush();
	Index.Flush();
	set_Stencil(FALSE);
#endif
}

void CBackend::Invalidate()
{
	OPTICK_EVENT("CBackend::Invalidate");

	bBlend = FALSE;
	srcBlend = D3DBLEND_ONE;
	dstBlend = D3DBLEND_ZERO;

	pRT[0] = NULL;
	pRT[1] = NULL;
	pRT[2] = NULL;
	pRT[3] = NULL;
	pZB = NULL;

	decl = NULL;
	vb = NULL;
	ib = NULL;
	vb_stride = 0;

	state = NULL;
	ps = NULL;
	vs = NULL;
	ctable = NULL;

	T = NULL;
	M = NULL;
	C = NULL;

	colorwrite_mask = u32(-1);

	for (u32 ps_it = 0; ps_it < 16;)
		textures_ps[ps_it++] = 0;
	for (u32 vs_it = 0; vs_it < 5;)
		textures_vs[vs_it++] = 0;
#ifdef _EDITOR
	for (u32 m_it = 0; m_it < 8;)
		matrices[m_it++] = 0;
#endif
}

void CBackend::set_ClipPlanes(u32 _enable, Fplane* _planes /*=NULL */, u32 count /* =0*/)
{
	OPTICK_EVENT("CBackend::set_ClipPlanes");

	if (0 == HW.Caps.geometry.dwClipPlanes)
		return;
	if (!_enable)
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE));
		return;
	}

	// Enable and setup planes
	VERIFY(_planes && count);
	if (count > HW.Caps.geometry.dwClipPlanes)
		count = HW.Caps.geometry.dwClipPlanes;

	D3DXMATRIX worldToClipMatrixIT;
	D3DXMatrixInverse(&worldToClipMatrixIT, NULL, (D3DXMATRIX*)&Device.mFullTransform);
	D3DXMatrixTranspose(&worldToClipMatrixIT, &worldToClipMatrixIT);
	for (u32 it = 0; it < count; it++)
	{
		Fplane& P = _planes[it];
		D3DXPLANE planeWorld(-P.n.x, -P.n.y, -P.n.z, -P.d), planeClip;
		D3DXPlaneNormalize(&planeWorld, &planeWorld);
		D3DXPlaneTransform(&planeClip, &planeWorld, &worldToClipMatrixIT);
		CHK_DX(HW.pDevice->SetClipPlane(it, planeClip));
	}

	// Enable them
	u32 e_mask = (1 << count) - 1;
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, e_mask));
}

#ifndef DEDICATED_SREVER
void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */)
{
	OPTICK_EVENT("CBackend::set_ClipPlanes");

	if (0 == HW.Caps.geometry.dwClipPlanes)
		return;
	if (!_enable)
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE));
		return;
	}
	VERIFY(_xform && fmask);
	CFrustum F;
	F.CreateFromMatrix(*_xform, fmask);
	set_ClipPlanes(_enable, F.planes, F.p_count);
}

void CBackend::set_Textures(STextureList* _T)
{
	OPTICK_EVENT("CBackend::set_Textures");

	if (T == _T)
		return;
	T = _T;
	u32 _last_ps = 0;
	u32 _last_vs = 0;
	STextureList::iterator _it = _T->begin();
	STextureList::iterator _end = _T->end();
	for (; _it != _end; _it++)
	{
		std::pair<u32, ref_texture>& loader = *_it;
		u32 load_id = loader.first;
		CTexture* load_surf = &*loader.second;
		if (load_id < 256)
		{
			// ordinary pixel surface
			if (load_id > _last_ps)
				_last_ps = load_id;
			if (textures_ps[load_id] != load_surf)
			{
				textures_ps[load_id] = load_surf;
#ifdef DEBUG
				stat.textures++;
#endif
				if (load_surf)
				{
					OPTICK_EVENT("set_Textures - load_surf");
					load_surf->bind(load_id);
					//					load_surf->Apply	(load_id);
				}
			}
		}
		else
		{
			// d-map or vertex
			u32 load_id_remapped = load_id - 256;
			if (load_id_remapped > _last_vs)
				_last_vs = load_id_remapped;
			if (textures_vs[load_id_remapped] != load_surf)
			{
				textures_vs[load_id_remapped] = load_surf;
#ifdef DEBUG
				stat.textures++;
#endif
				if (load_surf)
				{
					OPTICK_EVENT("set_Textures - load_surf");
					load_surf->bind(load_id);
					//					load_surf->Apply	(load_id);
				}
			}
		}
	}

	// clear remaining stages (PS)
	for (++_last_ps; _last_ps < 16 && textures_ps[_last_ps]; _last_ps++)
	{
		textures_ps[_last_ps] = 0;
		CHK_DX(HW.pDevice->SetTexture(_last_ps, NULL));
	}
	// clear remaining stages (VS)
	for (++_last_vs; _last_vs < 5 && textures_vs[_last_vs]; _last_vs++)
	{
		textures_vs[_last_vs] = 0;
		CHK_DX(HW.pDevice->SetTexture(_last_vs + 256, NULL));
	}
}
#else

void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */)
{
}
void CBackend::set_Textures(STextureList* _T)
{
}

#endif

void CBackend::set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
{
	R_ASSERT2(_1, "Rendertarget must have minimum one target surface (ref_rt& _1)");

	RenderBackend.setRenderTarget(_1->pRT, 0);

	if (_2)
		RenderBackend.setRenderTarget(_2->pRT, 1);
	else
		RenderBackend.setRenderTarget(NULL, 1);

	if (_3)
		RenderBackend.setRenderTarget(_3->pRT, 2);
	else
		RenderBackend.setRenderTarget(NULL, 2);

	if (_4)
		RenderBackend.setRenderTarget(_4->pRT, 3);
	else
		RenderBackend.setRenderTarget(NULL, 3);
}

void CBackend::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3, IDirect3DSurface9* _4)
{
	R_ASSERT2(_1, "Rendertarget must have minimum one target surface (IDirect3DSurface9* _1)");

	//dwWidth = W;
	//dwHeight = H;

	RenderBackend.setRenderTarget(_1, 0);

	if (_2)
		RenderBackend.setRenderTarget(_2, 1);
	else
		RenderBackend.setRenderTarget(NULL, 1);

	if (_3)
		RenderBackend.setRenderTarget(_3, 2);
	else
		RenderBackend.setRenderTarget(NULL, 2);

	if (_4)
		RenderBackend.setRenderTarget(_4, 3);
	else
		RenderBackend.setRenderTarget(NULL, 3);
}

void CBackend::set_Depth_Buffer(IDirect3DSurface9* zb)
{
	RenderBackend.setDepthBuffer(zb);
}

void CBackend::clear_Depth_Buffer(IDirect3DSurface9* zb)
{
	RenderBackend.setDepthBuffer(zb);
	CHK_DX(HW.pDevice->Clear(0L, nullptr, D3DCLEAR_ZBUFFER, 0x0, 1.0f, 0L));
}

void CBackend::set_Blend(BOOL enable, D3DBLEND src, D3DBLEND dest)
{
	OPTICK_EVENT("CBackend::set_Blend");

	// Check if state actually changed to avoid redundant API calls
	if (bBlend != enable || srcBlend != src || dstBlend != dest)
	{
		bBlend = enable;
		srcBlend = src;
		dstBlend = dest;

		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, enable));

		if (enable)
		{
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_SRCBLEND, src));
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_DESTBLEND, dest));

			// Также установим правильные состояния для альфа-тестинга если нужно
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE));
		}
	}
}

void CBackend::set_Blend_Alpha()
{
	set_Blend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
}

void CBackend::set_Blend_Add()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CBackend::set_Blend_Multiply()
{
	set_Blend(TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);
}

void CBackend::set_Blend_Default()
{
	set_Blend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
}

// Также добавим методы для других распространенных blend режимов
void CBackend::set_Blend_Subtract()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT));
}

void CBackend::set_Blend_Screen()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR);
}

void CBackend::set_Blend_LightAdd()
{
	set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CBackend::set_Blend_ColorAdd()
{
	set_Blend(TRUE, D3DBLEND_SRCCOLOR, D3DBLEND_ONE);
}

void CBackend::set_BlendEx(BOOL enable, D3DBLEND src, D3DBLEND dest, D3DBLENDOP op)
{
	OPTICK_EVENT("CBackend::set_BlendEx");

	if (bBlend != enable || srcBlend != src || dstBlend != dest)
	{
		bBlend = enable;
		srcBlend = src;
		dstBlend = dest;

		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, enable));

		if (enable)
		{
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_SRCBLEND, src));
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_DESTBLEND, dest));
			CHK_DX(HW.pDevice->SetRenderState(D3DRS_BLENDOP, op));

			CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE));
		}
	}
}

// 2D texgen (texture adjustment matrix)
void CBackend::u_compute_texgen_screen(Fmatrix& m_Texgen)
{
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	float o_w = (.5f / _w);
	float o_h = (.5f / _h);
	Fmatrix m_TexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 
							 0.0f, -0.5f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f, 
							 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};
	m_Texgen.mul(m_TexelAdjust, RenderBackend.xforms.m_wvp);
}

void CBackend::set_viewport_geometry(float w, float h, ref_geom geometry, u32& vOffset)
{
	OPTICK_EVENT("CRenderTarget::set_viewport_geometry")

	// Constants
	u32 C = color_rgba(0, 0, 0, 255);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, geometry->vb_stride, vOffset);
	pv->set_position(0, h, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p0.x, p1.y);
	pv++;

	pv->set_position(0, 0, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p0.x, p0.y);
	pv++;

	pv->set_position(w, h, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p1.x, p1.y);
	pv++;

	pv->set_position(w, 0, d_Z, d_W);
	pv->set_color(C);
	pv->set_uv(p1.x, p0.y);
	pv++;
	RenderBackend.Vertex.Unlock(4, geometry->vb_stride);

	// Set geometry
	RenderBackend.set_Geometry(geometry);
}

void CBackend::set_viewport_geometry(float w, float h, u32& vOffset)
{
	set_viewport_geometry(w, h, g_viewport, vOffset);
}

void CBackend::set_viewport_geometry(ref_geom geometry, u32& vOffset)
{
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);
	set_viewport_geometry(w, h, geometry, vOffset);
}

void CBackend::set_viewport_geometry(u32& vOffset)
{
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);
	set_viewport_geometry(w, h, g_viewport, vOffset);
}

void CBackend::RenderViewportSurface()
{
	u32 Offset = 0;
	set_viewport_geometry(Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CBackend::RenderViewportSurface(const ref_rt& _1)
{
	RenderViewportSurface(_1, NULL);
}

void CBackend::RenderViewportSurface(const ref_rt& _1, IDirect3DSurface9* zb)
{
	set_Render_Target_Surface(_1);
	set_Depth_Buffer(zb);

	u32 Offset = 0;
	set_viewport_geometry(Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CBackend::RenderViewportSurface(float w, float h, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
{
	set_Render_Target_Surface(_1, _2, _3, _4);
	set_Depth_Buffer(NULL);

	u32 Offset = 0;
	set_viewport_geometry(w, h, Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CBackend::RenderViewportSurface(float w, float h, IDirect3DSurface9* _1)
{
	set_Render_Target_Surface((u32)w, (u32)h, _1);
	set_Depth_Buffer(NULL);

	u32 Offset = 0;
	set_viewport_geometry(w, h, Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CBackend::RenderViewportSurface(float w, float h, IDirect3DSurface9* _1, IDirect3DSurface9* zb)
{
	set_Render_Target_Surface((u32)w, (u32)h, _1);
	set_Depth_Buffer(zb);

	u32 Offset = 0;
	set_viewport_geometry(w, h, Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CBackend::ClearTexture(const ref_rt& _1, u32 color)
{
	set_Render_Target_Surface(_1, NULL, NULL, NULL);
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L));
}

void CBackend::ClearTexture(const ref_rt& _1, const ref_rt& _2, u32 color)
{
	set_Render_Target_Surface(_1, _2, NULL, NULL);
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L));
}

void CBackend::ClearTexture(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, u32 color)
{
	set_Render_Target_Surface(_1, _2, _3, NULL);
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L));
}

void CBackend::ClearTexture(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4, u32 color)
{
	set_Render_Target_Surface(_1, _2, _3, _4);
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L));
}