#include "stdafx.h"
#include "blender_rain.h"

void CBlender_rain::Compile(CBlender_Compile& C)
{
	IBlender::Compile	(C);

	switch (C.iElement) 
	{
		// Patch normals
		case 0: 
			C.r_Pass("stub_notransform_2uv", "rain_wet_accum", false);
			C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

			C.r_dx10Texture("s_position", tex_rt_Position);
			C.r_dx10Texture("s_depth", tex_t_depth);
			C.r_dx10Texture("s_depthms", tex_t_msaa_depth);
			C.r_dx10Texture("s_smap", tex_rt_smap_depth);
			C.r_dx10Texture("s_diffuse", tex_rt_Color);
			C.r_dx10Texture("s_diffuse_ms", tex_rt_Color_ms);
			C.r_dx10Texture("s_water", "water\\water_SBumpVolume");
			C.r_dx10Texture("s_waterFall", "water\\water_flowing_nmap");

			C.r_dx10Sampler("smp_nofilter");
			C.r_dx10Sampler("smp_linear");
			C.r_dx10Sampler("smp_base");
			C.r_dx10Sampler("smp_smap");

			C.Jitter();

			C.r_End();
		break;

		// Apply normals
		case 1: 
			C.r_Pass("stub_notransform_2uv", "rain_apply_normal", false);
			C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

			C.r_dx10Texture("s_wet_accum", tex_rt_Accumulator);
			C.r_dx10Sampler("smp_nofilter");

			C.r_ColorWriteEnable(true, true, false, false);

			C.r_End();
		break;

		// Apply gloss
		case 2:
			C.r_Pass("stub_notransform_2uv", "rain_apply_color_gloss", FALSE, TRUE, FALSE, TRUE, D3D_BLEND_ONE, D3D_BLEND_ONE);

			C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer
			
			C.RS.SetRS(D3DRS_SRCBLEND,			D3D_BLEND_ZERO);
			C.RS.SetRS(D3DRS_DESTBLEND,			D3D_BLEND_SRC_COLOR);
			C.RS.SetRS(D3DRS_SRCBLENDALPHA,		D3D_BLEND_ONE);
			C.RS.SetRS(D3DRS_DESTBLENDALPHA,	D3D_BLEND_ONE);

			C.r_dx10Texture("s_wet_accum", tex_rt_Accumulator);
			C.r_dx10Sampler("smp_nofilter");

			C.r_ColorWriteEnable(true, true, true, true);

			C.r_End();

		break;
	}
}
