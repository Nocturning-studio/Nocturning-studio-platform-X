// BlenderDefault.cpp: implementation of the CBlender_Model_EbB class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "blender_Model_EbB.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Model_EbB::CBlender_Model_EbB()
{
	description.CLS = B_MODEL_EbB;
	description.version = 0x1;
	strcpy(oT2_Name, "$null");
	strcpy(oT2_xform, "$null");
	oBlend.value = FALSE;
}

CBlender_Model_EbB::~CBlender_Model_EbB()
{
}

void CBlender_Model_EbB::Save(IWriter& fs)
{
	description.version = 0x1;
	IBlender::Save(fs);
	xrPWRITE_MARKER(fs, "Environment map");
	xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
	xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
	xrPWRITE_PROP(fs, "Alpha-Blend", xrPID_BOOL, oBlend);
}

void CBlender_Model_EbB::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);
	xrPREAD_MARKER(fs);
	xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
	xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
	if (version >= 0x1)
	{
		xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
	}
}

#include "shader_configurator.h"
void CBlender_Model_EbB::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	if (oBlend.value)
	{
		// forward
		LPCSTR vsname = 0;
		LPCSTR psname = 0;
		switch (C.iElement)
		{
		case 0:
		case 1:
			vsname = psname = "model_env_lq";
			C.r_Pass(vsname, psname, TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, 0);
			C.r_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
			C.r_Sampler("s_env", oT2_Name, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR,
						true);
			C.r_End();
			break;
		}
	}
	else
	{
		// deferred
		switch (C.iElement)
		{
		case SE_NORMAL_HQ: // deffer
			configure_shader(C, true, "dynamic_mesh", "static_mesh", false);
			break;
		case SE_NORMAL_LQ: // deffer
			configure_shader(C, false, "dynamic_mesh", "static_mesh", false);
			break;
		case SE_SHADOW: // smap
			C.r_Pass("shadow_direct_dynamic_mesh", "shadow_direct_static_mesh", FALSE, TRUE, TRUE, FALSE);
			C.r_Sampler("s_base", C.L_textures[0]);
			C.r_End();
			break;
		}
	}
}

