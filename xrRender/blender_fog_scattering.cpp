///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_fog_scattering.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_fog_scattering::CBlender_fog_scattering()
{
	description.CLS = 0;
}

CBlender_fog_scattering::~CBlender_fog_scattering()
{
}

void CBlender_fog_scattering::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("null", "postprocess_stage_fog_scattering_pass_vertical_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();

		C.r_Pass("null", "postprocess_stage_fog_scattering_pass_horizontal_filter", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		gbuffer(C);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
