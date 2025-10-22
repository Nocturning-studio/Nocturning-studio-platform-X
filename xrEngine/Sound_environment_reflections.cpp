#include "stdafx.h"
#pragma hdrstop

#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_area.h"
#include "xr_object.h"
#include "xr_collide_defs.h"
#endif

#include "Sound_environment.h"
#include "Sound_environment_reflections.h"
///////////////////////////////////////////////////////////////////////////////////
// Advanced Reflection Analysis Implementation
///////////////////////////////////////////////////////////////////////////////////

const float CAdvancedReflectionAnalyzer::MAX_REFLECTION_DISTANCE = 10000.0f;

// Усовершенствованная выборка для анализа отражений
static const Fvector3 ReflectionAnalysisDirections[] = {
	// Основные направления (более плотная выборка)
	{0.000000, 0.000000, 1.000000},
	{0.382683, 0.000000, 0.923880},
	{0.000000, 0.382683, 0.923880},
	{-0.382683, 0.000000, 0.923880},
	{0.000000, -0.382683, 0.923880},
	{0.707107, 0.000000, 0.707107},
	{0.000000, 0.707107, 0.707107},
	{-0.707107, 0.000000, 0.707107},
	{0.000000, -0.707107, 0.707107},
	{0.923880, 0.000000, 0.382683},
	{0.000000, 0.923880, 0.382683},
	{-0.923880, 0.000000, 0.382683},
	{0.000000, -0.923880, 0.382683},

	// Диагональные направления
	{0.577350, 0.577350, 0.577350},
	{0.577350, -0.577350, 0.577350},
	{-0.577350, 0.577350, 0.577350},
	{-0.577350, -0.577350, 0.577350},
	{0.356822, 0.934172, 0.000000},
	{0.934172, 0.356822, 0.000000},
	{-0.356822, 0.934172, 0.000000},
	{-0.934172, 0.356822, 0.000000}};

static const u32 REFLECTION_ANALYSIS_DIRECTIONS = sizeof(ReflectionAnalysisDirections) / sizeof(Fvector3);

///////////////////////////////////////////////////////////////////////////////////

Fvector CAdvancedReflectionAnalyzer::CalculateReflectedDirection(Fvector incident, Fvector normal)
{
	// Простое отражение (можно улучшить с учетом шероховатости)
	Fvector reflected;
	reflected.reflect(incident, normal);
	return reflected;
}

