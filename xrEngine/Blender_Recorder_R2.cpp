#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "blenders/Blender_Recorder.h"
#include "blenders/Blender.h"

#include "tss.h"

void fix_texture_name(LPSTR fn)
{
	LPSTR _ext = strext(fn);
	if (_ext && (0 == stricmp(_ext, ".tga") || 0 == stricmp(_ext, ".dds") || 0 == stricmp(_ext, ".bmp") ||
				 0 == stricmp(_ext, ".ogm")))
		*_ext = 0;
}

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _ps, bool bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
							  D3DBLEND abSRC, D3DBLEND abDST, BOOL aTest, u32 aRef)
{
	RS.Invalidate();
	ctable.clear();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
	dwStage = 0;
	g_shader_compiled = false;

	// Setup FF-units (Z-buffer, blender)
	PassSET_ZB(bZtest, bZwrite);
	PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
	PassSET_LightFog(FALSE, bFog);

	// Create shaders
	dest.ps = Device.Resources->CreateShader<SPS>(_ps, macros);
	dest.vs = Device.Resources->CreateShader<SVS>(_vs, macros);
	macros.clear();

	dest.gs = Device.Resources->CreateShader<SGS>("null", macros);
	dest.hs = Device.Resources->CreateShader<SHS>("null", macros);
	dest.ds = Device.Resources->CreateShader<SDS>("null", macros);
	dest.cs = Device.Resources->CreateShader<SCS>("null", macros);

	ctable.merge(&dest.ps->constants);
	ctable.merge(&dest.vs->constants);

	// Last Stage - disable
	if (0 == stricmp(_ps, "null"))
	{
		RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void CBlender_Compile::r_Constant(LPCSTR name, R_constant_setup* s)
{
	R_ASSERT(s);
	ref_constant C = ctable.get(name);
	if (C)
		C->handler = s;
}

void CBlender_Compile::r_ColorWriteEnable(bool cR, bool cG, bool cB, bool cA)
{
	BYTE Mask = 0;
	Mask |= cR ? D3DCOLORWRITEENABLE_RED : 0;
	Mask |= cG ? D3DCOLORWRITEENABLE_GREEN : 0;
	Mask |= cB ? D3DCOLORWRITEENABLE_BLUE : 0;
	Mask |= cA ? D3DCOLORWRITEENABLE_ALPHA : 0;

	RS.SetRS(D3DRS_COLORWRITEENABLE, Mask);
	RS.SetRS(D3DRS_COLORWRITEENABLE1, Mask);
	RS.SetRS(D3DRS_COLORWRITEENABLE2, Mask);
	RS.SetRS(D3DRS_COLORWRITEENABLE3, Mask);
}

void CBlender_Compile::r_Stencil(BOOL Enable, u32 Func, u32 Mask, u32 WriteMask, u32 Fail, u32 Pass, u32 ZFail)
{
	RS.SetRS(D3DRS_STENCILENABLE, BC(Enable));
	if (!Enable)
		return;
	RS.SetRS(D3DRS_STENCILFUNC, Func);
	RS.SetRS(D3DRS_STENCILMASK, Mask);
	RS.SetRS(D3DRS_STENCILWRITEMASK, WriteMask);
	RS.SetRS(D3DRS_STENCILFAIL, Fail);
	RS.SetRS(D3DRS_STENCILPASS, Pass);
	RS.SetRS(D3DRS_STENCILZFAIL, ZFail);
	//	Since we never really support different options for
	//	CW/CCW stencil use it to mimic DX9 behaviour for
	//	single-sided stencil
	RS.SetRS(D3DRS_CCW_STENCILFUNC, Func);
	RS.SetRS(D3DRS_CCW_STENCILFAIL, Fail);
	RS.SetRS(D3DRS_CCW_STENCILPASS, Pass);
	RS.SetRS(D3DRS_CCW_STENCILZFAIL, ZFail);
}

void CBlender_Compile::r_StencilRef(u32 Ref)
{
	RS.SetRS(D3DRS_STENCILREF, Ref);
}

void CBlender_Compile::r_CullMode(D3DCULL Mode)
{
	RS.SetRS(D3DRS_CULLMODE, (u32)Mode);
}

void CBlender_Compile::r_dx10Texture(LPCSTR ResourceName, LPCSTR texture)
{
	VERIFY(ResourceName);
	if (!texture)
		return;
	//
	string256 TexName;
	strcpy_s(TexName, texture);
	fix_texture_name(TexName);

	// Find index
	ref_constant C = ctable.get(ResourceName);
	// VERIFY(C);
	if (!C)
		return;

	R_ASSERT(C->type == RC_dx10texture);
	u32 stage = C->samp.index;

	// debug
	if (g_shader_compiled)
	{
		Msg("*   - set tex: %d, %s (%s)", stage, ResourceName, texture);
	}

	passTextures.push_back(mk_pair(stage, ref_texture(Device.Resources->_CreateTexture(TexName))));
}

void CBlender_Compile::i_dx10Address(u32 s, u32 address)
{
	// VERIFY(s!=u32(-1));
	if (s == u32(-1))
	{
		Msg("s != u32(-1)");
	}
	RS.SetSAMP(s, D3DSAMP_ADDRESSU, address);
	RS.SetSAMP(s, D3DSAMP_ADDRESSV, address);
	RS.SetSAMP(s, D3DSAMP_ADDRESSW, address);
}

void CBlender_Compile::i_dx10BorderColor(u32 s, u32 color)
{
	RS.SetSAMP(s, D3DSAMP_BORDERCOLOR, color);
}
void CBlender_Compile::i_dx10Filter_Min(u32 s, u32 f)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, D3DSAMP_MINFILTER, f);
}

