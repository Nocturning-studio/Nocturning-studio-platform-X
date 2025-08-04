///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <Blender_depth_of_field.h>
///////////////////////////////////////////////////////////////////////////////////
constexpr double x = 43.266615300557; // Diagonal measurement for a 'normal' 35mm lens
constexpr float FocalDepthMultiplier = 1000.0f;
constexpr float CoC = 0.03f; // circle of confusion size in mm (35mm film = 0.03mm)
///////////////////////////////////////////////////////////////////////////////////
double fov_to_length(double fov)
{
	if (fov < 1 || fov > 179)
		return NULL;

	return (x / (2 * tan(M_PI * fov / 360.f)));
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_depth_of_field()
{
	OPTICK_EVENT("CRender::render_depth_of_field");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	Fvector3 Dof;
	g_pGamePersistent->GetCurrentDof(Dof);

	float FocalLength = fov_to_length(Device.fFOV);
	float FocalPlane = Dof.x * FocalDepthMultiplier;
	float COCPrecalc1 = FocalPlane * FocalLength;
	float COCPrecalc2 = FocalPlane - FocalLength;
	float COCPrecalc3 = FocalPlane * Dof.z * CoC;
	float B = COCPrecalc1 / COCPrecalc2;
	float C = COCPrecalc1 / COCPrecalc3;
	float CPremultiplied = C * CoC;

	int IterationsNum = 1;

	/*
	switch (ps_r_dof_quality)
	{
	case 1:
		IterationsNum = 1;
		break;
	case 2:
		IterationsNum = 1;
		break;
	case 3:
		IterationsNum = 2;
		break;
	}
	*/

	RenderBackend.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_PREPARE_BUFFER]);
	//RenderBackend.set_Constant("dof_params", Dof.x, FocalLength, Dof.z, CoC);
	RenderBackend.set_Constant("dof_coc_precalculated", FocalPlane, B, CPremultiplied, FocalLength);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);

	for (int i = 0; i < IterationsNum; i++)
	{
		int ShaderPass = ps_r_dof_quality > 2 ? SE_PASS_PROCESS_BOKEH_HQ : SE_PASS_PROCESS_BOKEH_LQ;
		RenderBackend.set_Element(RenderTarget->s_dof->E[ShaderPass], 0);
		RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0);

		ShaderPass = ps_r_dof_quality > 1 ? SE_PASS_PROCESS_BOKEH_HQ : SE_PASS_DOF_DUMMY;
		RenderBackend.set_Element(RenderTarget->s_dof->E[ShaderPass], 1);
		RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);
	}
}
///////////////////////////////////////////////////////////////////////////////////
