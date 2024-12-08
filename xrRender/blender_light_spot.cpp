#include "stdafx.h"
#pragma hdrstop

#include "Blender_light_spot.h"
#include "r_types.h"

CBlender_accum_spot::CBlender_accum_spot()
{
	description.CLS = 0;
}
CBlender_accum_spot::~CBlender_accum_spot()
{
}

void CBlender_accum_spot::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case SE_L_FILL: // masking
		C.r_Pass("null", "copy", false, FALSE, FALSE);
		C.r_Sampler("s_base", C.L_textures[0]);
		C.r_End();
		break;
	case SE_L_UNSHADOWED: // unshadowed
		C.sh_macro("USE_LIGHT_MAPPING", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
		gbuffer(C);
		//C.r_Sampler_clw("s_material", r_material);
		C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
		C.r_End();
		break;
	case SE_L_NORMAL: // normal
		C.sh_macro("USE_SHADOW_MAPPING", "1");
		C.sh_macro("USE_LIGHT_MAPPING", "1");
		C.sh_macro("USE_LIGHT_MAP_XFORM", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
		gbuffer(C);
		//C.r_Sampler_clw("s_material", r_material);
		C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
		C.r_Sampler_clf("s_smap", r_RT_smap_depth);
		jitter(C);
		C.r_End();
		break;
	case SE_L_FULLSIZE: // normal-fullsize
		C.sh_macro("USE_SHADOW_MAPPING", "1");
		C.sh_macro("USE_LIGHT_MAPPING", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
		gbuffer(C);
		//C.r_Sampler_clw("s_material", r_material);
		C.r_Sampler("s_lmap", C.L_textures[0], false, D3DTADDRESS_CLAMP);
		C.r_Sampler_clf("s_smap", r_RT_smap_depth);
		jitter(C);
		C.r_End();
		break;
	case SE_L_TRANSLUENT: // shadowed + transluency
		C.sh_macro("USE_SHADOW_MAPPING", "1");
		C.sh_macro("USE_LIGHT_MAPPING", "1");
		C.r_Pass("accumulating_light_stage_volume", "accumulating_light_stage_spot", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
		gbuffer(C);
		//C.r_Sampler_clw("s_material", r_material);
		C.r_Sampler_clf("s_lmap", r_RT_smap_surf); // diff here
		C.r_Sampler_clf("s_smap", r_RT_smap_depth);
		jitter(C);
		C.r_End();
		break;
	}
}
