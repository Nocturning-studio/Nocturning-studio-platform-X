// BlenderDefault.cpp: implementation of the CBlender_BmmD class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "blender_BmmD.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

string64 oR_Name_hack, oG_Name_hack, oB_Name_hack, oA_Name_hack; // HACK !!!!! FIX ME !!!!!

CBlender_BmmD::CBlender_BmmD	()
{
	description.CLS		= B_BmmD;
	xr_strcpy				(oT2_Name,	"$null");
	xr_strcpy				(oT2_xform,	"$null");
	description.version	= 3;
	xr_strcpy				(oR_Name,	"detail\\detail_grnd_grass");	//"$null");
	xr_strcpy				(oG_Name,	"detail\\detail_grnd_asphalt");	//"$null");
	xr_strcpy				(oB_Name,	"detail\\detail_grnd_earth");	//"$null");
	xr_strcpy				(oA_Name,	"detail\\detail_grnd_yantar");	//"$null");

	//oTessellation.Count = 4;
	//oTessellation.IDselected = 0;
}

CBlender_BmmD::~CBlender_BmmD	()
{
}

void	CBlender_BmmD::Save		(IWriter& fs )
{
	IBlender::Save	(fs);
	xrPWRITE_MARKER	(fs,"Detail map");
	xrPWRITE_PROP	(fs,"Name",				xrPID_TEXTURE,	oT2_Name);
	xrPWRITE_PROP	(fs,"Transform",		xrPID_MATRIX,	oT2_xform);
	xrPWRITE_PROP	(fs,"R2-R",				xrPID_TEXTURE,	oR_Name);
	xrPWRITE_PROP	(fs,"R2-G",				xrPID_TEXTURE,	oG_Name);
	xrPWRITE_PROP	(fs,"R2-B",				xrPID_TEXTURE,	oB_Name);
	xrPWRITE_PROP	(fs,"R2-A",				xrPID_TEXTURE,	oA_Name);

	/*xrP_TOKEN::Item	I;
	xrPWRITE_PROP(fs, "Tessellation", xrPID_TOKEN, oTessellation);
	I.ID = 0; xr_strcpy(I.str, "NO_TESS");	fs.w(&I, sizeof(I));
	I.ID = 1; xr_strcpy(I.str, "TESS_PN");	fs.w(&I, sizeof(I));
	I.ID = 2; xr_strcpy(I.str, "TESS_HM");	fs.w(&I, sizeof(I));
	I.ID = 3; xr_strcpy(I.str, "TESS_PN+HM");	fs.w(&I, sizeof(I));*/
}

void	CBlender_BmmD::Load		(IReader& fs, u16 version )
{
	IBlender::Load	(fs,version);
	if (version<3)	{
		xrPREAD_MARKER	(fs);
		xrPREAD_PROP	(fs,xrPID_TEXTURE,	oT2_Name);
		xrPREAD_PROP	(fs,xrPID_MATRIX,	oT2_xform);
	} else {
		xrPREAD_MARKER	(fs);
		xrPREAD_PROP	(fs,xrPID_TEXTURE,	oT2_Name);
		xrPREAD_PROP	(fs,xrPID_MATRIX,	oT2_xform);
		xrPREAD_PROP	(fs,xrPID_TEXTURE,	oR_Name);
		xrPREAD_PROP	(fs,xrPID_TEXTURE,	oG_Name);
		xrPREAD_PROP	(fs,xrPID_TEXTURE,	oB_Name);
		xrPREAD_PROP	(fs, xrPID_TEXTURE, oA_Name);
		//xrPREAD_PROP	(fs, xrPID_TOKEN,	oTessellation);
	}
}

#include "uber_deffer.h"

void	CBlender_BmmD::Compile	(CBlender_Compile& C)
{
	IBlender::Compile		(C);

	// codepath is the same, only the shaders differ
	// ***only pixel shaders differ***
	string256				mask;
	strconcat				(sizeof(mask),mask,C.L_textures[0].c_str(),"_mask");

	switch (C.iElement)
	{
	case SE_NORMAL_HQ: 		// deffer & forward
		xr_strcpy(oR_Name_hack, oR_Name);
		xr_strcpy(oG_Name_hack, oG_Name);
		xr_strcpy(oB_Name_hack, oB_Name);
		xr_strcpy(oA_Name_hack, oA_Name);
		//C.TessMethod = CBlender_Compile::TESS_HM;
		uber(C, true, "impl", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);
		C.r_dx10Texture("s_mask", mask);
		C.r_dx10Texture("s_lmap", C.L_textures[1]);
		C.r_dx10Texture("s_dt_r", oR_Name);
		C.r_dx10Texture("s_dt_g", oG_Name);
		C.r_dx10Texture("s_dt_b", oB_Name);
		C.r_dx10Texture("s_dt_a", oA_Name);
		C.r_dx10Texture("s_dn_r", strconcat(sizeof(mask), mask, oR_Name, "_bump"));
		C.r_dx10Texture("s_dn_g", strconcat(sizeof(mask), mask, oG_Name, "_bump"));
		C.r_dx10Texture("s_dn_b", strconcat(sizeof(mask), mask, oB_Name, "_bump"));
		C.r_dx10Texture("s_dn_a", strconcat(sizeof(mask), mask, oA_Name, "_bump"));
		C.r_dx10Texture("s_dx_r", strconcat(sizeof(mask), mask, oR_Name, "_bump#"));
		C.r_dx10Texture("s_dx_g", strconcat(sizeof(mask), mask, oG_Name, "_bump#"));
		C.r_dx10Texture("s_dx_b", strconcat(sizeof(mask), mask, oB_Name, "_bump#"));
		C.r_dx10Texture("s_dx_a", strconcat(sizeof(mask), mask, oA_Name, "_bump#"));
		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_linear");
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		//C.r_CullMode(D3D_CULL_NONE); // на будущее для автогенерации травы
		C.r_End();
		break;
	case SE_NORMAL_LQ: 		// deffer
		uber(C, false, "impl", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);
		C.r_dx10Texture("s_lmap", C.L_textures[1]);
		C.r_dx10Sampler("smp_linear");
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		C.r_End();
		break;
	case SE_SHADOW:			// smap
		C.r_Pass("shadow_direct_base", "shadow_direct_base", FALSE, TRUE, TRUE, FALSE);
		C.r_dx10Texture("s_base", C.L_textures[0]);
		C.r_dx10Sampler("smp_base");
		C.r_ColorWriteEnable(RImplementation.o.vsm, RImplementation.o.vsm, false, false);
		C.r_End();
		break;

	case SE_RSM_FILL_RTS:		// RSM
		uber_rsm(C, "base");
		break;
	case SE_ZPREPASS:		// zprepass
		uber_zprepass(C, "base_multisampled");
		break;
	case SE_PLANAR:
		uber_planar(C, "base");
		break;

	}
}