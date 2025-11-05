#pragma once

class CRenderTarget : public IRender_Target
{
  public:
	BOOL bAvailable;
	u32 rtWidth;
	u32 rtHeight;

	u32 curWidth;
	u32 curHeight;

	ref_rt RT;
	ref_rt RT_distort;
	//ref_rt ZB;

	//ref_shader s_postprocess;
	ref_shader s_postprocess_D;
	ref_geom g_postprocess;

	ref_shader s_menu;
	ref_geom g_menu;

	float im_noise_time;
	u32 im_noise_shift_w;
	u32 im_noise_shift_h;

	float param_blur;
	float param_gray;
	float param_duality_h;
	float param_duality_v;
	float param_noise;
	float param_noise_scale;
	float param_noise_fps;

	u32 param_color_base;
	u32 param_color_gray;
	u32 param_color_add;

  public:
	//IDirect3DSurface9* pTempZB;

	ID3D11Texture2D* tex_screenshot_normal;
	ID3D11Texture2D* tex_screenshot_gamesave;
	//IDirect3DSurface9* surf_screenshot_normal;	 // HW.fTarget, SM_NORMAL
	//IDirect3DTexture9* tex_screenshot_gamesave;	 // Container of "surf_screenshot_gamesave"
	//IDirect3DSurface9* surf_screenshot_gamesave; // DXT1, SM_FOR_GAMESAVE

	//struct
	//{
	//	IDirect3DSurface9* rt;
	//	IDirect3DSurface9* zb;
	//} RT_msaa;

	void EnableMSAA();
	void DisableMSAA();
	void EnableAlphaMSAA();
	void DisableAlphaMSAA();

  private:
	BOOL Create();
	BOOL NeedPostProcess();
	BOOL Available()
	{
		return bAvailable;
	}

	void calc_tc_noise(Fvector2& p0, Fvector2& p1);
	void calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);
	void phase_distortion();

  public:
	CRenderTarget();
	~CRenderTarget();

	void Begin();
	void End();

	IC void SetRT(const ref_rt& rt0 = 0, const ref_rt& rt1 = 0, const ref_rt& rt2 = 0, const ref_rt& rt3 = 0, 
		ID3D11DepthStencilView* zb = 0, bool clearZB = false)
	{
		if (rt0)
		{
			curWidth = rt0->dwWidth;	
			curHeight = rt0->dwHeight;
		}

		static ID3D11RenderTargetView* rts[4];
		rts[0] = rt0 ? rt0->pRT : 0;
		rts[1] = rt1 ? rt1->pRT : 0;
		rts[2] = rt2 ? rt2->pRT : 0;
		rts[3] = rt3 ? rt3->pRT : 0;

		HW.pContext11->OMSetRenderTargets(4, rts, zb);

		if (clearZB)
			ClearZB(zb, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL);
	}
	
	IC void SetRT(u32 w, u32 h, ID3D11RenderTargetView* rt0, ID3D11RenderTargetView* rt1 = 0, ID3D11RenderTargetView* rt2 = 0, 
		ID3D11RenderTargetView* rt3 = 0, ID3D11DepthStencilView* zb = 0, bool clearZB = false)
	{
		curWidth = w;
		curHeight = h;

		static ID3D11RenderTargetView* rts[4];
		rts[0] = rt0;
		rts[1] = rt1;
		rts[2] = rt2;
		rts[3] = rt3;

		HW.pContext11->OMSetRenderTargets(4, rts, zb);

		if (clearZB)
			ClearZB(zb, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL);
	}

	IC void ClearRT(const ref_rt& rt0, float r = 0, float g = 0, float b = 0, float a = 0)
	{
		static float color[4];
		color[0] = r;
		color[1] = g;
		color[2] = b;
		color[3] = a;
		HW.pContext11->ClearRenderTargetView(rt0->pRT, color);
	}
	
	IC void ClearRT(ID3D11RenderTargetView* rt0, float r = 0, float g = 0, float b = 0, float a = 0)
	{
		static float color[4];
		color[0] = r;
		color[1] = g;
		color[2] = b;
		color[3] = a;
		HW.pContext11->ClearRenderTargetView(rt0, color);
	}

	IC void ClearZB(ID3D11DepthStencilView* zb, u32 flags, float depth = 1, float stencil = 0)
	{
		HW.pContext11->ClearDepthStencilView(zb, flags, depth, stencil);
	}

	virtual void set_blur(float f)
	{
		param_blur = f;
	}
	virtual void set_gray(float f)
	{
		param_gray = f;
	}
	virtual void set_duality_h(float f)
	{
		param_duality_h = _abs(f);
	}
	virtual void set_duality_v(float f)
	{
		param_duality_v = _abs(f);
	}
	virtual void set_noise(float f)
	{
		param_noise = f;
	}
	virtual void set_noise_scale(float f)
	{
		param_noise_scale = f;
	}
	virtual void set_noise_fps(float f)
	{
		param_noise_fps = _abs(f) + EPS_S;
	}

	virtual void set_color_base(u32 f)
	{
		param_color_base = f;
	}
	virtual void set_color_gray(u32 f)
	{
		param_color_gray = f;
	}
	virtual void set_color_add(u32 f)
	{
		param_color_add = f;
	}

	virtual u32 get_width()
	{
		return curWidth;
	}
	virtual u32 get_height()
	{
		return curHeight;
	}
};
