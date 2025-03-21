////////////////////////////////////////////////////////////////////////////////
// Created: 07.01.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "r_rendertarget_backend.h"
#include "..\xrEngine\resourcemanager.h"
////////////////////////////////////////////////////////////////////////////////
CRenderTargetBackend::CRenderTargetBackend()
{
	dwWidth = Device.dwWidth;
	dwHeight = Device.dwHeight;

	g_viewport.create(FVF::F_TL, RenderBackend.Vertex.Buffer(), RenderBackend.QuadIB);
}

CRenderTargetBackend::~CRenderTargetBackend()
{
}

void CRenderTargetBackend::set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
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

void CRenderTargetBackend::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3, IDirect3DSurface9* _4)
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

void CRenderTargetBackend::set_Depth_Buffer(IDirect3DSurface9* zb)
{
	RenderBackend.setDepthBuffer(zb);
}

// 2D texgen (texture adjustment matrix)
void CRenderTargetBackend::u_compute_texgen_screen(Fmatrix& m_Texgen)
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

// 2D texgen for jitter (texture adjustment matrix)
void CRenderTargetBackend::u_compute_texgen_jitter(Fmatrix& m_Texgen_J)
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

void CRenderTargetBackend::set_viewport_geometry(float w, float h, u32& vOffset)
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

void CRenderTargetBackend::set_viewport_geometry(u32& vOffset)
{
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);
	set_viewport_geometry(w, h, vOffset);
}

void CRenderTargetBackend::RenderViewportSurface()
{
	u32 Offset = 0;
	set_viewport_geometry(Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTargetBackend::RenderViewportSurface(const ref_rt& _1)
{
	set_Render_Target_Surface(_1);
	set_Depth_Buffer(NULL);

	u32 Offset = 0;
	set_viewport_geometry(Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTargetBackend::RenderViewportSurface(float w, float h, const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4)
{
	set_Render_Target_Surface(_1, _2, _3, _4);
	set_Depth_Buffer(NULL);

	u32 Offset = 0;
	set_viewport_geometry(w, h, Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTargetBackend::RenderViewportSurface(float w, float h, IDirect3DSurface9* _1)
{
	set_Render_Target_Surface(w, h, _1);
	set_Depth_Buffer(NULL);

	u32 Offset = 0;
	set_viewport_geometry(w, h, Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTargetBackend::RenderViewportSurface(float w, float h, IDirect3DSurface9* _1, IDirect3DSurface9* zb)
{
	set_Render_Target_Surface(w, h, _1);
	set_Depth_Buffer(zb);

	u32 Offset = 0;
	set_viewport_geometry(w, h, Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTargetBackend::ClearTexture(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4, u32 color)
{
	set_Render_Target_Surface(_1, _2, _3, _4);
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L));
}
////////////////////////////////////////////////////////////////////////////////
