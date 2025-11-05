// Blender_Vertex_aref.cpp: implementation of the CBlender_Detail_Still class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "blender_detail_still.h"
#include "uber_deffer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Detail_Still::CBlender_Detail_Still()
{
	description.CLS		= B_DETAIL;
	description.version	= 0;
}

CBlender_Detail_Still::~CBlender_Detail_Still()
{

}

void	CBlender_Detail_Still::Save		(IWriter& fs )
{
	IBlender::Save		(fs);
	xrPWRITE_PROP		(fs,"Alpha-blend",	xrPID_BOOL,		oBlend);
}

void	CBlender_Detail_Still::Load		(IReader& fs, u16 version )
{
	IBlender::Load		(fs,version);
	xrPREAD_PROP		(fs,xrPID_BOOL,		oBlend);
}

#include "uber_deffer.h"

void	CBlender_Detail_Still::Compile	(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	switch(C.iElement) 
	{
	case SE_NORMAL_HQ: 
		uber(C, false, "detail_w", "base", true, (LPCSTR)0, true);
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		C.r_CullMode(D3D_CULL_NONE);
		C.r_End();
		break; // deffer wave
	case SE_NORMAL_LQ: 
		uber(C, false, "detail_s", "base", true, (LPCSTR)0, true);
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		C.r_CullMode(D3D_CULL_NONE);
		C.r_End();
		break; // deffer still
	case SE_SHADOW:
		C.r_Pass("shadow_direct_detail_w_aref", "shadow_direct_base_aref", FALSE, TRUE, TRUE, FALSE);
		C.r_CullMode(D3D_CULL_NONE);
		C.r_dx10Texture("s_base", C.L_textures[0]);
		C.r_dx10Sampler("smp_base");
		C.r_ColorWriteEnable(RImplementation.o.vsm, RImplementation.o.vsm, false, false);
		C.r_End();
		break;

	//case SE_RSM_FILL_RTS:
		//uber_rsm(C, "detail_w");
		//break;
	//case SE_ZPREPASS:
		//uber_zprepass(C, "detail_w_multisampled", TRUE);
		//break;
	case SE_PLANAR:
		uber_planar(C, "detail_w", TRUE);
		break;

	}
}