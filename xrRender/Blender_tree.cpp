// Blender_Vertex_aref.cpp: implementation of the CBlender_Tree class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "Blender_tree.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Tree::CBlender_Tree()
{
	description.CLS = B_TREE;
	description.version = 1;
	oBlend.value = FALSE;
	oNotAnTree.value = FALSE;
}

CBlender_Tree::~CBlender_Tree()
{
}

void CBlender_Tree::Save(IWriter& fs)
{
	IBlender::Save(fs);
	xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
	xrPWRITE_PROP(fs, "Object LOD", xrPID_BOOL, oNotAnTree);
}

void CBlender_Tree::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);
	xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
	if (version >= 1)
	{
		xrPREAD_PROP(fs, xrPID_BOOL, oNotAnTree);
	}
}
//////////////////////////////////////////////////////////////////////////
#include "shader_name_generator.h"
void CBlender_Tree::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	//*************** codepath is the same, only shaders differ
	LPCSTR tvs = "multiple_object_animated";
	LPCSTR tvs_s = "shadow_direct_multiple_object_animated";
	if (oNotAnTree.value)
	{
		tvs = "multiple_object";
		tvs_s = "shadow_direct_multiple_object";
	}

	switch (C.iElement)
	{
	case SE_NORMAL_HQ: // deffer
		generate_shader_name(C, true, tvs, "static_mesh", oBlend.value);
		break;
	case SE_NORMAL_LQ: // deffer
		generate_shader_name(C, false, tvs, "static_mesh", oBlend.value);
		break;
	case SE_SHADOW: // smap-spot
		if (oBlend.value)
			C.r_Pass(tvs_s, "shadow_direct_static_mesh_alphatest", FALSE, TRUE, TRUE, TRUE, D3DBLEND_ZERO, D3DBLEND_ONE,
					 TRUE, 200);
		else
			C.r_Pass(tvs_s, "shadow_direct_static_mesh", FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		C.r_End();
		break;
	}
}

