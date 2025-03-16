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
	m_previous.mul(RenderImplementation.m_saved_viewproj, m_invview);
	m_current.set(Device.mProject);

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth * 0.25f);
	float h = float(Device.dwHeight * 0.25f);
	float Power = ps_r_mblur * 0.25f;

	// Create dilation map
	RenderBackend.set_Element(s_motion_blur->E[0]);
	RenderBackend.set_Constant("m_blur_power", Power);
	RenderBackend.set_Constant("m_current", m_current);
	RenderBackend.set_Constant("m_previous", m_previous);
	RenderViewportSurface(w, h, rt_Motion_Blur_Dilation_Map_0);

	// Blur (pass 1)
	set_Render_Target_Surface(rt_Motion_Blur_Dilation_Map_1);
	set_Depth_Buffer(NULL);
	RenderBackend.set_Element(s_motion_blur->E[1]);
	RenderBackend.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
	RenderViewportSurface(w, h, rt_Motion_Blur_Dilation_Map_1);

	// Blur (pass 2)
	RenderBackend.set_Element(s_motion_blur->E[2]);
	RenderBackend.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
	RenderViewportSurface(w, h, rt_Motion_Blur_Dilation_Map_0);
}

void CRenderTarget::phase_motion_blur_pass_blur()
{
	OPTICK_EVENT("CRenderTarget::phase_motion_blur_pass_blur");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	// Blur
	RenderBackend.set_Element(s_motion_blur->E[4]);
	RenderViewportSurface(rt_Generic_1);

	RenderBackend.set_Element(s_motion_blur->E[5]);
	RenderViewportSurface(rt_Generic_0);
}

void CRenderTarget::phase_motion_blur_pass_save_depth()
{
	OPTICK_EVENT("CRenderTarget::phase_motion_blur_pass_save_depth");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(s_motion_blur->E[3]);
	RenderViewportSurface(rt_Motion_Blur_Previous_Frame_Depth);
}

void CRenderTarget::phase_motion_blur()
{
	OPTICK_EVENT("CRenderTarget::phase_motion_blur");
	motion_blur_phase_prepare_dilation_map();
	phase_motion_blur_pass_blur();
	phase_motion_blur_pass_save_depth();
}
///////////////////////////////////////////////////////////////////////////////////
