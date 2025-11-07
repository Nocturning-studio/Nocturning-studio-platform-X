#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"

CRT::CRT()
{
	pSurface = NULL;
	pRT = NULL;
	pZRT = NULL;
	pUAView = NULL;
	dwWidth = 0;
	dwHeight = 0;
	format = DXGI_FORMAT_UNKNOWN;
	bind_flags = 0;
	samples = 1;
}
CRT::~CRT()
{
	destroy();
	// release external reference
	Device.Resources->_DeleteRT(this);
}

void CRT::create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, u32 _bind_flags, u32 s)
{
	if (pSurface)
		return;

	R_ASSERT(HW.pDevice11 && name && name[0] && w && h);

	dwWidth = w;
	dwHeight = h;
	format = f;
	samples = s;
	bind_flags = _bind_flags;

	// Check width-and-height of render target surface
	if (w > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
		return;
	if (h > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
		return;

	DXGI_FORMAT ftex = f;

	switch (ftex)
	{
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		ftex = DXGI_FORMAT_R32G32_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT:
		ftex = DXGI_FORMAT_R32_TYPELESS;
		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		ftex = DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case DXGI_FORMAT_D16_UNORM:
		ftex = DXGI_FORMAT_R16_TYPELESS;
		break;
	}

	// Try to create texture/surface
	Device.Resources->Evict();

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = dwWidth;
	desc.Height = dwHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = ftex;
	desc.SampleDesc.Count = samples;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = bind_flags;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	//#ifdef DEBUG
	Msg("* Create texture: %dx%d, %d(sr=%d, rt=%d, ua=%d, ds=%d), s=%d, '%s'", dwWidth, dwHeight, format, (bool)(bind_flags & fSR), 
		(bool)(bind_flags & fRT), (bool)(bind_flags & fUA), (bool)(bind_flags & fDS), samples, name);
	//#endif

	// DX10.1 and later hardware with MSAA
	if (samples > 1 && HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1)
		desc.SampleDesc.Quality = UINT(D3D11_STANDARD_MULTISAMPLE_PATTERN);

	// Create Texture2D
	HW.pDevice11->CreateTexture2D(&desc, NULL, &pSurface);
	R_ASSERT(pSurface);
	//HW.stats_manager.increment_stats_rtarget(pSurface);

	// Create rendertarget view
	if (bind_flags & fRT)
	{
		HW.pDevice11->CreateRenderTargetView(pSurface, NULL, &pRT);
		R_ASSERT(pRT);
	}

	// Create depth stencil view
	if (bind_flags & fDS)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC depthstencil = {};
		depthstencil.Format = format;
		depthstencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthstencil.Texture2D.MipSlice = 0;
		depthstencil.Flags = 0;

		if (samples > 1)
		{
			depthstencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
			depthstencil.Texture2DMS.UnusedField_NothingToDefine = 0;
		}

		HW.pDevice11->CreateDepthStencilView(pSurface, &depthstencil, &pZRT);
		R_ASSERT(pZRT);
	}

	// Create unordered acces view
	if (bind_flags & fUA)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedacces = {};
		unorderedacces.Format = format;
		unorderedacces.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		unorderedacces.Buffer.FirstElement = 0;
		unorderedacces.Buffer.NumElements = dwWidth * dwHeight;

		HW.pDevice11->CreateUnorderedAccessView(pSurface, &unorderedacces, &pUAView);
		R_ASSERT(pUAView);
	}

	pTexture = Device.Resources->_CreateTexture(name);
	pTexture->surface_set(pSurface);
}

void CRT::destroy()
{
	if (pTexture._get())
	{
		pTexture->surface_set(0);
		pTexture = NULL;
	}

	_RELEASE(pRT);
	_RELEASE(pZRT);

	//HW.stats_manager.decrement_stats_rtarget(pSurface);

	_RELEASE(pSurface);
	_RELEASE(pUAView);
}
void CRT::reset_begin()
{
	destroy();
}
void CRT::reset_end()
{
	create(*cName, dwWidth, dwHeight, format, bind_flags, samples);
}

void resptrcode_crt::create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, u32 bind_flags, u32 samples)
{
	_set(Device.Resources->_CreateRT(name, w, h, f, bind_flags, samples));
}

void resptrcode_crt::create(LPCSTR name, u32 w, u32 h, D3DFORMAT f)
{
	DXGI_FORMAT _f = DXGI_FORMAT_UNKNOWN;
	u32 _flags = 0;

	switch (f)
	{
	case D3DFMT_A16B16G16R16F:
		_f = DXGI_FORMAT_R16G16B16A16_FLOAT;
		_flags = fSR | fRT;
		break;
	case D3DFMT_A8R8G8B8:
		_f = DXGI_FORMAT_R8G8B8A8_UNORM;
		_flags = fSR | fRT;
		break;
	case D3DFMT_D24X8:
		_f = DXGI_FORMAT_D24_UNORM_S8_UINT;
		_flags = fDS;
		break;
	case D3DFMT_R5G6B5:
		_f = DXGI_FORMAT_B5G6R5_UNORM;
		_flags = fSR | fRT;
		break;
	}

	R_ASSERT(_f != f);

	create(name, w, h, _f, _flags);
}
