// HW.cpp: implementation of the CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "HW.h"
#include "../xrEngine/XR_IOConsole.h"
#include "../Include/xrAPI/xrAPI.h"

#include "SamplerStateCache.h"
#include "StateCache.h"

#include "xrRender_R4.h"

#ifndef _EDITOR
void	fill_vid_mode_list			(CHW* _hw);
void	free_vid_mode_list			();

void	fill_render_mode_list		();
void	free_render_mode_list		();
#else
void	fill_vid_mode_list			(CHW* _hw)	{}
void	free_vid_mode_list			()			{}
void	fill_render_mode_list		()			{}
void	free_render_mode_list		()			{}
#endif

CHW			HW;

//	DX10: Don't neeed this?
/*
#ifdef DEBUG
IDirect3DStateBlock9*	dwDebugSB = 0;
#endif
*/

CHW::CHW() : 
//	hD3D(NULL),
	//pD3D(NULL),
	m_pAdapter(0),
	pDevice(NULL),
	m_move_window(true)
	//pBaseRT(NULL),
	//pBaseZB(NULL)
{
	Device.seqAppActivate.Add(this);
	Device.seqAppDeactivate.Add(this);
}

CHW::~CHW()
{
	Device.seqAppActivate.Remove(this);
	Device.seqAppDeactivate.Remove(this);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateD3D()
{
	IDXGIFactory * pFactory;
	R_CHK( CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory)) );

	m_pAdapter = 0;
	m_bUsePerfhud = false;

#ifndef	MASTER_GOLD
	// Look for 'NVIDIA NVPerfHUD' adapter
	// If it is present, override default settings
	UINT i = 0;
	while(pFactory->EnumAdapters(i, &m_pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		m_pAdapter->GetDesc(&desc);
		if(!wcscmp(desc.Description,L"NVIDIA PerfHUD"))
		{
			m_bUsePerfhud = true;
			break;
		}
		else
		{
			m_pAdapter->Release();
			m_pAdapter = 0;
		}
		++i;
	}
#endif	//	MASTER_GOLD

	if (!m_pAdapter)
		pFactory->EnumAdapters(0, &m_pAdapter);

	pFactory->Release();
}

void CHW::DestroyD3D()
{
	_SHOW_REF				("refCount:m_pAdapter",m_pAdapter);
	_RELEASE				(m_pAdapter);
}

