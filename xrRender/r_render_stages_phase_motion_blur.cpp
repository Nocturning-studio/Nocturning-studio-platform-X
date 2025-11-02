///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_motion_blur.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::motion_blur_pass_prepare_dilation_map()
{
	OPTICK_EVENT("CRenderTarget::motion_blur_pass_prepare_dilation_map");

	// (new-camera) -> (world) -> (old_viewproj)
	Fmatrix m_previous, m_current, m_invview;
	m_invview.invert(Device.mView);
	m_previous.mul(RenderImplementation.m_saved_viewproj, m_invview);
	m_current.set(Device.mProject);

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth * 0.5f);
	float h = float(Device.dwHeight * 0.5f);

	RenderBackend.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_PREPARE_DILATION_MAP]);
	RenderBackend.set_Constant("m_blur_power", ps_r_mblur);
	RenderBackend.set_Constant("m_current", m_current);
	RenderBackend.set_Constant("m_previous", m_previous);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Motion_Blur_Dilation_Map_0);

	for (int i = 0; i < 1; i++)
	{
		RenderBackend.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_BLUR_DILATION_MAP], 0);
		RenderBackend.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
		RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Motion_Blur_Dilation_Map_1);

		RenderBackend.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_BLUR_DILATION_MAP], 1);
		RenderBackend.set_Constant("image_resolution", w, h, 1 / w, 1 / h);
		RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Motion_Blur_Dilation_Map_0);
	}
}

void CRender::motion_blur_pass_blur()
{
	OPTICK_EVENT("CRenderTarget::motion_blur_pass_blur");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_BLUR_FRAME], 0);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0);

	RenderBackend.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_BLUR_FRAME], 1);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);
}

void CRender::motion_blur_pass_save_depth()
{
	OPTICK_EVENT("CRenderTarget::motion_blur_pass_save_depth");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	RenderBackend.set_Element(RenderTarget->s_motion_blur->E[SE_PASS_SAVE_DEPTH_BUFFER]);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Motion_Blur_Previous_Frame_Depth);
}

void CRender::render_motion_blur()
{
	OPTICK_EVENT("CRenderTarget::render_motion_blur");
	motion_blur_pass_prepare_dilation_map();
	motion_blur_pass_blur();
	motion_blur_pass_save_depth();
}
///////////////////////////////////////////////////////////////////////////////////
