////////////////////////////////////////////////////////////////////////////////
// Created: 07.01.2025
// Author: Deathman
// Nocturning studio for NS Project X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
#include "..\xrEngine\resourcemanager.h"
////////////////////////////////////////////////////////////////////////////////
float CRenderTarget::hclip(float v, float dim)
{
	return 2.f * v / dim - 1.f;
}

void CRenderTarget::set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
{
	R_ASSERT2(_1, "Rendertarget must have minimum one target surface (ref_rt& _1)");

	dwWidth = _1->dwWidth;
	dwHeight = _1->dwHeight;

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

void CRenderTarget::set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3)
{
	set_Render_Target_Surface(_1, _2, _3, NULL);
}

void CRenderTarget::set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2)
{
	set_Render_Target_Surface(_1, _2, NULL, NULL);
}

void CRenderTarget::set_Render_Target_Surface(const ref_rt& _1)
{
	set_Render_Target_Surface(_1, NULL, NULL, NULL);
}

void CRenderTarget::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3, IDirect3DSurface9* _4)
{
	R_ASSERT2(_1, "Rendertarget must have minimum one target surface (IDirect3DSurface9* _1)");

	dwWidth = W;
	dwHeight = H;

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

void CRenderTarget::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3)
{
	set_Render_Target_Surface(W, H, _1, _2, _3, NULL);
}

void CRenderTarget::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2)
{
	set_Render_Target_Surface(W, H, _1, _2, NULL, NULL);
}

void CRenderTarget::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1)
{
	set_Render_Target_Surface(W, H, _1, NULL, NULL, NULL);
}

void CRenderTarget::set_Depth_Buffer(IDirect3DSurface9* zb)
{
	RenderBackend.setDepthBuffer(zb);
}

void CRenderTarget::u_stencil_optimize(BOOL common_stencil)
{
	VERIFY(RenderImplementation.o.nvstencil);
	RenderBackend.set_ColorWriteEnable(FALSE);
	u32 Offset;
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	u32 C = color_rgba(255, 255, 255, 255);
	float eps = EPS_S;
	FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(eps, float(_h + eps), eps, 1.f, C, 0, 0);
	pv++;
	pv->set(eps, eps, eps, 1.f, C, 0, 0);
	pv++;
	pv->set(float(_w + eps), float(_h + eps), eps, 1.f, C, 0, 0);
	pv++;
	pv->set(float(_w + eps), eps, eps, 1.f, C, 0, 0);
	pv++;
	RenderBackend.Vertex.Unlock(4, g_combine->vb_stride);
	RenderBackend.set_CullMode(CULL_NONE);
	if (common_stencil)
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00); // keep/keep/keep
	RenderBackend.set_Element(s_occq->E[0]);
	RenderBackend.set_Geometry(g_combine);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

// 2D texgen (texture adjustment matrix)
void CRenderTarget::u_compute_texgen_screen(Fmatrix& m_Texgen)
{
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	float o_w = (.5f / _w);
	float o_h = (.5f / _h);
	Fmatrix m_TexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f,		 -0.5f,		 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f, 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};
	m_Texgen.mul(m_TexelAdjust, RenderBackend.xforms.m_wvp);
}

// 2D texgen for jitter (texture adjustment matrix)
void CRenderTarget::u_compute_texgen_jitter(Fmatrix& m_Texgen_J)
{
	// place into	0..1 space
	Fmatrix m_TexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f,  0.0f, 1.0f};
	m_Texgen_J.mul(m_TexelAdjust, RenderBackend.xforms.m_wvp);

	// rescale - tile it
	float scale_X = float(Device.dwWidth) / float(TEX_jitter);
	float scale_Y = float(Device.dwHeight) / float(TEX_jitter);
	float offset = (.5f / float(TEX_jitter));
	m_TexelAdjust.scale(scale_X, scale_Y, 1.f);
	m_TexelAdjust.translate_over(offset, offset, 0);
	m_Texgen_J.mulA_44(m_TexelAdjust);
}

void CRenderTarget::reset_light_marker(bool bResetStencil)
{
	dwLightMarkerID = 5;
	if (bResetStencil)
	{
		RenderBackend.set_ColorWriteEnable(FALSE);
		u32 Offset;
		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		u32 C = color_rgba(255, 255, 255, 255);
		float eps = EPS_S;
		FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, g_combine->vb_stride, Offset);
		pv->set(eps, float(_h + eps), eps, 1.f, C, 0, 0);
		pv++;
		pv->set(eps, eps, eps, 1.f, C, 0, 0);
		pv++;
		pv->set(float(_w + eps), float(_h + eps), eps, 1.f, C, 0, 0);
		pv++;
		pv->set(float(_w + eps), eps, eps, 1.f, C, 0, 0);
		pv++;
		RenderBackend.Vertex.Unlock(4, g_combine->vb_stride);
		RenderBackend.set_CullMode(CULL_NONE);
		//	Clear everything except last bit
		RenderBackend.set_Stencil(TRUE, D3DCMP_ALWAYS, dwLightMarkerID, 0x00, 0xFE, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO,
						   D3DSTENCILOP_ZERO);
		// RenderBackend.set_Stencil	(TRUE,D3DCMP_ALWAYS,dwLightMarkerID,0x00,0xFF, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO,
		// D3DSTENCILOP_ZERO);
		RenderBackend.set_Element(s_occq->E[0]);
		RenderBackend.set_Geometry(g_combine);
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}

void CRenderTarget::increment_light_marker()
{
	dwLightMarkerID += 2;

	// if (dwLightMarkerID>10)
	if (dwLightMarkerID > 255)
		reset_light_marker(true);
}

void CRenderTarget::set_viewport_geometry(float w, float h, u32& vOffset)
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
	FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, g_viewport->vb_stride, vOffset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RenderBackend.Vertex.Unlock(4, g_viewport->vb_stride);

	// Set geometry
	RenderBackend.set_Geometry(g_viewport);
}

void CRenderTarget::set_viewport_geometry(u32& vOffset)
{
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);
	set_viewport_geometry(w, h, vOffset);
}
////////////////////////////////////////////////////////////////////////////////