void CHW::CreateDevice(HWND m_hWnd, bool move_window)
{
	m_move_window = move_window;
	CreateD3D();

	// TODO: DX10: Create appropriate initialization

	// General - select adapter and device
	BOOL  bWindowed = !psDeviceFlags.is(rsFullscreen);

	m_DriverType = Caps.bForceGPU_REF ?
		D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_HARDWARE;

	if (m_bUsePerfhud)
		m_DriverType = D3D_DRIVER_TYPE_REFERENCE;

	// Display the name of video board
	DXGI_ADAPTER_DESC Desc;
	R_CHK(m_pAdapter->GetDesc(&Desc));

	//	Warning: Desc.Description is wide string
	Msg("* GPU [vendor:%X]-[device:%X]: %S", Desc.VendorId, Desc.DeviceId, Desc.Description);

	Caps.id_vendor = Desc.VendorId;
	Caps.id_device = Desc.DeviceId;

	// Set up the presentation parameters
	DXGI_SWAP_CHAIN_DESC& sd = m_ChainDesc;
	ZeroMemory(&sd, sizeof(sd));

	selectResolution(sd.BufferDesc.Width, sd.BufferDesc.Height, bWindowed);

	// Back buffer
	//	TODO: DX10: implement dynamic format selection
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferCount = 1;

	// Multisample
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	// Windoze
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.OutputWindow = m_hWnd;
	sd.Windowed = bWindowed;

	if (bWindowed)
	{
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
	}
	else
	{
		sd.BufferDesc.RefreshRate = selectRefresh(sd.BufferDesc.Width, sd.BufferDesc.Height, sd.BufferDesc.Format);
	}

	//	Additional set up
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	UINT createDeviceFlags = 0;
#ifdef DEBUG
	//createDeviceFlags |= D3Dxx_CREATE_DEVICE_DEBUG;
#endif
	HRESULT R;

#ifdef FEATURE_LEVELS_DEBUG1
	extern ENGINE_API u32 directx_level;

	if (directx_level)
	{
		R = D3D11CreateDeviceAndSwapChain(0,
			m_DriverType,
			NULL,
			createDeviceFlags,
			(D3D_FEATURE_LEVEL*)&directx_level,
			1,
			D3D11_SDK_VERSION,
			&sd,
			&m_pSwapChain,
			&pDevice,
			&FeatureLevel,
			&pContext);
	}
	else
#endif
	{
		R = D3D11CreateDeviceAndSwapChain(0,
			m_DriverType,
			NULL,
			createDeviceFlags,
			RenderCreationParams::levels12,
			RenderCreationParams::count12,
			D3D11_SDK_VERSION,
			&sd,
			&m_pSwapChain,
			&pDevice,
			&FeatureLevel,
			&pContext);

		if (FAILED(R))
		{
			R = D3D11CreateDeviceAndSwapChain(0,
				m_DriverType,
				NULL,
				createDeviceFlags,
				RenderCreationParams::levels,
				RenderCreationParams::count,
				D3D11_SDK_VERSION,
				&sd,
				&m_pSwapChain,
				&pDevice,
				&FeatureLevel,
				&pContext);
		}
	}

	if (FAILED(R))
	{
		// Fatal error! Cannot create rendering device AT STARTUP !!!
		Msg("Failed to initialize graphics hardware.\n"
			"Please try to restart the game.\n"
			"CreateDevice returned 0x%08x", R
		);
		FlushLog();
		MessageBox(NULL, "Failed to initialize graphics hardware.\nPlease try to restart the game.", "Error!", MB_OK | MB_ICONERROR);
		TerminateProcess(GetCurrentProcess(), 0);
	};

	R_CHK(R);

	m_12_level = false;

	{
		pDevice->QueryInterface(__uuidof(ID3D11Device3), reinterpret_cast<void**>(&pDevice3));
		pContext->QueryInterface(__uuidof(ID3D11DeviceContext3), reinterpret_cast<void**>(&pContext3));

		if (pDevice3 && pContext3)
		{
			Log("* DX11.3 supported and used");
			m_12_level = true;
		}
	}

	switch (FeatureLevel)
	{
	case D3D_FEATURE_LEVEL_10_0:	Msg("* Current DX11 feature level: 10.0"); break;
	case D3D_FEATURE_LEVEL_10_1:	Msg("* Current DX11 feature level: 10.1"); break;
	case D3D_FEATURE_LEVEL_11_0:	Msg("* Current DX11 feature level: 11.0"); break;
	case D3D_FEATURE_LEVEL_11_1:	Msg("* Current DX11 feature level: 11.1"); break;
	case D3D_FEATURE_LEVEL_12_0:	Msg("* Current DX11 feature level: 12.0"); break;
	case D3D_FEATURE_LEVEL_12_1:	Msg("* Current DX11 feature level: 12.1"); break;
	default:						Msg("* Current DX11 feature level: unknown"); break;
	}

	_SHOW_REF("* CREATE: DeviceREF:", HW.pDevice);

	// check for compute shaders support
	{
		D3D_OLD_HW_FEATURE_DATA Test;
		HW.pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, (void*)&Test, sizeof Test);
		m_cs_support = (BOOL)Test.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x;
		if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0) m_cs_support = true;
		if(!m_cs_support) Log("! Compute shaders are not supported");
	}

#ifdef __GFSDK_DX11__
	pSSAO = NULL;
	m_TXAA_initialized = false;

	if (r__ssao_mode == SSAO_HBAO_PLUS)
	{
		GFSDK_SSAO_CustomHeap CustomHeap;
		CustomHeap.new_ = ::operator new;
		CustomHeap.delete_ = ::operator delete;
		GFSDK_SSAO_CreateContext_D3D11(pDevice, &pSSAO, &CustomHeap, GFSDK_SSAO_Version());
	}

	if (r__aa == AA_TXAA ||
		r__aa == AA_TXAA2S ||
		r__aa == AA_TXAA4S)
	{
		NvTxaaStatus txaa_status = GFSDK_TXAA_DX11_InitializeContext(&m_TXAA, pDevice);
		m_TXAA_initialized = txaa_status == NV_TXAA_STATUS_OK || txaa_status == NV_TXAA_STATUS_CONTEXT_ALREADY_INITIALIZED;

		if (!m_TXAA_initialized)
		{
			Log("! TXAA does not supported");
		}
	}