void CBlender_Compile::i_dx10Filter_Mip(u32 s, u32 f)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, D3DSAMP_MIPFILTER, f);
}

void CBlender_Compile::i_dx10Filter_Mag(u32 s, u32 f)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, D3DSAMP_MAGFILTER, f);
}

void CBlender_Compile::i_dx10FilterAniso(u32 s, BOOL value)
{
	VERIFY(s != u32(-1));
	RS.SetSAMP(s, XRDX10SAMP_ANISOTROPICFILTER, value);
}

void CBlender_Compile::i_dx10Filter(u32 s, u32 _min, u32 _mip, u32 _mag)
{
	VERIFY(s != u32(-1));
	i_dx10Filter_Min(s, _min);
	i_dx10Filter_Mip(s, _mip);
	i_dx10Filter_Mag(s, _mag);
}

u32 CBlender_Compile::r_dx10Sampler(LPCSTR ResourceName)
{
	//	TEST
	// return ((u32)-1);
	VERIFY(ResourceName);
	string256 name;
	strcpy_s(name, ResourceName);
	fix_texture_name(name);

	// Find index
	// ref_constant C			= ctable.get(ResourceName);
	ref_constant C = ctable.get(name);
	// VERIFY(C);
	if (!C)
		return u32(-1);

	R_ASSERT(C->type == RC_sampler);
	u32 stage = C->samp.index;

	//	init defaults here

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT
	if (0 == xr_strcmp(ResourceName, "smp_nofilter"))
	{
		i_dx10Address(stage, D3D11_TEXTURE_ADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	}

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
	if (0 == xr_strcmp(ResourceName, "smp_rtlinear"))
	{
		i_dx10Address(stage, D3D11_TEXTURE_ADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
	}

	//	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
	if (0 == xr_strcmp(ResourceName, "smp_linear"))
	{
		i_dx10Address(stage, D3DTADDRESS_WRAP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
	}

	//	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC
	if (0 == xr_strcmp(ResourceName, "smp_base"))
	{
		i_dx10Address(stage, D3DTADDRESS_WRAP);
		i_dx10FilterAniso(stage, TRUE);
		// i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
	}

	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
	if (0 == xr_strcmp(ResourceName, "smp_material"))
	{
		i_dx10Address(stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
		RS.SetSAMP(stage, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
	}

	if (0 == xr_strcmp(ResourceName, "smp_smap"))
	{
		i_dx10Address(stage, D3DTADDRESS_CLAMP);
		i_dx10Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
		RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFILTER, TRUE);
		RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFUNC, D3D11_COMPARISON_LESS_EQUAL);
	}

	if (0 == xr_strcmp(ResourceName, "smp_jitter"))
	{
		i_dx10Address(stage, D3DTADDRESS_WRAP);
		i_dx10Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	}

	// debug
	if (g_shader_compiled)
	{
		Msg("*   - set smp: %d, %s", stage, ResourceName);
	}

	return stage;
}

void CBlender_Compile::r_End()
{
	SetMapping();
	dest.constants = Device.Resources->_CreateConstantTable(ctable);
	dest.state = Device.Resources->_CreateState(RS.GetContainer());
	dest.T = Device.Resources->_CreateTextureList(passTextures);
	dest.C = 0;
	ref_matrix_list temp(0);
	SH->passes.push_back(Device.Resources->_CreatePass(dest));
	// SH->passes.push_back	(DEV->_CreatePass(dest.state,dest.ps,dest.vs,dest.gs,dest.constants,dest.T,temp,dest.C));
}

// DX11 HACK!!!!

u32 CBlender_Compile::r_Sampler(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide, u32 address, u32 fmin,
								u32 fmip, u32 fmag)
{
#pragma message(Reminder("Rewrite me with r_dx10Texture please :("))
	r_dx10Texture(name, texture);
	// set all samplers
	r_dx10Sampler("smp_nofilter");
	r_dx10Sampler("smp_rtlinear");
	r_dx10Sampler("smp_linear");
	// hack... return smp_base id
	u32 i = i = r_dx10Sampler("smp_base");
	return i;
}

void CBlender_Compile::r_Sampler_rtf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
#pragma message(Reminder("Rewrite me with r_dx10Texture please :("))
	r_dx10Texture(name, texture);
	// point or bilienar
	r_dx10Sampler("smp_nofilter");
	r_dx10Sampler("smp_rtlinear");
}

void CBlender_Compile::r_Sampler_clf(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
#pragma message(Reminder("Rewrite me with r_dx10Texture please :("))
	r_dx10Texture(name, texture);
	// bilinear
	r_dx10Sampler("smp_rtlinear");
}

void CBlender_Compile::r_Sampler_clw(LPCSTR name, LPCSTR texture, bool b_ps1x_ProjectiveDivide)
{
#pragma message(Reminder("Rewrite me with r_dx10Texture please :("))
	r_dx10Texture(name, texture);
	// trilinear
	r_dx10Sampler("smp_linear");
}

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _ps, BOOL bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
							  D3D_BLEND abSRC, D3D_BLEND abDST, BOOL aTest, u32 aRef)
{
	RS.Invalidate();
	ctable.clear();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
	dwStage = 0;

	// Setup FF-units (Z-buffer, blender)
	PassSET_ZB(bZtest, bZwrite);
	PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
	PassSET_LightFog(FALSE, bFog);

	// Create shaders
	dest.ps = Device.Resources->CreateShader<SPS>(_ps, macros);
	dest.vs = Device.Resources->CreateShader<SVS>(_vs, macros);
	macros.clear();

	dest.gs = Device.Resources->CreateShader<SGS>("null", macros);
	dest.hs = Device.Resources->CreateShader<SHS>("null", macros);
	dest.ds = Device.Resources->CreateShader<SDS>("null", macros);
	dest.cs = Device.Resources->CreateShader<SCS>("null", macros);

	ctable.merge(&dest.ps->constants);
	ctable.merge(&dest.vs->constants);

	// Last Stage - disable
	if (0 == stricmp(_ps, "null"))
	{
		RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void CBlender_Compile::r_Pass(LPCSTR _vs, LPCSTR _gs, LPCSTR _ps, BOOL bFog, BOOL bZtest, BOOL bZwrite, BOOL bABlend,
							  D3D_BLEND abSRC, D3D_BLEND abDST, BOOL aTest, u32 aRef)
{
	RS.Invalidate();
	ctable.clear();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
	dwStage = 0;

	// Setup FF-units (Z-buffer, blender)
	PassSET_ZB(bZtest, bZwrite);
	PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
	PassSET_LightFog(FALSE, bFog);

	// Create shaders
	dest.ps = Device.Resources->CreateShader<SPS>(_ps, macros);
	dest.vs = Device.Resources->CreateShader<SVS>(_vs, macros);
	dest.gs = Device.Resources->CreateShader<SGS>(_gs, macros);
	macros.clear();

	dest.hs = Device.Resources->CreateShader<SHS>("null", macros);
	dest.ds = Device.Resources->CreateShader<SDS>("null", macros);
	dest.cs = Device.Resources->CreateShader<SCS>("null", macros);

	ctable.merge(&dest.ps->constants);
	ctable.merge(&dest.vs->constants);
	ctable.merge(&dest.gs->constants);

	// Last Stage - disable
	if (0 == stricmp(_ps, "null"))
	{
		RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void CBlender_Compile::r_TessPass(LPCSTR vs, LPCSTR hs, LPCSTR ds, LPCSTR gs, LPCSTR ps, BOOL bFog, BOOL bZtest,
								  BOOL bZwrite, BOOL bABlend, D3D_BLEND abSRC, D3D_BLEND abDST, BOOL aTest, u32 aRef)
{
	RS.Invalidate();
	ctable.clear();
	passTextures.clear();
	passMatrices.clear();
	passConstants.clear();
	dwStage = 0;

	// Setup FF-units (Z-buffer, blender)
	PassSET_ZB(bZtest, bZwrite);
	PassSET_Blend(bABlend, abSRC, abDST, aTest, aRef);
	PassSET_LightFog(FALSE, bFog);

	// Create shaders
	dest.ps = Device.Resources->CreateShader<SPS>(ps, macros);
	dest.vs = Device.Resources->CreateShader<SVS>(vs, macros);
	dest.gs = Device.Resources->CreateShader<SGS>(gs, macros);
	dest.hs = Device.Resources->CreateShader<SHS>(hs, macros);
	dest.ds = Device.Resources->CreateShader<SDS>(ds, macros);
	macros.clear();

	dest.cs = Device.Resources->CreateShader<SCS>("null", macros);

	ctable.merge(&dest.ps->constants);
	ctable.merge(&dest.vs->constants);
	ctable.merge(&dest.gs->constants);
	ctable.merge(&dest.hs->constants);
	ctable.merge(&dest.ds->constants);

	// Last Stage - disable
	if (0 == stricmp(ps, "null"))
	{
		RS.SetTSS(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RS.SetTSS(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void CBlender_Compile::r_ComputePass(LPCSTR cs)
{
	dest.cs = Device.Resources->CreateShader<SCS>(cs, macros);
	ctable.merge(&dest.cs->constants);
}

void CBlender_Compile::r_CullMode(D3D11_CULL_MODE Mode)
{
	RS.SetRS(D3DRS_CULLMODE, (u32)Mode);
}
