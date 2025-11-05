#include "stdafx.h"
#include "fstaticrender_rendertarget.h"
#include "../xrEngine/IGame_Persistent.h"
#include "../xrRender/xrRender_console.h"

static LPCSTR RTname = "$user$rendertarget";
static LPCSTR RTname_distort = "$user$distort";

CRenderTarget::CRenderTarget()
{
	bAvailable = FALSE;
	//RT = 0;
	//pTempZB = 0;
	//ZB = 0;

	param_blur = 0.f;
	param_gray = 0.f;
	param_noise = 0.f;
	param_duality_h = 0.f;
	param_duality_v = 0.f;
	param_noise_fps = 25.f;
	param_noise_scale = 1.f;

	im_noise_time = 1 / 100;
	im_noise_shift_w = 0;
	im_noise_shift_h = 0;

	param_color_base = color_rgba(127, 127, 127, 0);
	param_color_gray = color_rgba(85, 85, 85, 0);
	param_color_add = color_rgba(0, 0, 0, 0);

	bAvailable = Create();
	Msg("* SSample: %s", bAvailable ? "enabled" : "disabled");
}

BOOL CRenderTarget::Create()
{
	curWidth = Device.dwWidth;
	curHeight = Device.dwHeight;

	rtWidth = curWidth;
	rtHeight = curHeight;

	if (RImplementation.o.aa_type == SSAA)
	{
		rtWidth *= sqrt(RImplementation.o.ssaa_samples);
		rtHeight *= sqrt(RImplementation.o.ssaa_samples);
	}

	// SCREENSHOT
	{
		//D3DFORMAT format = psDeviceFlags.test(rsFullscreen) ? D3DFMT_A8R8G8B8 : HW.Caps.fTarget;
		//R_CHK(HW.pDevice->CreateOffscreenPlainSurface(Device.dwWidth, Device.dwHeight, format, D3DPOOL_SYSTEMMEM, &surf_screenshot_normal, NULL));

		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = Device.dwWidth;
		desc.Height = Device.dwHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Format = (DXGI_FORMAT)HW.Caps.fTarget;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		R_CHK(HW.pDevice11->CreateTexture2D(&desc, NULL, &tex_screenshot_normal));

		//R_CHK(HW.pDevice->CreateTexture(128, 128, 1, NULL, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &tex_screenshot_gamesave, NULL));
		//R_CHK(tex_screenshot_gamesave->GetSurfaceLevel(0, &surf_screenshot_gamesave));

		desc.Width = 128;
		desc.Height = 128;
		desc.Format = DXGI_FORMAT_BC1_UNORM;
		R_CHK(HW.pDevice11->CreateTexture2D(&desc, NULL, &tex_screenshot_gamesave));
	}
	
	// Bufferts
	//RT.create(RTname, rtWidth, rtHeight, HW.Caps.fTarget);
	//RT_distort.create(RTname_distort, rtWidth, rtHeight, HW.Caps.fTarget);
	RT.create(RTname, rtWidth, rtHeight, (DXGI_FORMAT)HW.Caps.fTarget, fSR | fRT);
	RT_distort.create(RTname_distort, rtWidth, rtHeight, DXGI_FORMAT_R8G8_UNORM, fSR | fRT); // .uv format

	//ZB.create("$user$depth", rtWidth, rtHeight, HW.Caps.fDepth);
	
#pragma message(Reminder("FIX MSAA/CSAA"))
	//if (RImplementation.o.aa_type == MSAA)
	//{
	//	R_CHK(HW.pDevice->CreateRenderTarget(rtWidth, rtHeight, HW.Caps.fTarget, 
	//		RImplementation.o.msaa_samples, RImplementation.o.csaa_samples, FALSE, &RT_msaa.rt, NULL));
	//	R_CHK(HW.pDevice->CreateDepthStencilSurface(rtWidth, rtHeight, HW.Caps.fDepth, 
	//		RImplementation.o.msaa_samples, RImplementation.o.csaa_samples, TRUE, &RT_msaa.zb, NULL));
	//}

#pragma message(Reminder("FIX LIGST SHADOWS"))
	// Temp ZB, used by some of the shadowing code
	//R_CHK(HW.pDevice->CreateDepthStencilSurface(512, 512, HW.Caps.fDepth, D3DMULTISAMPLE_NONE, 0, TRUE, &pTempZB, NULL));

	// Shaders and stream
	//s_postprocess.create("postprocess");
	s_postprocess_D.create("postprocess_d");
	g_postprocess.create(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX3, RCache.Vertex.Buffer(),
						 RCache.QuadIB);

	// Menu
	s_menu.create("distort");
	g_menu.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.QuadIB);

	//return RT->valid() && RT_distort->valid();
	return true;
}

