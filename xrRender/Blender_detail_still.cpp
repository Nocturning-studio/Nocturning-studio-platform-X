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
#include "shader_name_generator.h"
void CBlender_Detail_Still::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_NORMAL_HQ: // deffer wave
		generate_shader_name(C, false, "detail_object_animated", "detail_object", false);
		break;
	case SE_NORMAL_LQ: // deffer still
		generate_shader_name(C, false, "detail_object", "detail_object", false);
		break;
	}
}