void CAdvancedReflectionAnalyzer::AnalyzeReflections(Fvector start_pos, SReflectionAnalysis& result)
{
	ZeroMemory(&result, sizeof(result));

	float total_primary_energy = 0.0f;
	float total_secondary_energy = 0.0f;
	float total_delay = 0.0f;
	u32 valid_paths = 0;
	float avg_distance = 0.0f;

	for (u32 i = 0; i < _min(16u, REFLECTION_ANALYSIS_DIRECTIONS); ++i)
	{
		Fvector ray_dir = ReflectionAnalysisDirections[i];
		float path_delay = 0.0f;

		// ПРОХОДИМ ТОЛЬКО ПЕРВИЧНЫЕ ОТРАЖЕНИЯ для анализа
		float path_energy = TraceReflectionPath(start_pos, ray_dir, 1, 1.0f, path_delay);

		collide::rq_result first_hit;
		CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();
		BOOL bFirstHit = g_pGameLevel->ObjectSpace.RayPick(start_pos, ray_dir, MAX_REFLECTION_DISTANCE,
														   collide::rqtStatic, first_hit, ViewEntity);

		if (bFirstHit && first_hit.range > 0.1f)
		{
			avg_distance += first_hit.range;

			// ЭНЕРГИЯ ОТРАЖЕНИЙ СИЛЬНО ЗАВИСИТ ОТ РАССТОЯНИЯ
			float distance_factor = 1.0f - (first_hit.range / 50.0f);
			clamp(distance_factor, 0.05f, 1.0f);

			if (path_energy > 0.001f)
			{
				// ОСНОВНОЕ ИСПРАВЛЕНИЕ: энергия сильно зависит от расстояния
				total_primary_energy += path_energy * distance_factor * 0.3f;
				total_delay += path_delay;
				valid_paths++;
			}
		}
	}

	if (valid_paths > 0)
	{
		avg_distance /= valid_paths;

		// КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: энергия отражений ОБРАТНО пропорциональна среднему расстоянию
		float distance_attenuation = 1.0f - (avg_distance / 100.0f);
		clamp(distance_attenuation, 0.01f, 1.0f);

		result.fPrimaryReflections = (total_primary_energy / valid_paths) * distance_attenuation;
		result.fSecondaryReflections = result.fPrimaryReflections * 0.3f; // Вторичные = 30% от первичных
		result.fReflectionDelay = total_delay / valid_paths;
		result.fReflectionEnergy = result.fPrimaryReflections * 0.8f + result.fSecondaryReflections * 0.2f;
		result.fSurfaceComplexity = float(valid_paths) / 16.0f;

		// ДОПОЛНИТЕЛЬНОЕ ОГРАНИЧЕНИЕ ДЛЯ ОТКРЫТЫХ ПРОСТРАНСТВ
		if (avg_distance > 25.0f)
		{
			float open_space_reduction = 1.0f - ((avg_distance - 25.0f) / 75.0f);
			clamp(open_space_reduction, 0.05f, 1.0f);

			result.fReflectionEnergy *= open_space_reduction;
			result.fPrimaryReflections *= open_space_reduction;
			result.fSecondaryReflections *= open_space_reduction;
		}
	}
	else
	{
		// ОЧЕНЬ НИЗКИЕ значения для открытых пространств
		result.fPrimaryReflections = 0.02f;
		result.fSecondaryReflections = 0.005f;
		result.fReflectionDelay = 0.01f;
		result.fReflectionEnergy = 0.01f;
		result.fSurfaceComplexity = 0.02f;
	}

	// СТРОГИЕ ОГРАНИЧЕНИЯ
	clamp(result.fPrimaryReflections, 0.0f, 0.3f);
	clamp(result.fSecondaryReflections, 0.0f, 0.1f);
	clamp(result.fReflectionDelay, 0.005f, 0.05f);
	clamp(result.fReflectionEnergy, 0.005f, 0.25f);
	clamp(result.fSurfaceComplexity, 0.0f, 0.5f);
}

float CAdvancedReflectionAnalyzer::TraceReflectionPath(Fvector start, Fvector dir, u32 max_bounces, float energy_decay,
													   float& total_delay)
{
	if (max_bounces == 0 || energy_decay < 0.01f)
		return 0.0f;

	collide::rq_result hit;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	BOOL bHit =
		g_pGameLevel->ObjectSpace.RayPick(start, dir, MAX_REFLECTION_DISTANCE, collide::rqtStatic, hit, ViewEntity);

	if (!bHit || hit.range <= 0.1f)
		return 0.0f;

	float distance = hit.range;
	total_delay += distance / 340.0f;

	// СИЛЬНАЯ ЗАВИСИМОСТЬ ОТРАЖАТЕЛЬНОЙ СПОСОБНОСТИ ОТ РАССТОЯНИЯ
	float distance_factor = 1.0f - (distance / 100.0f);
	clamp(distance_factor, 0.05f, 1.0f);

	float surface_reflectivity = 0.6f * distance_factor;
	float reflected_energy = energy_decay * surface_reflectivity;

	if (reflected_energy < 0.005f)
		return reflected_energy;

	if (max_bounces > 1)
	{
		Fvector surface_normal;
		surface_normal.random_dir();
		Fvector reflected_dir;
		reflected_dir.reflect(dir, surface_normal);

		Fvector new_start;
		new_start.mad(start, dir, distance + 0.05f);

		// СИЛЬНОЕ ЗАТУХАНИЕ ДЛЯ ВТОРИЧНЫХ ОТРАЖЕНИЙ
		float secondary_energy =
			TraceReflectionPath(new_start, reflected_dir, max_bounces - 1, reflected_energy * 0.2f, total_delay);
		return reflected_energy + secondary_energy;
	}

	return reflected_energy;
}

float CAdvancedReflectionAnalyzer::EstimateSurfaceReflectivity(collide::rq_result& hit)
{
	// ПЕРЕСМОТРЕННАЯ оценка отражательной способности
	float distance_factor = 1.0f - (hit.range / MAX_REFLECTION_DISTANCE);
	clamp(distance_factor, 0.2f, 1.0f);

	// Близкие поверхности - более отражающие, дальние - менее
	float base_reflectivity = 0.7f - (distance_factor * 0.3f);

	return base_reflectivity;
}