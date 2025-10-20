///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_types.h"
///////////////////////////////////////////////////////////////////////////////////
class CBlender_light_occq : public IBlender
{
  public:
	virtual LPCSTR getComment()
	{
		return "INTERNAL: occlusion testing";
	}

	CBlender_light_occq()
	{
		description.CLS = 0;
	}

	~CBlender_light_occq() = default;

	void Compile(CBlender_Compile& C)
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
};
///////////////////////////////////////////////////////////////////////////////////
