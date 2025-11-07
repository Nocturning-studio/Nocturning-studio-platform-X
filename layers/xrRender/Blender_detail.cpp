// Blender_Vertex_aref.cpp: implementation of the CBlender_Detail class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "Blender_detail.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Detail::CBlender_Detail()
{
	description.CLS = B_DETAIL;
	description.version = 0;
}

CBlender_Detail::~CBlender_Detail()
{
}

void CBlender_Detail::Save(IWriter& fs)
{
	IBlender::Save(fs);
	xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_Detail::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);
	xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
}

#if RENDER == R_R1
void CBlender_Detail::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_R1_NORMAL_HQ:
		C.r_Pass("detail_wave", "detail", FALSE, TRUE, TRUE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
		C.r_Sampler("s_base", C.L_textures[0]);
		C.r_CullMode(D3DCULL_NONE);
		C.r_End();
		break;
	case SE_R1_NORMAL_LQ:
		C.r_Pass("detail_still", "detail", FALSE, TRUE, TRUE, FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
		C.r_Sampler("s_base", C.L_textures[0]);
		C.r_CullMode(D3DCULL_NONE);
		C.r_End();
		break;
	case SE_R1_LPOINT:
		break;
	case SE_R1_LSPOT:
		break;
	case SE_R1_LMODELS:
		break;
	}

}
#else 
//////////////////////////////////////////////////////////////////////////
// R2
//////////////////////////////////////////////////////////////////////////
/*#include "shader_name_generator.h"
void CBlender_Detail::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_R2_NORMAL_HQ: // deffer wave
		generate_shader_name(C, false, "detail_object_animated", "detail_object", false);
		break;
	case SE_R2_NORMAL_LQ: // deffer still
		generate_shader_name(C, false, "detail_object", "detail_object", false);
		break;
	}
}*/
#include "uber_deffer.h"
void CBlender_Detail::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_R2_NORMAL_HQ: // deffer wave
		uber_deffer(C, false, "detail_w", "base", true);
		break;
	case SE_R2_NORMAL_LQ: // deffer still
		uber_deffer(C, false, "detail_s", "base", true);
		break;
	}
}
#endif
