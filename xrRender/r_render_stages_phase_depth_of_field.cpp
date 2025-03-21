///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_depth_of_field()
{
	OPTICK_EVENT("CRender::render_depth_of_field");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	Fvector3 Dof;
	float DofDiaphragm;
	g_pGamePersistent->GetCurrentDof(Dof);
	g_pGamePersistent->GetDofDiaphragm(DofDiaphragm);

	for (int i = 0; i < 1; i++)
	{
		RenderBackend.set_Element(RenderTarget->s_dof->E[0]);
		RenderBackend.set_Constant("dof_params", Dof.x, Dof.y, Dof.z, ps_r_dof_sky);
		RenderBackend.set_Constant("dof_diaphragm", DofDiaphragm);
		RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0);

		RenderBackend.set_Element(RenderTarget->s_dof->E[1]);
		RenderBackend.set_Constant("dof_params", Dof.x, Dof.y, Dof.z, ps_r_dof_sky);
		RenderBackend.set_Constant("dof_diaphragm", DofDiaphragm);
		RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);
	}
}
///////////////////////////////////////////////////////////////////////////////////