#endif

	//	Create render target and depth-stencil views here
	UpdateViews();

	size_t	memory = Desc.DedicatedVideoMemory;
	Msg("*     Texture memory: %d M", memory / (1024 * 1024));

#ifndef _EDITOR
	updateWindowProps(m_hWnd);
	fill_vid_mode_list(this);
#endif
}

void CHW::DestroyDevice()
{
	//	Destroy state managers
	StateManager.Reset();
	RSManager.ClearStateArray();
	DSSManager.ClearStateArray();
	BSManager.ClearStateArray();
	SSManager.ClearStateArray();

	_SHOW_REF				("refCount:pBaseZB",		pBaseDepthReadWriteDSV);
	_SHOW_REF				("refCount:pBaseRT",		pBaseRT);

	_RELEASE				(pBaseDepthReadWriteDSV);
	_RELEASE				(pBaseDepthReadDSV);
	_RELEASE				(pBaseDepthReadSRV);
	_RELEASE				(pBaseRT);

	pBaseDepthSurface->Release();

	//	Must switch to windowed mode to release swap chain
	if (!m_ChainDesc.Windowed) m_pSwapChain->SetFullscreenState( FALSE, NULL);
	_SHOW_REF				("refCount:m_pSwapChain",m_pSwapChain);
	_RELEASE				(m_pSwapChain);


	_RELEASE				(pContext);

#ifdef __GFSDK_DX11__
	if(r__ssao_mode == SSAO_HBAO_PLUS)
		_RELEASE(pSSAO);

	if (r__aa == AA_TXAA ||
		r__aa == AA_TXAA2S ||
		r__aa == AA_TXAA4S)
		GFSDK_TXAA_DX11_ReleaseContext(&m_TXAA);
#endif

	_SHOW_REF				("DeviceREF:",HW.pDevice);
	_RELEASE				(HW.pDevice);

	DestroyD3D				();

#ifndef _EDITOR
	free_vid_mode_list		();
#endif
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset (HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC &cd = m_ChainDesc;

	BOOL	bWindowed		= !psDeviceFlags.is	(rsFullscreen);

	cd.Windowed = bWindowed;

	m_pSwapChain->SetFullscreenState(!bWindowed, NULL);

	DXGI_MODE_DESC	&desc = m_ChainDesc.BufferDesc;

	selectResolution(desc.Width, desc.Height, bWindowed);

	if (bWindowed)
	{
		desc.RefreshRate.Numerator = 60;
		desc.RefreshRate.Denominator = 1;
	}
	else
		desc.RefreshRate = selectRefresh( desc.Width, desc.Height, desc.Format);

	CHK_DX(m_pSwapChain->ResizeTarget(&desc));

	_SHOW_REF				("refCount:pBaseZB",	pBaseDepthReadWriteDSV);
	_SHOW_REF				("refCount:pBaseRT",	pBaseRT);

	_RELEASE(pBaseDepthReadWriteDSV);
	_RELEASE(pBaseDepthReadDSV);
	_RELEASE(pBaseDepthReadSRV);
	_RELEASE(pBaseRT);

	pBaseDepthSurface->Release();

	CHK_DX(m_pSwapChain->ResizeBuffers(
		cd.BufferCount,
		desc.Width,
		desc.Height,
		desc.Format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	UpdateViews();

	updateWindowProps	(hwnd);
}

void CHW::selectResolution( u32 &dwWidth, u32 &dwHeight, BOOL bWindowed )
{
	fill_vid_mode_list			(this);

	if(bWindowed)
	{
		dwWidth		= psCurrentVidMode[0];
		dwHeight	= psCurrentVidMode[1];
	}
	else //check
	{
		string64					buff;
		xr_sprintf					(buff,sizeof(buff),"%dx%d",psCurrentVidMode[0],psCurrentVidMode[1]);

		if(_ParseItem(buff,vid_mode_token)==u32(-1)) //not found
		{ //select safe
			xr_sprintf				(buff,sizeof(buff),"vid_mode %s",vid_mode_token[0].name);
			Console->Execute		(buff);
		}

		dwWidth						= psCurrentVidMode[0];
		dwHeight					= psCurrentVidMode[1];
	}
}

DXGI_RATIONAL CHW::selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt)
{
	DXGI_RATIONAL	res;

	res.Numerator = 60;
	res.Denominator = 1;

	float	CurrentFreq = 60.0f;

	if (psDeviceFlags.is(rsRefresh60hz))	
	{
		return res;
	}
	else
	{
		xr_vector<DXGI_MODE_DESC>	modes;

		IDXGIOutput *pOutput;
		m_pAdapter->EnumOutputs(0, &pOutput);
		VERIFY(pOutput);

		UINT num = 0;
		DXGI_FORMAT format = fmt;
		UINT flags         = 0;

		// Get the number of display modes available
		pOutput->GetDisplayModeList( format, flags, &num, 0);

		// Get the list of display modes
		modes.resize(num);
		pOutput->GetDisplayModeList( format, flags, &num, &modes.front());

		_RELEASE(pOutput);

		for (u32 i=0; i<num; ++i)
		{
			DXGI_MODE_DESC &desc = modes[i];

			if( (desc.Width == dwWidth) 
				&& (desc.Height == dwHeight)
				)
			{
				VERIFY(desc.RefreshRate.Denominator);
				float TempFreq = float(desc.RefreshRate.Numerator)/float(desc.RefreshRate.Denominator);
				if ( TempFreq > CurrentFreq )
				{
					CurrentFreq = TempFreq;
					res = desc.RefreshRate;
				}
			}
		}

		return res;
	}
}

void CHW::OnAppActivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		ShowWindow( m_ChainDesc.OutputWindow, SW_RESTORE );
		m_pSwapChain->SetFullscreenState( TRUE, NULL );
	}
}

