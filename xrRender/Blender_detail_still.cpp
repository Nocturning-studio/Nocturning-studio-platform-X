// Blender_Vertex_aref.cpp: implementation of the CBlender_Detail_Still class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "Blender_Detail_still.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Detail_Still::CBlender_Detail_Still()
{
	description.CLS = B_DETAIL;
	description.version = 0;
}

CBlender_Detail_Still::~CBlender_Detail_Still()
{
}

void CBlender_Detail_Still::Save(IWriter& fs)
{
	IBlender::Save(fs);
	xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_Detail_Still::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);
	xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
}

//////////////////////////////////////////////////////////////////////////
#include "shader_configurator.h"
void CBlender_Detail_Still::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_DETAIL_NORMAL_ANIMATED: // deffer wave
		configure_shader(C, false, "detail_object_animated", "detail_object", false);
		break;
	case SE_DETAIL_NORMAL_STATIC: // deffer still
		configure_shader(C, false, "detail_object", "detail_object", false);
		break;
	case SE_DETAIL_SHADOW_DEPTH_ANIMATED:
		C.r_Pass("shadow_depth_stage_detail_object_animated", "shadow_depth_stage_detail_object", FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		jitter(C);
		C.r_End();
		break;
	case SE_DETAIL_SHADOW_DEPTH_STATIC:
		C.r_Pass("shadow_depth_stage_detail_object", "shadow_depth_stage_detail_object", FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		jitter(C);
		C.r_End();
		break;
	//case SE_DETAIL_DEPTH_PREPASS_ANIMATED:
	//	C.sh_macro("USE_DETAILWAVE", "1");
	//case SE_DETAIL_DEPTH_PREPASS_STATIC:
	//	C.r_Pass("shadow_depth_stage_detail_object", "shadow_depth_stage_detail_object", FALSE);//C.r_Pass("depth_prepass_stage_detail_object", "depth_prepass_stage_detail_object", FALSE);
	//	C.r_Sampler("s_base", C.L_textures[0]);
	//	jitter(C);
	//	C.r_End();
	//	break;
	}
}