CRenderTarget::~CRenderTarget()
{
	//_RELEASE(pTempZB);
	s_postprocess_D.destroy();
	//s_postprocess.destroy();
	g_postprocess.destroy();
	s_menu.destroy();
	g_menu.destroy();
	RT_distort.destroy();
	RT.destroy();
	_RELEASE(tex_screenshot_normal);
	_RELEASE(tex_screenshot_gamesave);
	//ZB.destroy();
	//if (RImplementation.o.aa_type == MSAA)
	//{
	//	_RELEASE(RT_msaa.rt);
	//	_RELEASE(RT_msaa.zb);
	//}
}

void CRenderTarget::EnableMSAA()
{
#pragma message(Reminder("FIX MSAA ENABLE"))
	//HW.pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
}

void CRenderTarget::DisableMSAA()
{
#pragma message(Reminder("FIX MSAA DISABLE"))
	//HW.pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
}

void CRenderTarget::EnableAlphaMSAA()
{
#pragma message(Reminder("FIX ATOC ENABLE"))
	//HW.pDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, 
	//	RImplementation.o.aa_type == MSAA ? (D3DFORMAT)ps_r1_aa_transluency : NULL);
}

void CRenderTarget::DisableAlphaMSAA()
{
#pragma message(Reminder("FIX ATOC DISABLE"))
	//HW.pDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, NULL);
}

void CRenderTarget::calc_tc_noise(Fvector2& p0, Fvector2& p1)
{
	//.	CTexture*	T					= RCache.get_ActiveTexture	(2);
	//.	VERIFY2		(T, "Texture #3 in noise shader should be setted up");
	//.	u32			tw					= iCeil(float(T->get_Width	())*param_noise_scale+EPS_S);
	//.	u32			th					= iCeil(float(T->get_Height ())*param_noise_scale+EPS_S);
	u32 tw = iCeil(256 * param_noise_scale + EPS_S);
	u32 th = iCeil(256 * param_noise_scale + EPS_S);
	VERIFY2(tw && th, "Noise scale can't be zero in any way");
	//.	if (bDebug)	Msg			("%d,%d,%f",tw,th,param_noise_scale);

	// calculate shift from FPSes
	im_noise_time -= Device.fTimeDelta;
	if (im_noise_time < 0)
	{
		im_noise_shift_w = ::Random.randI(tw ? tw : 1);
		im_noise_shift_h = ::Random.randI(th ? th : 1);
		float fps_time = 1 / param_noise_fps;
		while (im_noise_time < 0)
			im_noise_time += fps_time;
	}

	u32 shift_w = im_noise_shift_w;
	u32 shift_h = im_noise_shift_h;
	float start_u = (float(shift_w) + .5f) / (tw);
	float start_v = (float(shift_h) + .5f) / (th);
	u32 _w = Device.dwWidth;
	u32 _h = Device.dwHeight;
	u32 cnt_w = _w / tw;
	u32 cnt_h = _h / th;
	float end_u = start_u + float(cnt_w) + 1;
	float end_v = start_v + float(cnt_h) + 1;

	p0.set(start_u, start_v);
	p1.set(end_u, end_v);
}

