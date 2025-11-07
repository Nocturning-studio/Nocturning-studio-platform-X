#include "stdafx.h"
#pragma hdrstop

#include "Blender_combine.h"

CBlender_combine::CBlender_combine()
{
	description.CLS = 0;
}
CBlender_combine::~CBlender_combine()
{
}

void CBlender_combine::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0: // combine
		C.r_Pass("scene_combine_stage", "scene_combine_stage", FALSE, FALSE, FALSE, TRUE, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA); //. MRT-blend?
		gbuffer(C);
		C.r_Sampler_rtf("s_diffuse_accumulator", r2_RT_Diffuse_Accumulator);
		C.r_Sampler_rtf("s_specular_accumulator", r2_RT_Specular_Accumulator);
		C.r_Sampler_rtf("s_material", r2_material);
		C.r_Sampler_rtf("s_ao", r2_RT_ao);
		C.r_Sampler_clf("env_s0", r2_T_envs0);
		C.r_Sampler_clf("env_s1", r2_T_envs1);
		C.r_Sampler_clf("sky_s0", r2_T_sky0);
		C.r_Sampler_clf("sky_s1", r2_T_sky1);
		C.r_Sampler_rtf("s_vollight", r2_RT_generic2);

		C.r_Sampler("s_debug_mask", "ed\\debug_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_Sampler("s_debug_diagonal_mask", "ed\\debug_diagonal_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_Sampler("s_debug_triple_mask", "ed\\debug_triple_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_Sampler("s_debug_quadro_mask", "ed\\debug_quadro_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);

		C.r_Sampler_clf("s_brdf_lut", "vfx\\vfx_brdf_lut");

		jitter(C);
		C.r_End();
		break;
	case 1:
		C.r_Pass("scene_combine_stage", "scene_combine_stage_debug", FALSE, FALSE, FALSE);
		gbuffer(C);
		C.r_Sampler_rtf("s_diffuse_accumulator", r2_RT_Diffuse_Accumulator);
		C.r_Sampler_rtf("s_specular_accumulator", r2_RT_Specular_Accumulator);
		C.r_Sampler_rtf("s_material", r2_material);
		C.r_Sampler_rtf("s_ao", r2_RT_ao);
		C.r_Sampler_clf("env_s0", r2_T_envs0);
		C.r_Sampler_clf("env_s1", r2_T_envs1);
		C.r_Sampler_clf("sky_s0", r2_T_sky0);
		C.r_Sampler_clf("sky_s1", r2_T_sky1);
		C.r_Sampler_rtf("s_vollight", r2_RT_generic2);

		C.r_Sampler("s_debug_mask", "ed\\debug_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_Sampler("s_debug_diagonal_mask", "ed\\debug_diagonal_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_Sampler("s_debug_triple_mask", "ed\\debug_triple_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
		C.r_Sampler("s_debug_quadro_mask", "ed\\debug_quadro_mask", false, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);

		C.r_Sampler_clf("s_brdf_lut", "vfx\\vfx_brdf_lut");
		C.r_End();
		break;
	}
}
