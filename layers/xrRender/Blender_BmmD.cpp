// BlenderDefault.cpp: implementation of the CBlender_BmmD class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "blender_BmmD.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_BmmD::CBlender_BmmD()
{
	description.CLS = B_BmmD;
	strcpy(oT2_Name, "$null");
	strcpy(oT2_xform, "$null");
	description.version = 3;
	strcpy(oR_Name, "detail\\detail_grnd_grass");	//"$null");
	strcpy(oG_Name, "detail\\detail_grnd_asphalt"); //"$null");
	strcpy(oB_Name, "detail\\detail_grnd_earth");	//"$null");
	strcpy(oA_Name, "detail\\detail_grnd_yantar");	//"$null");
}

CBlender_BmmD::~CBlender_BmmD()
{
}

void CBlender_BmmD::Save(IWriter& fs)
{
	IBlender::Save(fs);
	xrPWRITE_MARKER(fs, "Detail map");
	xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT2_Name);
	xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT2_xform);
	xrPWRITE_PROP(fs, "R2-R", xrPID_TEXTURE, oR_Name);
	xrPWRITE_PROP(fs, "R2-G", xrPID_TEXTURE, oG_Name);
	xrPWRITE_PROP(fs, "R2-B", xrPID_TEXTURE, oB_Name);
	xrPWRITE_PROP(fs, "R2-A", xrPID_TEXTURE, oA_Name);
}

void CBlender_BmmD::Load(IReader& fs, u16 version)
{
	IBlender::Load(fs, version);
	if (version < 3)
	{
		xrPREAD_MARKER(fs);
		xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
		xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
	}
	else
	{
		xrPREAD_MARKER(fs);
		xrPREAD_PROP(fs, xrPID_TEXTURE, oT2_Name);
		xrPREAD_PROP(fs, xrPID_MATRIX, oT2_xform);
		xrPREAD_PROP(fs, xrPID_TEXTURE, oR_Name);
		xrPREAD_PROP(fs, xrPID_TEXTURE, oG_Name);
		xrPREAD_PROP(fs, xrPID_TEXTURE, oB_Name);
		xrPREAD_PROP(fs, xrPID_TEXTURE, oA_Name);
	}
}

