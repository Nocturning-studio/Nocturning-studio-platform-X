///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <Blender_depth_of_field.h>
///////////////////////////////////////////////////////////////////////////////////
constexpr double x = 43.266615300557; // Diagonal measurement for a 'normal' 35mm lens
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
	float DofFocalLength = fov_to_length(Device.fFOV);
	g_pGamePersistent->GetCurrentDof(Dof);

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
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);

	for (int i = 0; i < IterationsNum; i++)
	{
		int ShaderPass = ps_r_dof_quality > 2 ? SE_PASS_PROCESS_BOKEH_HQ : SE_PASS_PROCESS_BOKEH_LQ;
		RenderBackend.set_Element(RenderTarget->s_dof->E[ShaderPass], 0);
		RenderBackend.set_Constant("dof_params", Dof.x, DofFocalLength, Dof.z, 1.0f);
		RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0);

		ShaderPass = ps_r_dof_quality > 1 ? SE_PASS_PROCESS_BOKEH_HQ : SE_PASS_DOF_DUMMY;
		RenderBackend.set_Element(RenderTarget->s_dof->E[ShaderPass], 1);
		RenderBackend.set_Constant("dof_params", Dof.x, DofFocalLength, Dof.z, 1.0f);
		RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);
	}
}
///////////////////////////////////////////////////////////////////////////////////
