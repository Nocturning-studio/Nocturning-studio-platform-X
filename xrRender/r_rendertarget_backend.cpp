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

void CRenderTarget::u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, IDirect3DSurface9* zb)
{
	VERIFY(_1);

	dwWidth = _1->dwWidth;
	dwHeight = _1->dwHeight;

	if (_1)
		RCache.set_RT(_1->pRT, 0);
	else
		RCache.set_RT(NULL, 0);

	if (_2)
		RCache.set_RT(_2->pRT, 1);
	else
		RCache.set_RT(NULL, 1);

	if (_3)
		RCache.set_RT(_3->pRT, 2);
	else
		RCache.set_RT(NULL, 2);

	RCache.set_ZB(zb);
}

void CRenderTarget::u_setrt(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3,
							IDirect3DSurface9* zb)
{
	VERIFY(_1);
	dwWidth = W;
	dwHeight = H;
	VERIFY(_1);
	RCache.set_RT(_1, 0);
	RCache.set_RT(_2, 1);
	RCache.set_RT(_3, 2);
	RCache.set_ZB(zb);
	//	RImplementation.rmNormal				();
}

void CRenderTarget::u_stencil_optimize(BOOL common_stencil)
{
	VERIFY(RImplementation.o.nvstencil);
	RCache.set_ColorWriteEnable(FALSE);
	u32 Offset;
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	u32 C = color_rgba(255, 255, 255, 255);
	float eps = EPS_S;
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(eps, float(_h + eps), eps, 1.f, C, 0, 0);
	pv++;
	pv->set(eps, eps, eps, 1.f, C, 0, 0);
	pv++;
	pv->set(float(_w + eps), float(_h + eps), eps, 1.f, C, 0, 0);
	pv++;
	pv->set(float(_w + eps), eps, eps, 1.f, C, 0, 0);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);
	RCache.set_CullMode(CULL_NONE);
	if (common_stencil)
		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00); // keep/keep/keep
	RCache.set_Element(s_occq->E[0]);
	RCache.set_Geometry(g_combine);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
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
	m_Texgen.mul(m_TexelAdjust, RCache.xforms.m_wvp);
}

// 2D texgen for jitter (texture adjustment matrix)
void CRenderTarget::u_compute_texgen_jitter(Fmatrix& m_Texgen_J)
{
	// place into	0..1 space
	Fmatrix m_TexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f,  0.0f, 1.0f};
	m_Texgen_J.mul(m_TexelAdjust, RCache.xforms.m_wvp);

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
		RCache.set_ColorWriteEnable(FALSE);
		u32 Offset;
		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		u32 C = color_rgba(255, 255, 255, 255);
		float eps = EPS_S;
		FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
		pv->set(eps, float(_h + eps), eps, 1.f, C, 0, 0);
		pv++;
		pv->set(eps, eps, eps, 1.f, C, 0, 0);
		pv++;
		pv->set(float(_w + eps), float(_h + eps), eps, 1.f, C, 0, 0);
		pv++;
		pv->set(float(_w + eps), eps, eps, 1.f, C, 0, 0);
		pv++;
		RCache.Vertex.Unlock(4, g_combine->vb_stride);
		RCache.set_CullMode(CULL_NONE);
		//	Clear everything except last bit
		RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, dwLightMarkerID, 0x00, 0xFE, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO,
						   D3DSTENCILOP_ZERO);
		// RCache.set_Stencil	(TRUE,D3DCMP_ALWAYS,dwLightMarkerID,0x00,0xFF, D3DSTENCILOP_ZERO, D3DSTENCILOP_ZERO,
		// D3DSTENCILOP_ZERO);
		RCache.set_Element(s_occq->E[0]);
		RCache.set_Geometry(g_combine);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}

void CRenderTarget::increment_light_marker()
{
	dwLightMarkerID += 2;

	// if (dwLightMarkerID>10)
	if (dwLightMarkerID > 255)
		reset_light_marker(true);
}
////////////////////////////////////////////////////////////////////////////////
