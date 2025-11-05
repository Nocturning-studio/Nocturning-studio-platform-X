// HW.h: interface for the CHW class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
#define AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_
#pragma once

#include "hwcaps.h"

#ifndef _MAYA_EXPORT
#include "stats_manager.h"
#endif

class  CHW

	:	public pureAppActivate, 
		public pureAppDeactivate

{
//	Functions section
public:
	CHW();
	~CHW();

	void					CreateD3D				();
	void					DestroyD3D				();
	void					CreateDevice			(HWND hw, bool move_window);

	void					DestroyDevice			();

	void					Reset					(HWND hw);

	void					selectResolution		(u32 &dwWidth, u32 &dwHeight, BOOL bWindowed);
	void					updateWindowProps		(HWND hw);

	void	Validate(void)	{};

//	Variables section

public:
	IDXGIAdapter*			m_pAdapter;	//	pD3D equivalent
	IDXGISwapChain*         m_pSwapChain;

	ID3DDevice*				pDevice;
	ID3DDeviceContext*		pContext;
	
	ID3D11Device3*			pDevice3;
	ID3D11DeviceContext3*	pContext3;

	ID3DTexture2D* pBaseDepthSurface; // Base depth surface

	/*
		! Shader resource view with depth buffer
		Depth:
		- Write No
		- Test:	No
		- Read:	Yes
		Stencil: 
		- ...No
	*/
	ID3DShaderResourceView*	pBaseDepthReadSRV;

#ifdef __GFSDK_DX11__
	GFSDK_SSAO_Context_D3D* pSSAO;

	NvTxaaContextDX11 m_TXAA;
	bool m_TXAA_initialized;
#endif

	bool m_cs_support;
	bool m_12_level; // D3D_FEATURE_LEVEL_12_X

	CHWCaps						Caps;

	D3D_DRIVER_TYPE				m_DriverType;	//	DevT equivalent
	DXGI_SWAP_CHAIN_DESC		m_ChainDesc;	//	DevPP equivalent
	bool						m_bUsePerfhud;
	D3D_FEATURE_LEVEL			FeatureLevel;

	ID3DRenderTargetView* pBaseRT;	//	combine with DX9 pBaseRT via typedef

	/*
		! Depth-stencil view
		Depth:
		- Write Yes
		- Test:	Yes
		- Read:	No
		Stencil:
		- Write: Yes
		- Test:	Yes
		- Read:	No
	*/
	ID3DDepthStencilView* pBaseDepthReadWriteDSV;

	/*
		! Depth-stencil view
		Depth:
		- Write DX11: No, other: Yes
		- Test:	Yes
		- Read:	Yes (can be used with pBaseDepthReadSRV)
		Stencil:
		- Write: Yes
		- Test:	Yes
		- Read:	No
	*/
	ID3DDepthStencilView* pBaseDepthReadDSV;

#ifndef _MAYA_EXPORT
	stats_manager			stats_manager;
#endif


	void			UpdateViews();
	DXGI_RATIONAL	selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt);

	virtual	void	OnAppActivate();
	virtual void	OnAppDeactivate();


private:
	bool					m_move_window;
};

extern ECORE_API CHW		HW;

#endif // !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
