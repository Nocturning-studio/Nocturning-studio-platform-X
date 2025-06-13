#include "stdafx.h"
#pragma hdrstop

#include "blender_output_to_screen.h"
#include "r_types.h"

CBlender_output_to_screen::CBlender_output_to_screen()
{
	description.CLS = 0;
}

CBlender_output_to_screen::~CBlender_output_to_screen()
{
}

void CBlender_output_to_screen::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("screen_quad", "output_to_screen_stage", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_image", r_RT_generic0);
		C.r_End();
		break;
	}
}
