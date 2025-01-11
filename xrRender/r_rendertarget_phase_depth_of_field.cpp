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
	g_pGamePersistent->GetCurrentDof(dof);
	vDofKernel.set(0.5f / Device.dwWidth, 0.5f / Device.dwHeight);
	vDofKernel.mul(ps_r_dof_kernel_size);

	for (int i = 0; i < 4; i++)
	{
		u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_dof->E[0]);
		RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r_dof_sky);
		RCache.set_c("dof_kernel", vDofKernel.x, vDofKernel.y, ps_r_dof_kernel_size, 0);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_dof->E[1]);
		RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r_dof_sky);
		RCache.set_c("dof_kernel", vDofKernel.x, vDofKernel.y, ps_r_dof_kernel_size, 0);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
///////////////////////////////////////////////////////////////////////////////////
