#ifndef SH_RT_H
#define SH_RT_H
#pragma once

enum TextureBindFlags
{
	fSR = D3D11_BIND_SHADER_RESOURCE,	// ShaderResource
	fRT = D3D11_BIND_RENDER_TARGET,		// RenderTarget
	fDS = D3D11_BIND_DEPTH_STENCIL,		// DepthStencil
	fUA = D3D11_BIND_UNORDERED_ACCESS,	// UnorderedAcces

	// legacy formats compatible with DWM mod
	SRV_RTV_DSV = fSR | fRT | fDS, // Shader Resource & Render Target & Depth Stencil
	SRV_RTV_UAV = fSR | fRT | fUA, // Shader Resource & Render Target with Unordered Acces
	SRV_RTV		= fSR | fRT      , // Shader Resource & Render Target
	SRV_DSV		= fSR | fDS      , // Shader Resource & Depth Stencil
};

//////////////////////////////////////////////////////////////////////////
class ENGINE_API CRT : public xr_resource_named
{
  public:
	CRT();
	~CRT();

	void create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, u32 bind_flags, u32 samples);
	void destroy();
	void reset_begin();
	void reset_end();
	IC BOOL valid()
	{
		return !!pTexture;
	}

  public:
	ID3D11DepthStencilView* pZRT;
	ID3D11UnorderedAccessView* pUAView;
	DXGI_FORMAT format;
	u32 bind_flags;
	u32 samples;

	u32 dwWidth;
	u32 dwHeight;

	ID3D11Texture2D* pSurface;
	ID3D11RenderTargetView* pRT;
	ref_texture pTexture;

	u64 _order;
};
struct ENGINE_API resptrcode_crt : public resptr_base<CRT>
{
	// Depth Stencil view type required DXGI_FORMAT_D...
	void create(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, u32 bind_flags, u32 samples = 1);

	void destroy()
	{
		_set(NULL);
	}
};
typedef resptr_core<CRT, resptrcode_crt> ref_rt;


#endif // SH_RT_H
