// BlenderDefault.cpp: implementation of the CBlender_LmEbB class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "blender_Lm(EbB).h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_LmEbB::CBlender_LmEbB()
{
	description.CLS = B_LmEbB;
	description.version = 0x1;
	strcpy(oT2_Name, "$null");
	strcpy(oT2_xform, "$null");
	oBlend.value = FALSE;
}

CBlender_LmEbB::~CBlender_LmEbB()
{
}

void CBlender_LmEbB::Save(IWriter& fs)
{
	description.version = 0x1;
	IBlender::Save(fs);
	xrPWRITE_MARKER(fs, "Environment map");
	xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
	xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
	xrPWRITE_PROP(fs, "Alpha-Blend", xrPID_BOOL, oBlend);
}

void CBlender_LmEbB::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);
	xrPREAD_MARKER(fs);
	xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
	xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
	if (version >= 0x1)
	{
		xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
	}
}
//////////////////////////////////////////////////////////////////////////
void CBlender_LmEbB::Compile(CBlender_Compile& C)
{
	if (oBlend.value)
		C.r_Pass("lmapE", "lmapE", TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, 0);
	else
		C.r_Pass("lmapE", "lmapE", TRUE);
	C.r_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
	C.r_Sampler("s_lmap", C.L_textures[1]);
	C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
	C.r_Sampler("s_env", oT2_Name, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
	C.r_End();
}

