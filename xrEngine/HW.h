// HW.h: interface for the CHW class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
#define AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_
#pragma once

#include "hwcaps.h"

enum enum_stats_buffer_type
{
	enum_stats_buffer_type_vertex,
	enum_stats_buffer_type_index,
	enum_stats_buffer_type_rtarget,
	enum_stats_buffer_type_COUNT
};

class ENGINE_API stats_manager
{

  public:
	~stats_manager();
	void increment_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location);
	void decrement_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location);

	void increment_stats_rtarget(ID3D11Texture2D* buff);
	void increment_stats_vb(ID3D11Buffer* buff);
	void increment_stats_ib(ID3D11Buffer* buff);

	void decrement_stats_rtarget(ID3D11Texture2D* buff);
	void decrement_stats_vb(ID3D11Buffer* buff);
	void decrement_stats_ib(ID3D11Buffer* buff);

	u32 memory_usage_summary[enum_stats_buffer_type_COUNT][4];

  private:
	void increment_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location, void* buff_ptr);
	void decrement_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location, void* buff_ptr);

#ifdef DEBUG
	struct stats_item
	{
		void* buff_ptr;
		u32 size;
		enum_stats_buffer_type type;
		_D3DPOOL location;
	}; // stats_item

	xr_vector<stats_item> m_buffers_list;
#endif
}; // class stats_manager

u32 get_format_pixel_size(DXGI_FORMAT format);

class CHW

	: public pureAppActivate,
	  public pureAppDeactivate

{
	//	Functions section
  public:
	CHW();
	~CHW();

	void CreateD3D();
	void DestroyD3D();
	void CreateDevice(HWND hw, bool move_window);

	void DestroyDevice();

	void Reset(HWND hw);

	void selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed);
	void updateWindowProps(HWND hw);

	void Validate(void){};

	//	Variables section

  public:
	IDXGIAdapter* m_pAdapter; //	pD3D equivalent
	IDXGISwapChain* m_pSwapChain;

	ID3D11Device* pDevice11;
	ID3D11DeviceContext* pContext11;

#ifdef USE_DX11_3
	ID3D11Device3* pDevice11_3;
	ID3D11DeviceContext3* pContext11_3;
#endif

	ID3D11Texture2D* pBaseDepthSurface; // Base depth surface

	/*
		! Shader resource view with depth buffer
		Depth:
		- Write No
		- Test:	No
		- Read:	Yes
		Stencil:
		- ...No
	*/
	ID3D11ShaderResourceView* pBaseDepthReadSRV;

#ifdef __GFSDK_DX11__
	GFSDK_SSAO_Context_D3D* pSSAO;

	NvTxaaContextDX11 m_TXAA;
	bool m_TXAA_initialized;
#endif

	bool m_cs_support;
	bool m_12_level; // D3D_FEATURE_LEVEL_12_X

	CHWCaps Caps;

	D3D_DRIVER_TYPE m_DriverType;	  //	DevT equivalent
	DXGI_SWAP_CHAIN_DESC m_ChainDesc; //	DevPP equivalent
	bool m_bUsePerfhud;
	D3D_FEATURE_LEVEL FeatureLevel;

	ID3D11RenderTargetView* pBaseRT; //	combine with DX9 pBaseRT via typedef
	//ID3D11DepthStencilView* pBaseZB; // pBaseDepthReadWriteDSV
#define pBaseZB pBaseDepthReadWriteDSV

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
	ID3D11DepthStencilView* pBaseDepthReadWriteDSV;

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
	ID3D11DepthStencilView* pBaseDepthReadDSV;

#ifndef _MAYA_EXPORT
	stats_manager stats_manager;
#endif

	void UpdateViews();
	DXGI_RATIONAL selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt);

	virtual void OnAppActivate();
	virtual void OnAppDeactivate();

  private:
	bool m_move_window;
};

extern ENGINE_API CHW HW;

#endif // !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
