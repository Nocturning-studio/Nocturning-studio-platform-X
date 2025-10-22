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

#include "Sound_environment_geometry.h"

void CGeometryAnalyzer::AnalyzeEnvironmentGeometry(Fvector start_pos, SGeometryAnalysis& result)
{
	ZeroMemory(&result, sizeof(result));

	std::vector<Fvector> hit_points;
	std::vector<float> horizontal_distances;
	std::vector<float> vertical_distances;

	Fvector vUp = {0, 1, 0};
	Fvector vDown = {0, -1, 0};

	// Трассировка вертикалей для определения высоты
	collide::rq_result hit_up, hit_down;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	// Луч вверх для определения высоты потолка
	BOOL bHitUp = g_pGameLevel->ObjectSpace.RayPick(start_pos, vUp, 50.0f, collide::rqtStatic, hit_up, ViewEntity);
	// Луч вниз для определения высоты пола
	BOOL bHitDown =
		g_pGameLevel->ObjectSpace.RayPick(start_pos, vDown, 10.0f, collide::rqtStatic, hit_down, ViewEntity);

	float ceiling_height = bHitUp ? hit_up.range : 50.0f;
	float floor_depth = bHitDown ? hit_down.range : 10.0f;

	result.fAvgCeilingHeight = (ceiling_height + floor_depth) * 0.5f;

	// Сбор данных по всем направлениям
	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		Fvector ray_dir = DetailedSphereDirections[i];
		collide::rq_result hit;

		BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start_pos, ray_dir, 100.0f, collide::rqtStatic, hit, ViewEntity);

		if (bHit && hit.range > 0.1f)
		{
			Fvector hit_point;
			hit_point.mad(start_pos, ray_dir, hit.range);
			hit_points.push_back(hit_point);

			// Классификация расстояний
			if (fabs(ray_dir.y) < 0.3f) // Горизонтальные лучи
			{
				horizontal_distances.push_back(hit.range);
			}
			else if (ray_dir.y > 0.7f) // Вертикальные лучи вверх
			{
				vertical_distances.push_back(hit.range);
			}
		}
	}

	// Расчет средних расстояний
	float total_horizontal = 0.0f;
	for (u32 i = 0; i < horizontal_distances.size(); ++i)
	{
		total_horizontal += horizontal_distances[i];
	}
	result.fAvgWallDistance = horizontal_distances.size() > 0 ? total_horizontal / horizontal_distances.size() : 10.0f;

	// Расчет сложности геометрии
	if (hit_points.size() > 10)
	{
		result.vMainAxis = EstimateMainAxis(hit_points);

		// Простая оценка объема на основе собранных расстояний
		float avg_distance = 0.0f;
		for (u32 i = 0; i < hit_points.size(); ++i)
		{
			avg_distance += start_pos.distance_to(hit_points[i]);
		}
		avg_distance /= hit_points.size();

		// Предполагаем сферическую форму для оценки объема
		result.fApproxVolume = (4.0f / 3.0f) * PI * avg_distance * avg_distance * avg_distance;

		// Оценка сложности на основе разброса расстояний
		float variance = 0.0f;
		for (u32 i = 0; i < hit_points.size(); ++i)
		{
			float dist = start_pos.distance_to(hit_points[i]);
			float diff = dist - avg_distance;
			variance += diff * diff;
		}
		variance /= hit_points.size();

		result.fComplexity = _min(1.0f, variance / (avg_distance * 0.5f));
		result.fVerticality =
			vertical_distances.size() > 0 ? (result.fAvgCeilingHeight / (result.fAvgWallDistance + 0.1f)) : 0.5f;
		clamp(result.fVerticality, 0.1f, 2.0f);
	}
	else
	{
		// Открытое пространство
		result.fApproxVolume = 10000.0f;
		result.fComplexity = 0.1f;
		result.fVerticality = 1.0f;
		result.fAvgWallDistance = 50.0f;
	}

	// Нормализация объема для расчетов
	result.fApproxVolume = _min(result.fApproxVolume, 50000.0f);
}

Fvector CGeometryAnalyzer::EstimateMainAxis(const std::vector<Fvector>& hit_points)
{
	// Простой анализ главных направлений
	Fvector avg_direction = {0, 0, 0};

	for (u32 i = 0; i < hit_points.size(); ++i)
	{
		Fvector dir;
		dir.sub(hit_points[i], Device.vCameraPosition);
		dir.normalize_safe();
		avg_direction.add(dir);
	}

	if (hit_points.size() > 0)
	{
		avg_direction.mul(1.0f / hit_points.size());
		avg_direction.normalize_safe();
	}

	return avg_direction;
}