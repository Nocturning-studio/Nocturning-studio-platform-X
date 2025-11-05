#include "stdafx.h"
#pragma hdrstop

#include "blender_Editor_Selection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBlender_Editor_Selection::CBlender_Editor_Selection()
{
	description.CLS		= B_EDITOR_SEL;
	xr_strcpy				(oT_Factor,"$null");
}

CBlender_Editor_Selection::~CBlender_Editor_Selection()
{
	
}

void	CBlender_Editor_Selection::Save	( IWriter& fs	)
{
	IBlender::Save	(fs);
	xrPWRITE_PROP	(fs,"TFactor",	xrPID_CONSTANT, oT_Factor);
}

void	CBlender_Editor_Selection::Load	( IReader& fs, u16 version	)
{
	IBlender::Load	(fs,version);
	xrPREAD_PROP	(fs,xrPID_CONSTANT,	oT_Factor);
}

void	CBlender_Editor_Selection::Compile	(CBlender_Compile& C)
{
	IBlender::Compile		(C);	
	{
		C.r_Pass	("editor","simple_color",FALSE,TRUE,FALSE,TRUE, D3D_BLEND_SRC_ALPHA, D3D_BLEND_INV_SRC_ALPHA);
		C.r_End		();
	}
}
