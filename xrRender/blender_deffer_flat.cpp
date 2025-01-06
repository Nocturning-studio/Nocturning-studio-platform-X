#include "stdafx.h"
#pragma hdrstop

#include "shader_configurator.h"
#include "Blender_deffer_flat.h"

CBlender_deffer_flat::CBlender_deffer_flat()
{
	description.CLS = B_DEFAULT;
}
CBlender_deffer_flat::~CBlender_deffer_flat()
{
}

void CBlender_deffer_flat::Save(IWriter& fs)
{
	IBlender::Save(fs);
}
void CBlender_deffer_flat::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);
}

void CBlender_deffer_flat::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	// codepath is the same, only the shaders differ
	switch (C.iElement)
	{
	case SE_NORMAL_HQ: // deffer
		configure_shader(C, true, "static_mesh", "static_mesh", false);
		break;
	case SE_NORMAL_LQ: // deffer
		configure_shader(C, false, "static_mesh", "static_mesh", false);
		break;
	case SE_SHADOW_DEPTH: // smap-direct
		C.r_Pass("shadow_depth_stage_static_mesh", "shadow_depth_stage_static_mesh", FALSE, TRUE, TRUE, FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		C.r_End();
		break;
	case SE_DEPTH_PREPASS:
		C.r_Pass("depth_prepass_stage_static_mesh", "depth_prepass_stage_static_mesh", FALSE, TRUE, TRUE, FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		C.r_End();
		break;
	}
}
