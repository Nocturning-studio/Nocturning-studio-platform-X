///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "blender_autoexposure.h"
///////////////////////////////////////////////////////////////////////////////////
void CRender::save_scene_luminance()
{
	OPTICK_EVENT("CRender::save_scene_luminance");

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_SAVE_LUMINANCE]);
	RenderBackend.RenderViewportSurface(1, 1, RenderTarget->rt_SceneLuminancePrevious);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::downsample_scene_luminance()
{
	OPTICK_EVENT("CRender::downsample_scene_luminance");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	// Копируем исходное изображение в уровень 0
	ref_rt MipChain = RenderTarget->rt_LUM_Mip_Chain;
	IDirect3DSurface9* dst_level0 = MipChain->get_surface_level(0);
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN], 0);
	RenderBackend.RenderViewportSurface((LONG)MipChain->dwWidth, (LONG)MipChain->dwHeight, dst_level0);

	// Генерируем остальные mip-уровни
	for (u32 i = 1; i < MipChain->get_levels_count(); i++)
	{
		// Устанавливаем шейдер
		RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_GENERATE_MIP_CHAIN], 1);

		u32 prev_mip, prev_mip_width, prev_mip_height;
		prev_mip = i - 1;
		MipChain->get_level_desc(prev_mip, prev_mip_width, prev_mip_height);
		RenderBackend.set_Constant("mip_data", 0.0f, prev_mip, 1.0f / prev_mip_width, 1.0f / prev_mip_height);

		// Рендерим
		u32 width, height;
		MipChain->get_level_desc(i, width, height);
		IDirect3DSurface9* mip_surface = MipChain->get_surface_level(i);
		RenderBackend.RenderViewportSurface((float)width, (float)height, mip_surface);

		mip_surface->Release();
	}
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::prepare_scene_luminance()
{
	// Параметры автоэкспозиции
	float TimeDelta = Device.fTimeDelta;
	float adaptation_speed = ps_r_autoexposure_adaptation;

	// Гарантируем разумные пределы
	adaptation_speed = std::max(0.01f, std::min(adaptation_speed, 10.0f));
	TimeDelta = std::min(TimeDelta, 0.033f);

	Fvector4 adaptation_params;
	adaptation_params.set(adaptation_speed, TimeDelta, 0.0f, 0.0f);

	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_PREPARE_LUMINANCE]);
	RenderBackend.set_Constant("adaptation_params", adaptation_params);
	RenderBackend.RenderViewportSurface(1.0f, 1.0f, RenderTarget->rt_SceneLuminance);
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

	u32 mip_width, mip_height;
	RenderTarget->rt_LUM_Mip_Chain->get_level_desc(8, mip_width, mip_height);
	RenderBackend.set_Constant("mip_data", 0.0f, 8, 1.0f / mip_width, 1.0f / mip_height);

	// Применяем экспозицию
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_APPLY_EXPOSURE]);
	RenderBackend.set_Constant("autoexposure_params", adaptation_params);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Generic_0);

	// Копируем результат обратно в основной буфер
	RenderBackend.set_Element(RenderTarget->s_autoexposure->E[SE_PASS_AUTOEXPOSURE_COPY_RENDERTARGET]);
	RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Generic_1);
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_autoexposure()
{
	OPTICK_EVENT("CRender::render_autoexposure");

	save_scene_luminance();
	downsample_scene_luminance();
	prepare_scene_luminance();
	apply_exposure();
}
///////////////////////////////////////////////////////////////////////////////////