void CRenderTarget::calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1)
{
	// Calculate ordinaty TCs from blur and SS
	float tw = float(rtWidth);
	float th = float(rtHeight);
	if (rtHeight != Device.dwHeight)
		param_blur = 1.f;
	Fvector2 shift, p0, p1;
	shift.set(.5f / tw, .5f / th);
	shift.mul(param_blur);
	p0.set(.5f / tw, .5f / th).add(shift);
	p1.set((tw + .5f) / tw, (th + .5f) / th).add(shift);

	// Calculate Duality TC
	float shift_u = param_duality_h * .5f;
	float shift_v = param_duality_v * .5f;

	r0.set(p0.x, p0.y);
	r1.set(p1.x - shift_u, p1.y - shift_v);
	l0.set(p0.x + shift_u, p0.y + shift_v);
	l1.set(p1.x, p1.y);
}

BOOL CRenderTarget::NeedPostProcess()
{
	bool _blur = (param_blur > 0.001f);
	bool _gray = (param_gray > 0.001f);
	bool _noise = (param_noise > 0.001f);
	bool _dual = (param_duality_h > 0.001f) || (param_duality_v > 0.001f);

	bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	bool _cbase = false;
	{
		int _r = color_get_R(param_color_base);
		_r = _abs(_r - int(0x7f));
		int _g = color_get_G(param_color_base);
		_g = _abs(_g - int(0x7f));
		int _b = color_get_B(param_color_base);
		_b = _abs(_b - int(0x7f));
		if (_r > 2 || _g > 2 || _b > 2)
			_cbase = true;
	}
	bool _cadd = false;
	{
		int _r = color_get_R(param_color_add);
		int _g = color_get_G(param_color_add);
		int _b = color_get_B(param_color_add);
		if (_r > 2 || _g > 2 || _b > 2)
			_cadd = true;
	}
	return _blur || _gray || _noise || _dual || _cbase || _cadd || _menu_pp;
}

#include <dinput.h>
#define SHOW(a) Log(#a, a);
#define SHOWX(a) Msg("%s %x", #a, a);
void CRenderTarget::Begin()
{
	//if (RImplementation.o.aa_type == MSAA)
	//{
	//	RCache.set_RT(RT_msaa.rt);
	//	RCache.set_ZB(RT_msaa.zb);
	//	EnableMSAA();
	//}
	//else
	//{
	//	RCache.set_RT(RT->pRT);
	//	RCache.set_ZB(ZB->pRT);
	//}

	SetRT(RT, 0, 0, 0, HW.pBaseDepthReadDSV, true);

	RCache.set_CullMode(D3D11_CULL_BACK);

	//Device.Clear();
}

struct TL_2c3uv
{
	Fvector4 p;
	u32 color0;
	u32 color1;
	Fvector2 uv[3];
	IC void set(float x, float y, u32 c0, u32 c1, float u0, float v0, float u1, float v1, float u2, float v2)
	{
		p.set(x, y, EPS_S, 1.f);
		color0 = c0;
		color1 = c1;
		uv[0].set(u0, v0);
		uv[1].set(u1, v1);
		uv[2].set(u2, v2);
	}
};

