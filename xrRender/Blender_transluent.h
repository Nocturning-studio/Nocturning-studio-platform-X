///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
#include "shader_configurator.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_transluent : public IBlender
{
  public:
	xrP_Integer oAREF;
	xrP_BOOL oBlend;
	bool lmapped;

  public:
	virtual LPCSTR getComment()
	{
		return "LEVEL: defer-base-aref";
	}

	virtual BOOL canBeDetailed()
	{
		return TRUE;
	}

	virtual BOOL canBeLMAPped()
	{
		return lmapped;
	}

	CBlender_transluent(bool _lmapped = false) : lmapped(_lmapped)
	{
		description.CLS = B_STATIC_MESH_TRANSLUENT;
		oAREF.value = 200;
		oAREF.min = 0;
		oAREF.max = 255;
		oBlend.value = FALSE;
	}

	void Save(IWriter& fs)
	{
		IBlender::Save(fs);
		xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
		xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
	}
	void Load(IReader& fs, u16 version)
	{
		IBlender::Load(fs, version);
		if (1 == version)
		{
			xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
			xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
		}
	}

	void Compile(CBlender_Compile& C)
	{
		IBlender::Compile(C);

		if (oBlend.value)
		{
			switch (C.iElement)
			{
			case SE_NORMAL_HQ:
			case SE_NORMAL_LQ:
				if (lmapped)
				{
					C.r_Pass("alpha_blend_lightmap_lighted", "alpha_blend_lightmap_lighted", TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
					C.r_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
					C.r_Sampler("s_lmap", C.L_textures[1]);
					C.r_Sampler_clf("s_hemi", *C.L_textures[2]);
					C.r_Sampler("s_env", r_T_envs0, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR, true);
					C.r_End();
				}
				else
				{
					C.r_Pass("alpha_blend_vertex_lighted", "alpha_blend_vertex_lighted", TRUE, TRUE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
					C.r_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
					C.r_End();
				}
				break;
			default:
				break;
			}
		}
		else
		{
			C.SetParams(1, false); //.

			// codepath is the same, only the shaders differ
			// ***only pixel shaders differ***
			switch (C.iElement)
			{
			case SE_NORMAL_HQ: // deffer
				configure_shader(C, true, "static_mesh", "static_mesh", true);
				break;
			case SE_NORMAL_LQ: // deffer
				configure_shader(C, false, "static_mesh", "static_mesh", true);
				break;
			case SE_SHADOW_DEPTH: // smap
								  // C.r_Pass("shadow_depth_stage_static_mesh_alphatest",
								  // "shadow_depth_stage_static_mesh_alphatest", FALSE); C.r_Sampler("s_base",
								  // C.L_textures[0]); jitter(C); C.r_End(); break;
			case SE_DEPTH_PREPASS:
				// C.r_Pass("depth_prepass_stage_static_mesh_alphatest", "depth_prepass_stage_static_mesh_alphatest",
				// FALSE, TRUE, TRUE, FALSE); C.r_Sampler("s_base", C.L_textures[0]); jitter(C); C.r_End();
				configure_shader(C, true, "static_mesh", "static_mesh", true, true);
				break;
			}
		}
	}

	virtual ~CBlender_transluent() = default;
};
///////////////////////////////////////////////////////////////////////////////////
