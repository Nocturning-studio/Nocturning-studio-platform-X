#include "stdafx.h"
#pragma hdrstop

#include "blender_deffer_flat.h"
#include "uber_deffer.h"

CBlender_deffer_flat::CBlender_deffer_flat	()
{
	description.CLS		= B_DEFAULT;
	description.version         = 1;
	oTessellation.Count         = 4;
	oTessellation.IDselected	= 0;
}

CBlender_deffer_flat::~CBlender_deffer_flat	()	{	}

void	CBlender_deffer_flat::Save	(	IWriter& fs )
{
	IBlender::Save	(fs);
	xrP_TOKEN::Item	I;
	xrPWRITE_PROP	(fs,"Tessellation",	xrPID_TOKEN, oTessellation);
	I.ID = 0; xr_strcpy(I.str,"NO_TESS");	fs.w(&I,sizeof(I));
	I.ID = 1; xr_strcpy(I.str,"TESS_PN");	fs.w(&I,sizeof(I));
	I.ID = 2; xr_strcpy(I.str,"TESS_HM");	fs.w(&I,sizeof(I));
	I.ID = 3; xr_strcpy(I.str,"TESS_PN+HM");	fs.w(&I,sizeof(I));
}
void	CBlender_deffer_flat::Load	(	IReader& fs, u16 version )
{
	IBlender::Load	(fs,version);
	if (version>0)
	{
		xrPREAD_PROP(fs,xrPID_TOKEN,oTessellation);	oTessellation.Count = 4;
	}
}

void	CBlender_deffer_flat::Compile(CBlender_Compile& C)
{
	IBlender::Compile		(C);
	
	C.TessMethod = oTessellation.IDselected;
	// codepath is the same, only the shaders differ
	switch(C.iElement) 
	{
	case SE_NORMAL_HQ: 		// deffer
		uber(C, true, "base", "base", false, 0, true); 
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS,0xff,0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		C.r_End			();
		break;
	case SE_NORMAL_LQ: 		// deffer
		uber(C, false, "base", "base", false, 0, true); 
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS,0xff,0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		C.r_End			();
		break;
	case SE_SHADOW:			// smap-direct
		uber_shadow(C, "base");
		C.r_dx10Texture("s_base",C.L_textures[0]);
		C.r_dx10Sampler("smp_base");
		C.r_ColorWriteEnable(RImplementation.o.vsm, RImplementation.o.vsm, false, false);
		C.r_End			();
		break;

	case SE_ZPREPASS:		// zprepass
		uber_zprepass(C, "base_multisampled", FALSE, TRUE);
		break;
	case SE_RSM_FILL_RTS:		// RSM
		uber_rsm(C, "base");
		break;
	case SE_PLANAR:
		uber_planar(C, "base");
		break;

	}
}
