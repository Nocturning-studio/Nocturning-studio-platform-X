#include "stdafx.h"
#pragma hdrstop

#include "blender_light_direct.h"

CBlender_accum_direct::CBlender_accum_direct() { description.CLS = 0; }
CBlender_accum_direct::~CBlender_accum_direct() {	}

void	CBlender_accum_direct::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	BOOL		blend = FALSE;
	D3D_BLEND	dest = blend ? D3D_BLEND_ONE : D3D_BLEND_ZERO;

	switch (C.iElement)
	{
	case SE_SUN_NEAR:		// near pass - enable Z-test to perform depth-clipping
	case SE_SUN_MIDDLE:		// middle pass - enable Z-test to perform depth-clipping
		//	FVF::TL2uv
		C.r_Pass("accum_v", "accum_sun_near_middle", FALSE, TRUE, FALSE, FALSE, D3D_BLEND_ONE, dest);
		C.r_CullMode(D3D_CULL_NONE);
		C.PassSET_ZB(TRUE, FALSE, TRUE);	// force inverted Z-Buffer
		C.r_dx10Texture("s_material", tex_t_material);
		C.r_dx10Texture("s_diffuse", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_smap", tex_rt_smap_depth);
		C.r_dx10Texture("s_vsm", tex_rt_vsm_depth);
		C.r_dx10Texture("s_lmap", "sunmask");
		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_smap");
		C.Jitter();
		break;
	case SE_SUN_FAR:		// far pass, only stencil clipping performed
		//	FVF::TL2uv
		C.r_Pass("accum_v", "accum_sun_far", FALSE, TRUE, FALSE, blend, D3D_BLEND_ONE, dest);
		C.r_CullMode(D3D_CULL_NONE);
		C.r_dx10Texture("s_material", tex_t_material);
		C.r_dx10Texture("s_diffuse", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_smap", tex_rt_smap_depth);
		C.r_dx10Texture("s_vsm", tex_rt_vsm_depth);
		C.r_dx10Texture("s_lmap", "sunmask");
		C.Jitter();

		{
			u32 s = C.r_dx10Sampler("smp_smap");
			C.i_dx10Address(s, D3D_TEXTURE_ADDRESS_BORDER);
			C.i_dx10BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
		}

		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_smap");

		break;
	case SE_SUN_FAR_VOLUMETRIC:		// volumetric pass
		C.r_Pass("accum_v", "accum_sun_far_volume", FALSE, FALSE, FALSE, TRUE, D3D_BLEND_ONE, D3D_BLEND_ONE);
		C.r_dx10Texture("s_material", tex_t_material);
		C.r_dx10Texture("s_diffuse", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_smap", tex_rt_smap_depth);
		C.r_dx10Texture("s_vsm", tex_rt_vsm_depth);
		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_smap");
		C.Jitter();
		break;

	case SE_SUN_FAR_VOL_MINMAX:		// volumetric pass with minmax
		C.r_Pass("accum_v", "accum_sun_far_volume_minmax", FALSE, FALSE, FALSE, TRUE, D3D_BLEND_ONE, D3D_BLEND_ONE);
		C.r_dx10Texture("s_material", tex_t_material);
		C.r_dx10Texture("s_diffuse", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_smap", tex_rt_smap_depth);
		C.r_dx10Texture("s_vsm", tex_rt_vsm_depth);
		C.r_dx10Texture("s_smap_minmax", tex_rt_smap_depth_minmax);
		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_smap");
		C.Jitter();
		break;

	case SE_SUN_VSM:		// enable Z-test to perform depth-clipping
		//	FVF::TL2uv
		C.r_Pass("accum_v", "accum_sun_vsm", FALSE, TRUE, FALSE, FALSE, D3D_BLEND_ONE, dest);
		C.r_CullMode(D3D_CULL_NONE);
		C.PassSET_ZB(TRUE, FALSE, TRUE);	// force inverted Z-Buffer
		C.r_dx10Texture("s_material", tex_t_material);
		C.r_dx10Texture("s_diffuse", tex_rt_Color);
		C.r_dx10Texture("s_position", tex_rt_Position);
		C.r_dx10Texture("s_depth", tex_t_depth);
		C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
		C.r_dx10Texture("s_smap", tex_rt_smap_depth);
		C.r_dx10Texture("s_vsm", tex_rt_vsm_depth);
		C.r_dx10Texture("s_lmap", "sunmask");
		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_smap");
		C.Jitter();
		break;
	}

	if (RImplementation.o.cspecular) C.r_ColorWriteEnable(true, true, true, false);
	C.r_End();
}
