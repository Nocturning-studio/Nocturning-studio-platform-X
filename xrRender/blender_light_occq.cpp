#include "stdafx.h"
#pragma hdrstop

#include "Blender_light_occq.h"

CBlender_light_occq::CBlender_light_occq()
{
	description.CLS = 0;
}
CBlender_light_occq::~CBlender_light_occq()
{
}

void CBlender_light_occq::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0: // occlusion testing
		C.r_Pass("accumulating_light_stage_occlusion_culling", "accumulating_light_stage_occlusion_culling", false,
				 TRUE, FALSE, FALSE);
		C.r_End();
		break;
	}
}
