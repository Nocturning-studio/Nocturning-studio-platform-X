///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_area.h"
#include "xr_object.h"
#include "xr_collide_defs.h"
#endif
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_geometry.h"
///////////////////////////////////////////////////////////////////////////////////
Fvector CGeometryAnalyzer::EstimateMainAxis(const std::vector<Fvector>& hit_points)
{
	// Simple main direction analysis
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

void CGeometryAnalyzer::AnalyzeEnvironmentGeometry(Fvector start_pos, SGeometryAnalysis& result)
{
	ZeroMemory(&result, sizeof(result));

	std::vector<Fvector> hit_points;
	std::vector<float> horizontal_distances;
	std::vector<float> vertical_distances;

	// Улучшенный анализ с большим количеством лучей
	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		Fvector ray_dir = DetailedSphereDirections[i];
		collide::rq_result hit;
		CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

		BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start_pos, ray_dir, 200.0f, collide::rqtStatic, hit, ViewEntity);

		if (bHit && hit.range > 0.1f && hit.range < 195.0f)
		{
			Fvector hit_point;
			hit_point.mad(start_pos, ray_dir, hit.range);
			hit_points.push_back(hit_point);

			// Классификация расстояний
			if (fabs(ray_dir.y) < 0.3f)
			{
				horizontal_distances.push_back(hit.range);
			}
			else if (fabs(ray_dir.y) > 0.7f)
			{
				vertical_distances.push_back(hit.range);
			}
		}
	}

	// Улучшенный расчет объема и сложности
	if (hit_points.size() > 15)
	{
		// Расчет приблизительного объема через выпуклую оболочку
		float min_x = 1e10f, max_x = -1e10f;
		float min_y = 1e10f, max_y = -1e10f;
		float min_z = 1e10f, max_z = -1e10f;

		for (const Fvector& point : hit_points)
		{
			min_x = _min(min_x, point.x);
			max_x = _max(max_x, point.x);
			min_y = _min(min_y, point.y);
			max_y = _max(max_y, point.y);
			min_z = _min(min_z, point.z);
			max_z = _max(max_z, point.z);
		}

		float width = max_x - min_x;
		float height = max_y - min_y;
		float depth = max_z - min_z;

		// Объем как произведение размеров
		result.fApproxVolume = width * height * depth;

		// Сложность геометрии на основе дисперсии расстояний
		float avg_distance = 0.0f;
		for (const Fvector& point : hit_points)
		{
			avg_distance += start_pos.distance_to(point);
		}
		avg_distance /= hit_points.size();

		float variance = 0.0f;
		for (const Fvector& point : hit_points)
		{
			float dist = start_pos.distance_to(point);
			variance += (dist - avg_distance) * (dist - avg_distance);
		}
		variance /= hit_points.size();

		result.fComplexity = _min(1.0f, sqrtf(variance) / (avg_distance + 0.1f));

		// Вертикальность
		float avg_horizontal = 0.0f;
		for (float dist : horizontal_distances)
		{
			avg_horizontal += dist;
		}
		avg_horizontal /= (horizontal_distances.size() + 0.001f);

		float avg_vertical = 0.0f;
		for (float dist : vertical_distances)
		{
			avg_vertical += dist;
		}
		avg_vertical /= (vertical_distances.size() + 0.001f);

		result.fVerticality = avg_vertical / (avg_horizontal + 0.1f);
		clamp(result.fVerticality, 0.1f, 3.0f);
	}
	else
	{
		// Открытое пространство
		result.fApproxVolume = 100000.0f;
		result.fComplexity = 0.1f;
		result.fVerticality = 1.0f;
	}

	// Ограничение объема
	result.fApproxVolume = _min(result.fApproxVolume, 200000.0f);
}
