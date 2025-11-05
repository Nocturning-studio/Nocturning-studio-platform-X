#ifndef SH_RT_H
#define SH_RT_H
#pragma once

//////////////////////////////////////////////////////////////////////////
enum VIEW_TYPE
{
	SRV_RTV,		// Shader Resource & Render Target
	SRV_RTV_UAV,	// Shader Resource & Render Target with Unordered Acces
	SRV_DSV,		// Shader Resource & Depth Stencil
	SRV_RTV_DSV,	// Shader Resource & Render Target & Depth Stencil
};
class		CRT		:	public xr_resource_named	
{
public:
	CRT();
	~CRT();

	void	create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, VIEW_TYPE view, u32 samples);
	void	destroy();
	void	reset_begin();
	void	reset_end();
	IC BOOL	valid()	{ return !!pTexture; }

public:
	ID3DTexture2D*				pSurface;
	ID3DRenderTargetView*		pRT;
	ID3DDepthStencilView*		pZRT;
	ID3DUnorderedAccessView*	pUAView;
	ref_texture					pTexture;

	u32							dwWidth;
	u32							dwHeight;
	DXGI_FORMAT					format;
	VIEW_TYPE					view;
	u32							samples;

	u64							_order;
};
struct 		resptrcode_crt	: public resptr_base<CRT>
{
// Depth Stencil view type required DXGI_FORMAT_D...
	void				create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, VIEW_TYPE view, u32 samples = 1);
	void				destroy			()	{ _set(NULL);		}
};
typedef	resptr_core<CRT,resptrcode_crt>		ref_rt;

#endif // SH_RT_H
