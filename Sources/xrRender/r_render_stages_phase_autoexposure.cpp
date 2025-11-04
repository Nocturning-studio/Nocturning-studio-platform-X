///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "blender_autoexposure.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::downsample_scene_luminance()
{
	OPTICK_EVENT("CRender::downsample_scene_luminance");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float input_w = float(Device.dwWidth);
	float input_h = float(Device.dwHeight);

	// Проход 0: полноразмерный -> 64x64 (максимальное размытие)
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_1], 0);
	RenderBackend.set_Constant("image_resolution", input_w, input_h, 1.0f / input_w, 1.0f / input_h);
	RenderBackend.RenderViewportSurface(64.0f, 64.0f, RenderTarget->rt_LUM_64);

	// Проход 1: 64x64 -> 32x32
	input_w = input_h = 64.0f;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_1], 1);
	RenderBackend.set_Constant("image_resolution", input_w, input_h, 1.0f / input_w, 1.0f / input_h);
	RenderBackend.RenderViewportSurface(32.0f, 32.0f, RenderTarget->rt_LUM_32);

	// Проход 2: 32x32 -> 16x16
	input_w = input_h = 32.0f;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_1], 2);
	RenderBackend.set_Constant("image_resolution", input_w, input_h, 1.0f / input_w, 1.0f / input_h);
	RenderBackend.RenderViewportSurface(16.0f, 16.0f, RenderTarget->rt_LUM_16);

	// Проход 3: 16x16 -> 8x8
	input_w = input_h = 16.0f;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_2], 0);
	RenderBackend.set_Constant("image_resolution", input_w, input_h, 1.0f / input_w, 1.0f / input_h);
	RenderBackend.RenderViewportSurface(8.0f, 8.0f, RenderTarget->rt_LUM_8);

	// Проход 4: 8x8 -> 4x4
	input_w = input_h = 8.0f;
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_2], 1);
	RenderBackend.set_Constant("image_resolution", input_w, input_h, 1.0f / input_w, 1.0f / input_h);
	RenderBackend.RenderViewportSurface(4.0f, 4.0f, RenderTarget->rt_LUM_4);

	// Проход 5: 4x4 -> 1x1 (финальное усреднение)
	input_w = input_h = 4.0f;

	float w = 1.0f;
	float h = 1.0f;

	// Параметры автоэкспозиции
	float TimeDelta = Device.fTimeDelta;
	float adaptation_speed = ps_r_autoexposure_adaptation;

	// Гарантируем разумные пределы
	adaptation_speed = std::max(0.01f, std::min(adaptation_speed, 10.0f));
	TimeDelta = std::min(TimeDelta, 0.033f);

	Fvector4 adaptation_params;
	adaptation_params.set(adaptation_speed, TimeDelta, 0.0f, 0.0f);

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DOWNSAMPLE_PART_2], 2);
	RenderBackend.set_Constant("image_resolution", input_w, input_h, 1.0f / input_w, 1.0f / input_h);
	RenderBackend.set_Constant("adaptation_params", adaptation_params);
	RenderBackend.RenderViewportSurface(1.0f, 1.0f, RenderTarget->rt_SceneLuminance);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::save_scene_luminance()
{
	OPTICK_EVENT("CRender::save_scene_luminance");

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_SAVE_LUMINANCE], 0); // Pass 0
	RenderBackend.RenderViewportSurface(1, 1, RenderTarget->rt_SceneLuminancePrevious);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::apply_exposure()
{
	OPTICK_EVENT("CRender::apply_exposure");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float w = float(Device.dwWidth);
	float h = float(Device.dwHeight);

	// Параметры автоэкспозиции
	float TimeDelta = Device.fTimeDelta;
	float adaptation_speed = ps_r_autoexposure_adaptation;
	float amount = ps_r_autoexposure_amount;

	Fvector3 _none, _full, _result;
	_none.set(1, 0, 1);
	_full.set(ps_r_autoexposure_middlegray, 1.f, ps_r_autoexposure_low_lum);
	_result.lerp(_none, _full, amount);

	// Передаем параметры с увеличенной точностью
	Fvector4 adaptation_params;
	adaptation_params.set(_result.x, _result.z, adaptation_speed, TimeDelta);

	// Применяем экспозицию
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_APPLY_EXPOSURE]);
	RenderBackend.set_Constant("autoexposure_params", adaptation_params);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Generic_0);

	// Копируем результат обратно в основной буфер
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_DUMMY]);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Generic_1);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_autoexposure()
{
	OPTICK_EVENT("CRender::render_autoexposure");

	save_scene_luminance();

	downsample_scene_luminance();

	apply_exposure();
}
///////////////////////////////////////////////////////////////////////////////////

