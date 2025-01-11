///////////////////////////////////////////////////////////////////////////////////
// Author: Deathman
// Nocturning studio for NS Project X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "r_rendertarget.h"
///////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::motion_blur_phase_prepare_dilation_map()
{
	OPTICK_EVENT("CRenderTarget::motion_blur_phase_prepare_dilation_map");

	// (new-camera) -> (world) -> (old_viewproj)
	Fmatrix m_previous, m_current, m_invview;
	m_invview.invert(Device.mView);
	m_previous.mul(RImplementation.m_saved_viewproj, m_invview);
	m_current.set(Device.mProject);

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	u32 Offset = 0;
	float w = float(Device.dwWidth * 0.25f);
	float h = float(Device.dwHeight * 0.25f);

	set_viewport_vertex_buffer(w, h, Offset);

	u_setrt(rt_Motion_Blur_Dilation_Map_0, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_motion_blur->E[0]);
	RCache.set_c("m_blur_power", ps_r_mblur, 0, 0, 0);
	RCache.set_c("m_current", m_current);
	RCache.set_c("m_previous", m_previous);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	//if(0)//for (int i = 0; i < 1; i++)
	//{
		u_setrt(rt_Motion_Blur_Dilation_Map_1, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_motion_blur->E[1]);
		RCache.set_c("image_resolution", w, h, 1 / w, 1 / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Motion_Blur_Dilation_Map_0, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_motion_blur->E[2]);
		RCache.set_c("image_resolution", w, h, 1 / w, 1 / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	//}
}

void CRenderTarget::phase_motion_blur_pass_blur()
{
	OPTICK_EVENT("CRenderTarget::phase_motion_blur_pass_blur");

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;
	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	set_viewport_vertex_buffer(w, h, Offset);

	u_setrt(rt_Generic_1, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_motion_blur->E[4]);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	u_setrt(rt_Generic_0, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_motion_blur->E[5]);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

	u_setrt(rt_Motion_Blur_Previous_Frame_Depth, NULL, NULL, NULL, NULL);
	RCache.set_Element(s_motion_blur->E[3]);
	RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderTarget::phase_motion_blur()
{
	OPTICK_EVENT("CRenderTarget::phase_motion_blur");
	motion_blur_phase_prepare_dilation_map();
	phase_motion_blur_pass_blur();
}
///////////////////////////////////////////////////////////////////////////////////