void CHW::OnAppDeactivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		m_pSwapChain->SetFullscreenState( FALSE, NULL );
		ShowWindow( m_ChainDesc.OutputWindow, SW_MINIMIZE );
	}
}

void CHW::updateWindowProps(HWND m_hWnd)
{
	BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);

	u32		dwWindowStyle			= 0;

	// Set window properties depending on what mode were in.
	if (bWindowed)		{
		if (m_move_window) {
			if (psDeviceFlags.is(rsBorderless))
				SetWindowLong	( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_BORDER|WS_VISIBLE) );
			else
				SetWindowLong	( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_BORDER|WS_DLGFRAME|WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX ) );
			// When moving from fullscreen to windowed mode, it is important to
			// adjust the window size after recreating the device rather than
			// beforehand to ensure that you get the window size you want.  For
			// example, when switching from 640x480 fullscreen to windowed with
			// a 1000x600 window on a 1024x768 desktop, it is impossible to set
			// the window size to 1000x600 until after the display mode has
			// changed to 1024x768, because windows cannot be larger than the
			// desktop.

			RECT			m_rcWindowBounds;
			BOOL			bCenter = psDeviceFlags.is(rsCenterScreen);

			if (bCenter) {
				RECT				DesktopRect;

				GetClientRect		(GetDesktopWindow(), &DesktopRect);

				SetRect(			&m_rcWindowBounds, 
					(DesktopRect.right-m_ChainDesc.BufferDesc.Width)/2, 
					(DesktopRect.bottom-m_ChainDesc.BufferDesc.Height)/2, 
					(DesktopRect.right+m_ChainDesc.BufferDesc.Width)/2, 
					(DesktopRect.bottom+m_ChainDesc.BufferDesc.Height)/2);
			}else{
				SetRect(			&m_rcWindowBounds,
					0, 
					0, 
					m_ChainDesc.BufferDesc.Width, 
					m_ChainDesc.BufferDesc.Height);
			};

			AdjustWindowRect		(	&m_rcWindowBounds, dwWindowStyle, FALSE );

			SetWindowPos			(	m_hWnd, 
				HWND_NOTOPMOST,	
				m_rcWindowBounds.left, 
				m_rcWindowBounds.top,
				( m_rcWindowBounds.right - m_rcWindowBounds.left ),
				( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
				SWP_SHOWWINDOW|SWP_NOCOPYBITS|SWP_DRAWFRAME );
		}
	}
	else
	{
		SetWindowLong			( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_POPUP|WS_VISIBLE) );
	}

	ShowCursor	(FALSE);
	SetForegroundWindow( m_hWnd );
}


