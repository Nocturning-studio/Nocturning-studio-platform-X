#include "stdafx.h"
#pragma hdrstop

#include "..\xrRender\shader_name_generator.h"
#include "Blender_deffer_model.h"

CBlender_deffer_model::CBlender_deffer_model()
{
	description.CLS = B_MODEL;
	description.version = 1;
	oAREF.value = 32;
	oAREF.min = 0;
	oAREF.max = 255;
	oBlend.value = FALSE;
}
CBlender_deffer_model::~CBlender_deffer_model()
{
}

void CBlender_deffer_model::Save(IWriter& fs)
{
	IBlender::Save(fs);
	xrPWRITE_PROP(fs, "Use alpha-channel", xrPID_BOOL, oBlend);
	xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
}
void CBlender_deffer_model::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);

	switch (version)
	{
	case 0:
		oAREF.value = 32;
		oAREF.min = 0;
		oAREF.max = 255;
		oBlend.value = FALSE;
		break;
	case 1:
	default:
		xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
		xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
		break;
	}
}

void CBlender_deffer_model::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	BOOL bForward = FALSE;
	if (oBlend.value && oAREF.value < 16)
		bForward = TRUE;
	if (oStrictSorting.value)
		bForward = TRUE;

	if (bForward)
	{
		// forward rendering
		LPCSTR vsname, psname;
		switch (C.iElement)
		{
		case 0: //
		case 1: //
			vsname = psname = "model_def_lq";
			C.r_Pass(vsname, psname, TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE,
					 oAREF.value);
			C.r_Sampler("s_base", C.L_textures[0]);
			C.r_End();
			break;
		default:
			break;
		}
	}
	else
	{
		BOOL bAref = oBlend.value;
		// deferred rendering
		// codepath is the same, only the shaders differ
		switch (C.iElement)
		{
		case SE_R2_NORMAL_HQ: // deffer
			generate_shader_name(C, true, "dynamic_mesh", "static_mesh", bAref);
			break;
		case SE_R2_NORMAL_LQ: // deffer
			generate_shader_name(C, false, "dynamic_mesh", "static_mesh", bAref);
			break;
		case SE_R2_SHADOW: // smap
			if (bAref)
			{
				C.r_Pass("shadow_direct_dynamic_mesh_alphatest", "shadow_direct_static_mesh_alphatest", FALSE, TRUE, TRUE, FALSE, D3DBLEND_ZERO, D3DBLEND_ONE, TRUE, 220);
				C.r_Sampler("s_base", C.L_textures[0]);
				C.r_End();
				break;
			}
			else
			{
				C.r_Pass("shadow_direct_dynamic_mesh", "shadow_direct_static_mesh", FALSE, TRUE, TRUE, FALSE);
				C.r_Sampler("s_base", C.L_textures[0]);
				C.r_End();
				break;
			}
		}
	}
}
