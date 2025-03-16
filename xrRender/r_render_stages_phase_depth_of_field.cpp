///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::phase_depth_of_field()
{
	OPTICK_EVENT("CRenderTarget::phase_depth_of_field");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	u32 Offset = 0;
	set_viewport_geometry(Offset);

	Fvector3 Dof;
	float DofDiaphragm;
	g_pGamePersistent->GetCurrentDof(Dof);
	g_pGamePersistent->GetDofDiaphragm(DofDiaphragm);

	for (int i = 0; i < 1; i++)
	{
		set_Render_Target_Surface(rt_Generic_0);
		set_Depth_Buffer(NULL);
		RenderBackend.set_Element(s_dof->E[0]);
		RenderBackend.set_Constant("dof_params", Dof.x, Dof.y, Dof.z, ps_r_dof_sky);
		RenderBackend.set_Constant("dof_diaphragm", DofDiaphragm);
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		set_Render_Target_Surface(rt_Generic_1);
		set_Depth_Buffer(NULL);
		RenderBackend.set_Element(s_dof->E[1]);
		RenderBackend.set_Constant("dof_params", Dof.x, Dof.y, Dof.z, ps_r_dof_sky);
		RenderBackend.set_Constant("dof_diaphragm", DofDiaphragm);
		RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
///////////////////////////////////////////////////////////////////////////////////