#if RENDER == R_R1
//////////////////////////////////////////////////////////////////////////
// R1
//////////////////////////////////////////////////////////////////////////
void CBlender_BmmD::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

		if (C.L_textures.size() < 2)
			Debug.fatal(DEBUG_INFO, "Not enought textures for shader, base tex: %s", *C.L_textures[0]);
		string256 mask;
		strconcat(sizeof(mask), mask, C.L_textures[0].c_str(), "_mask");
		switch (C.iElement)
		{
		case SE_R1_NORMAL_HQ:
			if (HW.Caps.raster_major >= 2)
			{
				C.r_Pass("impl_dt_adv", "impl_dt_adv", TRUE);
				C.r_Sampler("s_base", C.L_textures[0]);
				C.r_Sampler("s_lmap", C.L_textures[1]);
				C.r_Sampler("s_dt_r", oR_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
							D3DTEXF_ANISOTROPIC);
				C.r_Sampler("s_dt_g", oG_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
							D3DTEXF_ANISOTROPIC);
				C.r_Sampler("s_dt_b", oB_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
							D3DTEXF_ANISOTROPIC);
				C.r_Sampler("s_dt_a", oA_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
							D3DTEXF_ANISOTROPIC);
				C.r_Sampler("s_mask", mask);
			}
			else
			{
				C.r_Pass("impl_dt", "impl_dt", TRUE);
				C.r_Sampler("s_base", C.L_textures[0]);
				C.r_Sampler("s_lmap", C.L_textures[1]);
				C.r_Sampler("s_detail", oT2_Name);
			}
			C.r_End();
			break;
		case SE_R1_NORMAL_LQ:
			C.r_Pass("impl", "impl", TRUE);
			C.r_Sampler("s_base", C.L_textures[0]);
			C.r_Sampler("s_lmap", C.L_textures[1]);
			C.r_Sampler("s_detail", oT2_Name);
			C.r_End();
			break;
		case SE_R1_LPOINT:
			C.r_Pass("impl_point", "add_point", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
			C.r_Sampler("s_base", C.L_textures[0]);
			C.r_Sampler_clf("s_lmap_l", TEX_POINT_ATT);
			C.r_Sampler_clf("s_att", TEX_POINT_ATT);
			C.r_End();
			break;
		case SE_R1_LSPOT:
			C.r_Pass("impl_spot", "add_spot", FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
			C.r_Sampler("s_base", C.L_textures[0]);
			C.r_Sampler_clf("s_lmap_l", "internal\\internal_light_att", true);
			C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
			C.r_End();
			break;
		case SE_R1_LMODELS:
			C.r_Pass("impl_l", "impl_l", FALSE);
			C.r_Sampler("s_base", C.L_textures[0]);
			C.r_Sampler("s_lmap", C.L_textures[1]);
			C.r_End();
			break;
		}

}
#else
//////////////////////////////////////////////////////////////////////////
// R2
//////////////////////////////////////////////////////////////////////////
/*#include "shader_name_generator.h"
void CBlender_BmmD::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	// codepath is the same, only the shaders differ
	// ***only pixel shaders differ***
	extern u32 ps_r2_bump_mode;
	string256 mask;
	int BumpType = 0;

	strconcat(sizeof(mask), mask, C.L_textures[0].c_str(), "_mask");

	switch (C.iElement)
	{
	case SE_R2_NORMAL_HQ: // deffer
		if (ps_r2_bump_mode == 1)
			BumpType = 1; // normal
		else if (ps_r2_bump_mode == 2)
			BumpType = 2; // parallax
		else if (ps_r2_bump_mode == 3)
			BumpType = 3; // steep parallax

		//C.sh_macro(BumpType == 1, "USE_NORMAL_MAPPING", "1");
		//C.sh_macro(BumpType == 2, "USE_PARALLAX_MAPPING", "1");
		//C.sh_macro(BumpType == 3, "USE_STEEP_PARALLAX_MAPPING", "1");
		C.macros.add(BumpType == 1, "USE_NORMAL_MAPPING", "1");
		C.macros.add(BumpType == 2, "USE_PARALLAX_MAPPING", "1");
		C.macros.add(BumpType == 3, "USE_STEEP_PARALLAX_MAPPING", "1");

		C.r_Pass("gbuffer_stage_terrain", "gbuffer_stage_terrain", TRUE);

		C.r_Sampler("s_mask", mask);
		C.r_Sampler("s_lmap", C.L_textures[1], false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);

		C.r_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		C.r_Sampler("s_dt_r", oR_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_dt_g", oG_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_dt_b", oB_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_dt_a", oA_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		C.r_Sampler("s_dn_r", strconcat(sizeof(mask), mask, oR_Name, "_bump"));
		C.r_Sampler("s_dn_g", strconcat(sizeof(mask), mask, oG_Name, "_bump"));
		C.r_Sampler("s_dn_b", strconcat(sizeof(mask), mask, oB_Name, "_bump"));
		C.r_Sampler("s_dn_a", strconcat(sizeof(mask), mask, oA_Name, "_bump"));

		C.r_Sampler("s_dn_rX", strconcat(sizeof(mask), mask, oR_Name, "_bump#"));
		C.r_Sampler("s_dn_gX", strconcat(sizeof(mask), mask, oG_Name, "_bump#"));
		C.r_Sampler("s_dn_bX", strconcat(sizeof(mask), mask, oB_Name, "_bump#"));
		C.r_Sampler("s_dn_aX", strconcat(sizeof(mask), mask, oA_Name, "_bump#"));
		C.r_End();
		break;
	case SE_R2_NORMAL_LQ: // deffer
		C.r_Pass("gbuffer_stage_terrain_lq", "gbuffer_stage_terrain_lq", TRUE);

		C.r_Sampler("s_lmap", C.L_textures[1], false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
		C.r_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_detail", oT2_Name);

		C.r_End();
		break;
	case SE_R2_SHADOW: // smap
		C.r_Pass("shadow_direct_static_mesh", "shadow_direct_static_mesh", FALSE, TRUE, TRUE, FALSE);

		C.r_Sampler("s_base", C.L_textures[0]);

		C.r_End();
		break;
	}
}*/
#include "uber_deffer.h"
void CBlender_BmmD::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	// codepath is the same, only the shaders differ
	// ***only pixel shaders differ***
	string256 mask;
	strconcat(sizeof(mask), mask, C.L_textures[0].c_str(), "_mask");
	switch (C.iElement)
	{
	case SE_R2_NORMAL_HQ: // deffer
		uber_deffer(C, true, "impl", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);
		C.r_Sampler("s_mask", mask);
		C.r_Sampler("s_lmap", C.L_textures[1]);

		C.r_Sampler("s_dt_r", oR_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_dt_g", oG_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_dt_b", oB_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_dt_a", oA_Name, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					D3DTEXF_ANISOTROPIC);

		C.r_Sampler("s_dn_r", strconcat(sizeof(mask), mask, oR_Name, "_bump"));
		C.r_Sampler("s_dn_g", strconcat(sizeof(mask), mask, oG_Name, "_bump"));
		C.r_Sampler("s_dn_b", strconcat(sizeof(mask), mask, oB_Name, "_bump"));
		C.r_Sampler("s_dn_a", strconcat(sizeof(mask), mask, oA_Name, "_bump"));

		C.r_End();
		break;
	case SE_R2_NORMAL_LQ: // deffer
		uber_deffer(C, false, "base", "impl", false, oT2_Name[0] ? oT2_Name : 0, true);
		C.r_Sampler("s_lmap", C.L_textures[1]);
		C.r_End();
		break;
	case SE_R2_SHADOW: // smap
		if (RImplementation.o.HW_smap)
			C.r_Pass("shadow_direct_base", "dumb", FALSE, TRUE, TRUE, FALSE);
		else
			C.r_Pass("shadow_direct_base", "shadow_direct_base", FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		C.r_End();
		break;
	}
}
#endif
