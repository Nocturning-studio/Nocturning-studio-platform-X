#include "stdafx.h"
#pragma hdrstop

#include "blender_deffer_aref.h"
#include "uber_deffer.h"

CBlender_deffer_aref::CBlender_deffer_aref	(bool _lmapped) : lmapped(_lmapped)	{	
	description.CLS		= B_DEFAULT_AREF;
	oAREF.value			= 200;
	oAREF.min			= 0;
	oAREF.max			= 255;
	oBlend.value		= FALSE;
	description.version	= 1;
}
CBlender_deffer_aref::~CBlender_deffer_aref	()	{	}

void	CBlender_deffer_aref::Save	(	IWriter& fs )
{
	IBlender::Save	(fs);
	xrPWRITE_PROP	(fs,"Alpha ref",	xrPID_INTEGER,	oAREF);
	xrPWRITE_PROP	(fs,"Alpha-blend",	xrPID_BOOL,		oBlend);
}
void	CBlender_deffer_aref::Load	(	IReader& fs, u16 version )
{
	IBlender::Load	(fs,version);
	if (1==version)	{
		xrPREAD_PROP	(fs,xrPID_INTEGER,	oAREF);
		xrPREAD_PROP	(fs,xrPID_BOOL,		oBlend);
	}
}

void	CBlender_deffer_aref::Compile(CBlender_Compile& C)
{
	IBlender::Compile		(C);

	// oBlend.value	= FALSE	;

	if (oBlend.value)	
	{
		switch(C.iElement) 
		{
		case 0:
		case 1:
			if (lmapped)
			{
				C.r_Pass("lmapE", "lmapE", TRUE, TRUE, TRUE, TRUE, D3D_BLEND_SRC_ALPHA, D3D_BLEND_INV_SRC_ALPHA, TRUE, oAREF.value);
				C.r_dx10Texture("s_base", C.L_textures[0]);
				C.r_dx10Texture("s_lmap", C.L_textures[1]);
				C.r_dx10Texture("s_hemi", C.L_textures[2]);
				C.r_dx10Texture("s_env", tex_t_envmap_0);
				C.r_dx10Sampler("smp_linear");
				C.r_dx10Sampler("smp_rtlinear");
				C.r_dx10Sampler("smp_base");
				C.r_End();
			}
			else
			{
				C.r_Pass("vert", "vert", TRUE, TRUE, TRUE, TRUE, D3D_BLEND_SRC_ALPHA, D3D_BLEND_INV_SRC_ALPHA, TRUE, oAREF.value);
				C.r_dx10Texture("s_base", C.L_textures[0]);
				C.r_dx10Sampler("smp_base");
				C.r_End();
			}
			break;
		case 2:		// smap
			C.r_Pass("shadow_direct_base_aref", "shadow_direct_base_aref", FALSE, TRUE, TRUE, FALSE);
			C.r_dx10Texture("s_base", C.L_textures[0]);
			C.r_dx10Sampler("smp_base");
			C.r_ColorWriteEnable(false, false, false, false);
			C.r_End();
			break;
		default:
			break;
		}
	} 
	else 
	{
		C.SetParams(1, false);	//.
		// codepath is the same, only the shaders differ
		// ***only pixel shaders differ***
		switch (C.iElement)
		{
		case SE_NORMAL_HQ: 	// deffer
			uber(C, true, "base", "base", true, 0, true);
			C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			C.r_StencilRef(0x01);
			C.r_End();
			break;
		case SE_NORMAL_LQ: 	// deffer
			uber(C, false, "base", "base", true, 0, true);
			C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			C.r_StencilRef(0x01);
			C.r_End();
			break;
		case SE_SHADOW:		// smap
			C.r_Pass("shadow_direct_base_aref", "shadow_direct_base_aref", FALSE, TRUE, TRUE, FALSE);
			C.r_dx10Texture("s_base", C.L_textures[0]);
			C.r_dx10Sampler("smp_base");
			C.r_ColorWriteEnable(RImplementation.o.vsm, RImplementation.o.vsm, false, false);
			C.r_End();
			break;

		case SE_RSM_FILL_RTS:	// RSM
			uber_rsm(C, "base", TRUE);
			break;
		case SE_ZPREPASS:	// zprepass
			uber_zprepass(C, "base_multisampled", TRUE);
			break;
		case SE_PLANAR:
			uber_planar(C, "base", TRUE);
			break;

		}
	}
}
