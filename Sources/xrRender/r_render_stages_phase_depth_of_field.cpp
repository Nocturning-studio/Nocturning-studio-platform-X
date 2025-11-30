///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Blender_depth_of_field.h"
///////////////////////////////////////////////////////////////////////////////////
constexpr double x = 43.266615300557; // Diagonal measurement for a 'normal' 35mm lens
constexpr float FocalDepthMultiplier = 1000.0f;
constexpr float CoC = 0.03f; // circle of confusion size in mm (35mm film = 0.03mm)
///////////////////////////////////////////////////////////////////////////////////
double fov_to_length(double fov)
{
	if (fov < 1 || fov > 179)
		return NULL;

	return (x / (2 * tan(M_PI * fov / 360.f)));
}
///////////////////////////////////////////////////////////////////////////////////
void CRender::render_depth_of_field()
{
	OPTICK_EVENT("CRender::render_depth_of_field");

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	Fvector3 Dof;
	g_pGamePersistent->GetCurrentDof(Dof);
	// Dof.x = Focus Distance (Game Units usually meters)
	// Dof.y = Focus Range (Not used in physical model directly)
	// Dof.z = Aperture (F-Stop)

	float FocalLength = fov_to_length(Device.fFOV);

	// Конвертация дистанции фокуса в мм (если Dof.x в метрах)
	float FocalPlane = Dof.x * 1000.0f; // FocalDepthMultiplier

	// Безопасность
	if (FocalPlane < FocalLength + 10.0f)
		FocalPlane = FocalLength + 10.0f;

	float COCPrecalc1 = FocalPlane * FocalLength;
	float COCPrecalc2 = FocalPlane - FocalLength;
	float SafeAperture = std::max(Dof.z, 1.4f);			 // F1.4 min
	float COCPrecalc3 = FocalPlane * SafeAperture * CoC; // CoC = 0.03f

	float B = COCPrecalc1 / COCPrecalc2; // (d*f)/(d-f)
	float C = COCPrecalc1 / COCPrecalc3; // (d*f)/(d*F*c) -> f / (F*c) ? Нет, формула (d-f)/(d*F*c)

	// Формула C для шейдера:
	// CoC = abs(a - b) * c
	// a = (o*f)/(o-f)
	// b = (d*f)/(d-f)
	// c = (d-f)/(d*F*c) -- Обычно это Scaling factor

	// Передаем параметры
	// x: FocalPlane (unused in shader optimization but passed)
	// y: B (Image distance of focused object)
	// z: C (CoC scaling factor)
	// w: FocalLength

	// Используем B и CPremultiplied
	float CPremultiplied = C * CoC;
	// В шейдере: return abs(a - b) * c;

	RenderBackend.set_Element(RenderTarget->s_dof->E[SE_PASS_DOF_PREPARE_BUFFER]);
	RenderBackend.set_Constant("dof_coc_precalculated", FocalPlane, B, CPremultiplied, FocalLength);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);

	RenderBackend.set_Element(RenderTarget->s_dof->E[SE_PASS_PROCESS_BOKEH_HQ], 0);
	RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_0);

	// Если все же нужно 2 прохода (для очень большого радиуса):
	// RenderBackend.set_Element(RenderTarget->s_dof->E[SE_PASS_PROCESS_BOKEH_HQ], 1);
	// RenderBackend.RenderViewportSurface(RenderTarget->rt_Generic_1);
}
///////////////////////////////////////////////////////////////////////////////////
