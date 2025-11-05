#include "stdafx.h"

#include "blender_smaa.h"

CBlender_smaa::CBlender_smaa() { description.CLS = 0; }

CBlender_smaa::~CBlender_smaa()
{
}

void CBlender_smaa::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	switch (C.iElement)
	{
	case 0: //Edge detection
		C.r_Pass("smaa_edge_detection", "smaa_edge_detection", FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_image", r2_RT_generic0);
		C.r_End();
		break;
	case 1:	//Weight 
		C.r_Pass("smaa_blending_weight_calculator", "smaa_blending_weight_calculator", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		C.r_Sampler_rtf("s_edgetex", r2_RT_smaa_edgetex);
		C.r_Sampler_rtf("s_areatex", "shaders\\vfx\\smaa_area");
		C.r_Sampler_rtf("s_searchtex", "shaders\\vfx\\smaa_search_tex");
		C.r_End();
		break;
	case 2:	//Blending
		C.r_Pass("smaa_neighborhood_blending", "smaa_neighborhood_blending", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r2_RT_generic0);
		C.r_Sampler_rtf("s_blendtex", r2_RT_smaa_blendtex);
		C.r_End();
		break;
	}
}
