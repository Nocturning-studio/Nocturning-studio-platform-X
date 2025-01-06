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
#include "shader_configurator.h"
void CBlender_Tree::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	//*************** codepath is the same, only shaders differ
	LPCSTR tvs = "multiple_usage_object_animated";
	LPCSTR tvs_s = "shadow_depth_stage_multiple_usage_object_animated";
	if (oNotAnTree.value)
	{
		tvs = "multiple_usage_object";
		tvs_s = "shadow_depth_stage_multiple_usage_object";
	}

	switch (C.iElement)
	{
	case SE_NORMAL_HQ: // deffer
		configure_shader(C, true, tvs, "static_mesh", oBlend.value);
		break;
	case SE_NORMAL_LQ: // deffer
		configure_shader(C, false, tvs, "static_mesh", oBlend.value);
		break;
	case SE_SHADOW_DEPTH: // smap-spot
		if (oBlend.value)
			C.r_Pass(tvs_s, "shadow_depth_stage_static_mesh_alphatest", FALSE);
		else
			C.r_Pass(tvs_s, "shadow_depth_stage_static_mesh", FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		jitter(C);
		C.r_End();
		break;
	case SE_DEPTH_PREPASS: // smap-spot
		if (oBlend.value)
			C.r_Pass(tvs_s, "depth_prepass_stage_static_mesh_alphatest", FALSE, TRUE, TRUE, FALSE);
		else
			C.r_Pass(tvs_s, "depth_prepass_stage_static_mesh", FALSE, TRUE, TRUE, FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		jitter(C);
		C.r_End();
		break;
	}
}

