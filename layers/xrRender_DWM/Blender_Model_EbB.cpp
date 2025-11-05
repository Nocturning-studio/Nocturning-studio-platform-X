// BlenderDefault.cpp: implementation of the CBlender_Model_EbB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "blender_Model_EbB.h"
#include "uber_deffer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Model_EbB::CBlender_Model_EbB	()
{
	description.CLS		= B_MODEL_EbB;
	description.version	= 0x1;
	xr_strcpy				(oT2_Name,	"$null");
	xr_strcpy				(oT2_xform,	"$null");
	oBlend.value		= FALSE;
}

CBlender_Model_EbB::~CBlender_Model_EbB	()
{
	
}

void	CBlender_Model_EbB::Save(	IWriter& fs )
{
	description.version	= 0x1;
	IBlender::Save	(fs);
	xrPWRITE_MARKER	(fs,"Environment map");
	xrPWRITE_PROP	(fs,"Name",				xrPID_TEXTURE,	oT2_Name);
	xrPWRITE_PROP	(fs,"Transform",		xrPID_MATRIX,	oT2_xform);
	xrPWRITE_PROP	(fs,"Alpha-Blend",		xrPID_BOOL,		oBlend);
}

void	CBlender_Model_EbB::Load(	IReader& fs, u16 version )
{
	IBlender::Load	(fs,version);
	xrPREAD_MARKER	(fs);
	xrPREAD_PROP	(fs,xrPID_TEXTURE,	oT2_Name);
	xrPREAD_PROP	(fs,xrPID_MATRIX,	oT2_xform);
	if (version>=0x1)	{
		xrPREAD_PROP	(fs,xrPID_BOOL,	oBlend);
	}
}

#include "uber_deffer.h"

void CBlender_Model_EbB::Compile( CBlender_Compile& C )
{
	IBlender::Compile(C);

	if (oBlend.value)	
	{
		// forward
		switch(C.iElement) 
		{
		case SE_NORMAL_HQ:
		case SE_NORMAL_LQ:
			C.r_Pass("model_env_lq", "model_env_lq", TRUE, TRUE, FALSE, TRUE, D3D_BLEND_SRC_ALPHA, D3D_BLEND_INV_SRC_ALPHA, TRUE, 0);
			C.r_dx10Texture("s_base", C.L_textures[0]);
			C.r_dx10Texture("s_env", oT2_Name);
			C.r_dx10Sampler("smp_base");
			C.r_dx10Sampler("smp_rtlinear");
			break;
		}
	} 
	else 
	{
		// deferred
		switch(C.iElement) 
		{
		case SE_NORMAL_HQ: 		// deffer
			uber(C, true, "model", "base", false, 0, true);
			C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			C.r_StencilRef(0x01);
			C.r_End();
			break;
		case SE_NORMAL_LQ: 		// deffer
			uber(C, false, "model", "base", false, 0, true);
			C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			C.r_StencilRef(0x01);
			C.r_End();
			break;
		case SE_SHADOW:		// smap
			C.r_Pass("shadow_direct_model", "shadow_direct_base", FALSE, TRUE, TRUE, FALSE);
			C.r_dx10Texture("s_base", C.L_textures[0]);
			C.r_dx10Sampler("smp_base");
			C.r_ColorWriteEnable(RImplementation.o.vsm, RImplementation.o.vsm, false, false);
			C.r_End();
			break;

		case SE_RSM_FILL_RTS:	// RSM
			uber_rsm(C, "model");
			break;
		case SE_ZPREPASS:	// zprepass
			uber_zprepass(C, "model_multisampled", oBlend.value);
			break;
		case SE_PLANAR:
			uber_planar(C, "model");
			break;

		}
	}
}
