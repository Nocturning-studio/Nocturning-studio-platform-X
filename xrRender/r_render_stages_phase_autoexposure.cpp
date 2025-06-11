///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <blender_autoexposure.h>
///////////////////////////////////////////////////////////////////////////////////
void CRender::downsample_scene_luminance()
{
	OPTICK_EVENT("CRender::downsample_scene_luminance");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_1], 0);
	RenderBackend.set_Constant("image_resolution", w, h, 1.0f / w, 1.0f / h);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_LUM_512);

	w = h = 256.0f;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_1], 1);
	RenderBackend.set_Constant("image_resolution", w, h, 1.0f / w, 1.0f / h);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_LUM_256);

	w = h = 128;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_1], 2);
	RenderBackend.set_Constant("image_resolution", w, h, 1.0f / w, 1.0f / h);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_LUM_128);

	w = h = 64;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_2], 0);
	RenderBackend.set_Constant("image_resolution", w, h, 1.0f / w, 1.0f / h);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_LUM_64);

	w = h = 8;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_2], 1);
	RenderBackend.set_Constant("image_resolution", w, h, 1.0f / w, 1.0f / h);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_SceneLuminance);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::save_scene_luminance()
{
	OPTICK_EVENT("CRender::save_scene_luminance");

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_SAVE_LUMINANCE], 0);
	RenderBackend.RenderViewportSurface(8.0f, 8.0f, RenderTarget->rt_SceneLuminancePrevious);
}
///////////////////////////////////////////////////////////////////////////////////
float autoexposure_adapation = 0.5f;
///////////////////////////////////////////////////////////////////////////////////
void CRender::apply_exposure()
{
	OPTICK_EVENT("CRender::apply_exposure");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	autoexposure_adapation = .9f * autoexposure_adapation + .1f * Device.fTimeDelta * ps_r_autoexposure_adaptation;
	float amount = ps_r_autoexposure_amount;
	Fvector3 _none, _full, _result;
	_none.set(1, 0, 1);
	_full.set(ps_r_autoexposure_middlegray, 1.f, ps_r_autoexposure_low_lum);
	_result.lerp(_none, _full, amount);

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_APPLY_EXPOSURE]);
	RenderBackend.set_Constant("middle_gray", _result.x, _result.y, _result.z, autoexposure_adapation);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Generic_1);

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DUMMY]);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Generic_0);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_autoexposure()
{
	OPTICK_EVENT("CRender::render_autoexposure");

	//downsample_scene_luminance();

	//apply_exposure();

	//save_scene_luminance();
}
///////////////////////////////////////////////////////////////////////////////////

