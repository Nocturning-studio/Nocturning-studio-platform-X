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

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	set_viewport_vertex_buffer(w, h, Offset);

	Fvector3 dof;
	Fvector2 vDofKernel;
	float DofDiaphragm;
	g_pGamePersistent->GetCurrentDof(dof);
	g_pGamePersistent->GetDofDiaphragm(DofDiaphragm);
	vDofKernel.mul(ps_r_dof_diaphragm_size);

	for (int i = 0; i < 1; i++)
	{
		u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_dof->E[0]);
		RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r_dof_sky);
		RCache.set_c("dof_diaphragm", DofDiaphragm, 0, 0, 0);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_dof->E[1]);
		RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r_dof_sky);
		RCache.set_c("dof_diaphragm", DofDiaphragm, 0, 0, 0);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
///////////////////////////////////////////////////////////////////////////////////
