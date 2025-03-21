///////////////////////////////////////////////////////////////////////////////////
// Created: 15.11.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "blender_antialiasing.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
CBlender_antialiasing::CBlender_antialiasing()
{
	description.CLS = 0;
}

CBlender_antialiasing::~CBlender_antialiasing()
{
}

void CBlender_antialiasing::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("null", "postprocess_stage_antialiasing_pass_fxaa", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_End();
		break;
	}
}
///////////////////////////////////////////////////////////////////////////////////
