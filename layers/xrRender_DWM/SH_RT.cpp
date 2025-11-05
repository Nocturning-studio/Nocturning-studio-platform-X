#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "dxRenderDeviceRender.h"

CRT::CRT()
{
	pSurface	= NULL;
	pRT			= NULL;
	pZRT		= NULL;
	pUAView		= NULL;
	dwWidth		= 0;
	dwHeight	= 0;
	format		= DXGI_FORMAT_UNKNOWN;
	view		= SRV_RTV;
	samples		= 1;
}
CRT::~CRT()
{
	destroy();
	// release external reference
	DEV->_DeleteRT(this);
}

void CRT::create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, VIEW_TYPE view, u32 s)
{
	if (pSurface) return;

	R_ASSERT(HW.pDevice && name && name[0] && w && h);

	_order = CPU::GetCLK();

	dwWidth		= w;
	dwHeight	= h;
	format		= f;
	samples		= s;

	// Check width-and-height of render target surface
	if (w > D3D_REQ_TEXTURE2D_U_OR_V_DIMENSION) return;
	if (h > D3D_REQ_TEXTURE2D_U_OR_V_DIMENSION) return;

	DXGI_FORMAT ftex = f;

	switch (ftex)
	{
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:	ftex = DXGI_FORMAT_R32G32_TYPELESS;		break;
	case DXGI_FORMAT_D32_FLOAT:				ftex = DXGI_FORMAT_R32_TYPELESS;		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:		ftex = DXGI_FORMAT_R24G8_TYPELESS;		break;
	case DXGI_FORMAT_D16_UNORM:				ftex = DXGI_FORMAT_R16_TYPELESS;		break;
	}

//#ifdef DEBUG
	Msg("* Make 2D tex: N=%s, %dx%d, FMT=%d, MSAA=%d, TYPE=%d", name, dwWidth, dwHeight, format, samples, view);
//#endif

	// Try to create texture/surface
	DEV->Evict();

	D3D_TEXTURE2D_DESC desc = {};
	desc.Width = dwWidth;
	desc.Height = dwHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = ftex;
	desc.SampleDesc.Count = samples;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D_USAGE_DEFAULT;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	bool use_uav = HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_0 && view == SRV_RTV_UAV && samples <= 1;
	bool use_dsv = view == SRV_DSV || view == SRV_RTV_DSV;
	bool use_rtv = view == SRV_RTV || view == SRV_RTV_UAV || view == SRV_RTV_DSV;
	bool use_srv = HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1 || !use_dsv || samples <= 1;

	if (use_srv) desc.BindFlags |= D3D_BIND_SHADER_RESOURCE;
	if (use_rtv) desc.BindFlags |= D3D_BIND_RENDER_TARGET;
	if (use_uav) desc.BindFlags |= D3D_BIND_UNORDERED_ACCESS;
	if (use_dsv) desc.BindFlags |= D3D_BIND_DEPTH_STENCIL;

	// DX10.1 and later hardware with MSAA
	if (samples > 1 && HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1)
		desc.SampleDesc.Quality = UINT(D3D_STANDARD_MULTISAMPLE_PATTERN); 

	// Create Texture2D
	CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &pSurface));
	HW.stats_manager.increment_stats_rtarget(pSurface);

	// Create rendertarget view
	if (use_rtv)
	{
		CHK_DX(HW.pDevice->CreateRenderTargetView(pSurface, NULL, &pRT));
	}

	// Create depth stencil view
	if (use_dsv)
	{
		D3D_DEPTH_STENCIL_VIEW_DESC	depthstencil = {};
		depthstencil.Format = format;
		depthstencil.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2D;
		depthstencil.Texture2D.MipSlice = 0;
		depthstencil.Flags = 0;

		if (samples > 1)
		{
			depthstencil.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DMS;
			depthstencil.Texture2DMS.UnusedField_NothingToDefine = 0;
		}

		CHK_DX(HW.pDevice->CreateDepthStencilView(pSurface, &depthstencil, &pZRT));
	}

	// Create unordered acces view
	if (use_uav)
	{
		D3D_UNORDERED_ACCESS_VIEW_DESC unorderedacces = {};
		unorderedacces.Format = format;
		unorderedacces.ViewDimension = D3D_UAV_DIMENSION_TEXTURE2D;
		unorderedacces.Buffer.FirstElement = 0;
		unorderedacces.Buffer.NumElements = dwWidth * dwHeight;

		CHK_DX(HW.pDevice->CreateUnorderedAccessView(pSurface, &unorderedacces, &pUAView));
	}

	pTexture = DEV->_CreateTexture(name);
	pTexture->surface_set(pSurface);
}

void CRT::destroy		()
{
	if (pTexture._get())	
	{
		pTexture->surface_set(0);
		pTexture = NULL;
	}

	_RELEASE(pRT);
	_RELEASE(pZRT);
	
	HW.stats_manager.decrement_stats_rtarget(pSurface);

	_RELEASE(pSurface);
	_RELEASE(pUAView);

}
void CRT::reset_begin()
{
	destroy();
}
void CRT::reset_end()
{
	create(*cName, dwWidth, dwHeight, format, view, samples);
}

void resptrcode_crt::create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, VIEW_TYPE view, u32 samples)
{
	_set(DEV->_CreateRT(name, w, h, f, view, samples));
}
