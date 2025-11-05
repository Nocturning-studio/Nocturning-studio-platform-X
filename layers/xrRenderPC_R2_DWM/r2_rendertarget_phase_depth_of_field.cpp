///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r2_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
#pragma message(Reminder("Fix DoF"))
/* void CRenderTarget::depth_of_field_pass_poisson_filter()
{
	u_setrt(rt_Generic_0, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	// Set pass
	RCache.set_Element(s_dof->E[0]);

	Fvector3 dof;
	Fvector2 vDofKernel;
	g_pGamePersistent->GetCurrentDof(dof);
	vDofKernel.set(0.5f / Device.dwWidth, 0.5f / Device.dwHeight);
	vDofKernel.mul(ps_r2_dof_kernel_size);

	RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r2_dof_sky);
	RCache.set_c("dof_kernel", vDofKernel.x, vDofKernel.y, ps_r2_dof_kernel_size, 0);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Draw
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::depth_of_field_pass_separated_filter()
{
	u_setrt(rt_Generic_0, NULL, NULL, NULL);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	u32 C = color_rgba(0, 0, 0, 255);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	// Set geometry
	RCache.set_Geometry(g_combine);

	Fvector3 dof;
	Fvector2 vDofKernel;
	g_pGamePersistent->GetCurrentDof(dof);
	vDofKernel.set(0.5f / Device.dwWidth, 0.5f / Device.dwHeight);
	vDofKernel.mul(ps_r2_dof_kernel_size);

	for (u32 i = 0; i < s_dof->E[1]->passes.size(); i++)
	{
		RCache.set_Element(s_dof->E[1], i);
		RCache.set_c("dof_params", dof.x, dof.y, dof.z, ps_r2_dof_sky);
		RCache.set_c("dof_kernel", vDofKernel.x, vDofKernel.y, ps_r2_dof_kernel_size, 0);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}

void CRenderTarget::phase_depth_of_field()
{
	if (ps_r2_dof_quality <= 2)
	{
		depth_of_field_pass_poisson_filter();
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			depth_of_field_pass_separated_filter();
		}
	}
}*/
///////////////////////////////////////////////////////////////////////////////////