void CRenderTarget::End()
{
	//bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;
	//
	//BOOL bDistort = TRUE;
	//if ((0 == RImplementation.mapDistort.size()) && !_menu_pp)
	//	bDistort = FALSE;
	//
	//if (bDistort)
	//	phase_distortion();

	// render distort
	phase_distortion();

	// combination/postprocess
	//RCache.set_RT(HW.pBaseRT);
	//RCache.set_ZB(HW.pBaseZB);
	//curWidth = Device.dwWidth;
	//curHeight = Device.dwHeight;
	SetRT(Device.dwWidth, Device.dwHeight, HW.pBaseRT);

#pragma message(Reminder("FIX MSAA RESOLVE"))
	//if (RImplementation.o.aa_type == MSAA)
	//{
	//	HW.pDevice->StretchRect(RT_msaa.rt, NULL, RT->pRT, NULL, D3DTEXF_POINT);
	//	DisableMSAA();
	//}

	int gblend = clampr(iFloor((1 - param_gray) * 255.f), 0, 255);
	int nblend = clampr(iFloor((1 - param_noise) * 255.f), 0, 255);
	u32 p_color = subst_alpha(param_color_base, nblend);
	u32 p_gray = subst_alpha(param_color_gray, gblend);
	u32 p_brightness = param_color_add;

	// Draw full-screen quad textured with our scene image
	u32 Offset;
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);

	Fvector2 n0, n1, r0, r1, l0, l1;
	calc_tc_duality_ss(r0, r1, l0, l1);
	calc_tc_noise(n0, n1);

	// Fill vertex buffer
	float du = ps_pps_u, dv = ps_pps_v;
	TL_2c3uv* pv = (TL_2c3uv*)RCache.Vertex.Lock(4, g_postprocess.stride(), Offset);
	pv->set(du + 0, dv + float(_h), p_color, p_gray, r0.x, r1.y, l0.x, l1.y, n0.x, n1.y);
	pv++;
	pv->set(du + 0, dv + 0, p_color, p_gray, r0.x, r0.y, l0.x, l0.y, n0.x, n0.y);
	pv++;
	pv->set(du + float(_w), dv + float(_h), p_color, p_gray, r1.x, r1.y, l1.x, l1.y, n1.x, n1.y);
	pv++;
	pv->set(du + float(_w), dv + 0, p_color, p_gray, r1.x, r0.y, l1.x, l0.y, n1.x, n0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_postprocess.stride());

	//if (_menu_pp)
	//{
	//	RCache.set_Shader(s_menu);
	//}
	//else
	//{
	//	if (bDistort)
	//		RCache.set_Shader(s_postprocess_D);
	//	else
	//		RCache.set_Shader(s_postprocess);
	//}
	
	// Set shader
	RCache.set_Shader(s_postprocess_D);

	// Actual rendering
	static shared_str s_brightness = "c_brightness";
	RCache.set_c(s_brightness, color_get_R(p_brightness) / 255.f, color_get_G(p_brightness) / 255.f, color_get_B(p_brightness) / 255.f, 0);

	RCache.set_c("hdr_params", ps_r_hdr_white_intensity, ps_r_hdr_gamma, ps_r_hdr_brightness, 0);

	Fvector3 dof;
	Fvector2 vDofKernel;
	g_pGamePersistent->GetCurrentDof(dof);
	vDofKernel.set(0.5f / Device.dwWidth, 0.5f / Device.dwHeight);
	vDofKernel.mul(ps_r2_dof_kernel_size);

	RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r2_dof_sky);
	RCache.set_c("dof_kernel", vDofKernel.x, vDofKernel.y, ps_r2_dof_kernel_size, 0);

	RCache.set_Geometry(g_postprocess);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_distortion()
{
	//RCache.set_RT(RT_distort->pRT);
	//RCache.set_ZB(ZB->pRT);
	//RCache.set_CullMode(CULL_CCW);
	//RCache.set_ColorWriteEnable();
	//CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(127, 127, 127, 127), 1.0f, 0L));

	SetRT(RT_distort);
	ClearRT(RT_distort, 0.5f, 0.5f);
	RCache.set_CullMode(D3D11_CULL_BACK);
	RCache.set_ColorWriteEnable();
	
	if (g_pGameLevel && g_pGamePersistent && !g_pGamePersistent->OnRenderPPUI_query())
		RImplementation.r_dsgraph_render_distort();
	else
		RImplementation.mapDistort.clear();
	
	if (g_pGamePersistent)
		g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI
}