struct _uniq_mode
{
	_uniq_mode(LPCSTR v):_val(v){}
	LPCSTR _val;
	bool operator() (LPCSTR _other) {return !stricmp(_val,_other);}
};

#ifndef _EDITOR
void free_vid_mode_list()
{
	for( int i=0; vid_mode_token[i].name; i++ )
	{
		xr_free					(vid_mode_token[i].name);
	}
	xr_free						(vid_mode_token);
	vid_mode_token				= NULL;
}

void fill_vid_mode_list(CHW* _hw)
{
	if(vid_mode_token != NULL)		return;
	xr_vector<LPCSTR>	_tmp;
	xr_vector<DXGI_MODE_DESC>	modes;

	IDXGIOutput *pOutput;
	_hw->m_pAdapter->EnumOutputs(0, &pOutput);
	VERIFY(pOutput);

	UINT num = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT flags         = 0;

	// Get the number of display modes available
	pOutput->GetDisplayModeList( format, flags, &num, 0);

	// Get the list of display modes
	modes.resize(num);
	pOutput->GetDisplayModeList( format, flags, &num, &modes.front());

	_RELEASE(pOutput);

	for (u32 i=0; i<num; ++i)
	{
		DXGI_MODE_DESC &desc = modes[i];
		string32		str;

		if(desc.Width < 640)
			continue;

		xr_sprintf(str, sizeof(str), "%dx%d", desc.Width, desc.Height);

		if(_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
			continue;

		_tmp.push_back				(NULL);
		_tmp.back()					= xr_strdup(str);
	}
	
	u32 _cnt						= _tmp.size()+1;

	vid_mode_token					= xr_alloc<xr_token>(_cnt);

	vid_mode_token[_cnt-1].id			= -1;
	vid_mode_token[_cnt-1].name		= NULL;

#ifdef DEBUG
	Msg("Available video modes[%d]:",_tmp.size());
#endif // DEBUG
	for( u32 i=0; i<_tmp.size(); ++i )
	{
		vid_mode_token[i].id		= i;
		vid_mode_token[i].name		= _tmp[i];
#ifdef DEBUG
		Msg							("[%s]",_tmp[i]);
#endif // DEBUG
	}
}

void CHW::UpdateViews()
{
	{
		// Create a base render-target view
		ID3DTexture2D* pBuffer;
		R_CHK(m_pSwapChain->GetBuffer(0, __uuidof(ID3DTexture2D), (LPVOID*)&pBuffer));
		R_CHK(pDevice->CreateRenderTargetView(pBuffer, NULL, &pBaseRT));
		pBuffer->Release();
	}

	{
		//	Create a base depth-stencil view with a shader resource view
		D3D_TEXTURE2D_DESC descDepth = {};
		descDepth.Width = m_ChainDesc.BufferDesc.Width;
		descDepth.Height = m_ChainDesc.BufferDesc.Height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D_USAGE_DEFAULT;
		descDepth.BindFlags = D3D_BIND_DEPTH_STENCIL | D3D_BIND_SHADER_RESOURCE;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;
		R_CHK(pDevice->CreateTexture2D(&descDepth, NULL, &pBaseDepthSurface)); // write Texture2D

		// Depth-stencil view
		D3D_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Flags = 0;
		dsvDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		R_CHK(pDevice->CreateDepthStencilView(pBaseDepthSurface, &dsvDesc, &pBaseDepthReadWriteDSV)); // read & wtire DSV

		if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0)
		{
			dsvDesc.Flags = D3D_DSV_READ_ONLY_DEPTH; // We can write stencil
			R_CHK(pDevice->CreateDepthStencilView(pBaseDepthSurface, &dsvDesc, &pBaseDepthReadDSV)); // read DSV
		}
		else
		{
			pBaseDepthReadDSV = pBaseDepthReadWriteDSV; // Hack
		}

		// Shader resource view
		D3D_SHADER_RESOURCE_VIEW_DESC depthSRVDesc = {};
		depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		depthSRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		depthSRVDesc.Texture2D.MipLevels = 1;
		depthSRVDesc.Texture2D.MostDetailedMip = 0; // No MIP
		R_CHK(pDevice->CreateShaderResourceView(pBaseDepthSurface, &depthSRVDesc, &pBaseDepthReadSRV)); // read SRV
	}
}
#endif
