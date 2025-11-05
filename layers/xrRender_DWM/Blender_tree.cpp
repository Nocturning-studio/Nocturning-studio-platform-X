// Blender_Vertex_aref.cpp: implementation of the CBlender_Tree class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "blender_tree.h"
#include "uber_deffer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Tree::CBlender_Tree()
{
	description.CLS		= B_TREE;
	description.version	= 1;
	oBlend.value		= FALSE;
	oNotAnTree.value	= FALSE;
}

CBlender_Tree::~CBlender_Tree()
{

}

void	CBlender_Tree::Save		(IWriter& fs )
{
	IBlender::Save		(fs);
	xrPWRITE_PROP		(fs,"Alpha-blend",	xrPID_BOOL,		oBlend);
	xrPWRITE_PROP		(fs,"Object LOD",	xrPID_BOOL,		oNotAnTree);
}

void	CBlender_Tree::Load		(IReader& fs, u16 version )
{
	IBlender::Load		(fs,version);
	xrPREAD_PROP		(fs,xrPID_BOOL,		oBlend);
	if (version>=1)		{
		xrPREAD_PROP		(fs,xrPID_BOOL,		oNotAnTree);
	}
}

//////////////////////////////////////////////////////////////////////////
// R4
//////////////////////////////////////////////////////////////////////////
#include "uber_deffer.h"

void	CBlender_Tree::Compile	(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	//*************** codepath is the same, only shaders differ
	LPCSTR tvs, tvs_s, ztvs_s;

	if (oNotAnTree.value)	
	{
		tvs = "tree_s";
		ztvs_s = "tree_multisampled_s";

		if (oBlend.value)
			tvs_s = "shadow_direct_tree_s_aref";
		else
			tvs_s = "shadow_direct_tree_s";
	}
	else
	{
		tvs = "tree";
		ztvs_s = "tree_multisampled";

		if (oBlend.value) 
			tvs_s = "shadow_direct_tree_aref";
		else
			tvs_s = "shadow_direct_tree";
	}

	switch (C.iElement)
	{
	case SE_NORMAL_HQ:	// deffer
		uber(C, true, tvs, "base", oBlend.value, 0, true);
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		C.r_End();
		break;
	case SE_NORMAL_LQ:	// deffer
		uber(C, false, tvs, "base", oBlend.value, 0, true);
		C.r_Stencil(TRUE, D3D_COMPARISON_ALWAYS, 0xff, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		C.r_StencilRef(0x01);
		C.r_End();
		break;
	case SE_SHADOW:		// smap-spot
		//if (oBlend.value) break;
		//if (oBlend.value)	C.r_Pass(tvs_s, "shadow_direct_base_aref", FALSE, TRUE, TRUE, TRUE, D3D_BLEND_ZERO, D3D_BLEND_ONE, TRUE, 200);
		if (oBlend.value)	C.r_Pass(tvs_s, "shadow_direct_base_aref", FALSE, TRUE, TRUE, FALSE);
		else				C.r_Pass(tvs_s, "shadow_direct_base", FALSE);
		C.r_dx10Texture("s_base", C.L_textures[0]);
		C.r_dx10Sampler("smp_base");
		C.r_ColorWriteEnable(RImplementation.o.vsm, RImplementation.o.vsm, false, false);
		C.r_End();
		break;

	case SE_RSM_FILL_RTS:	// RSM
		uber_rsm(C, "tree", oBlend.value);
		break;
	case SE_ZPREPASS:	// zprepass
		uber_zprepass(C, ztvs_s, oBlend.value);
		break;
	case SE_PLANAR:
		uber_planar(C, tvs, oBlend.value);
		break;

	}
}
