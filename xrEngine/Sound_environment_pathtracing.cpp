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
#include "../xrCDB/xrCDB.h"
#endif

#include "Sound_environment_pathtracing.h"

const float CPathTracingSystem::ENERGY_THRESHOLD = 0.001f;
const float CPathTracingSystem::RAY_OFFSET = 0.01f;

SRayHitInfo CPathTracingSystem::PerformRaycastWithNormal(Fvector start, Fvector dir, float max_dist)
{
	SRayHitInfo result;

	collide::rq_result RQ;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	// Используем RayPick для получения быстрого попадания
	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, RQ, ViewEntity);

	if (bHit && RQ.range > 0.01f)
	{
		result.bHit = true;
		result.fDistance = RQ.range;
		result.vPosition.mad(start, dir, RQ.range);

		// ПОЛУЧАЕМ РЕАЛЬНУЮ НОРМАЛЬ через дополнительный запрос
		collide::rq_result detailed_rq;
		Fvector detailed_start;
		detailed_start.mad(result.vPosition, dir, -0.1f); // Немного отступаем назад

		// Делаем очень короткий луч для получения нормали
		if (g_pGameLevel->ObjectSpace.RayPick(detailed_start, dir, 0.2f, collide::rqtStatic, detailed_rq, ViewEntity))
		{
			// Вычисляем нормаль на основе разности положений
			// В реальном движке нужно получить нормаль из CDB
			Fvector hit_pos;
			hit_pos.mad(detailed_start, dir, detailed_rq.range);

			// Упрощенное вычисление нормали (в реальном проекте используйте нормали из COLLIDE)
			// Создаем псевдо-нормаль на основе направления
			result.vNormal.set(-dir.x, -dir.y, -dir.z);
			result.vNormal.normalize_safe();

			// Альтернативный метод: делаем несколько лучей вокруг для определения нормали
			Fvector pseudo_normal;
			pseudo_normal.set(0, 0, 0);
			int valid_normals = 0;

			// Небольшие смещения для определения плоскости
			float offsets[][2] = {{0.01f, 0.0f}, {-0.01f, 0.0f}, {0.0f, 0.01f}, {0.0f, -0.01f}};

			for (int i = 0; i < 4; i++)
			{
				Fvector offset_dir = dir;
				offset_dir.x += offsets[i][0];
				offset_dir.y += offsets[i][1];
				offset_dir.normalize_safe();

				collide::rq_result offset_rq;
				if (g_pGameLevel->ObjectSpace.RayPick(detailed_start, offset_dir, 0.2f, collide::rqtStatic, offset_rq,
													  ViewEntity))
				{
					Fvector offset_pos;
					offset_pos.mad(detailed_start, offset_dir, offset_rq.range);
					Fvector diff;
					diff.sub(offset_pos, hit_pos);
					pseudo_normal.add(diff);
					valid_normals++;
				}
			}

			if (valid_normals > 0)
			{
				pseudo_normal.mul(1.0f / valid_normals);
				result.vNormal = pseudo_normal;
				result.vNormal.normalize_safe();
			}
		}
		else
		{
			// Резервный метод: нормаль противоположна направлению луча
			result.vNormal.set(-dir.x, -dir.y, -dir.z);
			result.vNormal.normalize_safe();
		}

		// Определяем тип материала на основе расстояния
		if (result.fDistance < 2.0f)
			result.material_type = 1; // Близкие поверхности - высокое отражение
		else if (result.fDistance < 8.0f)
			result.material_type = 2; // Средние расстояния
		else if (result.fDistance < 20.0f)
			result.material_type = 3; // Далёкие поверхности
		else
			result.material_type = 4; // Очень далекие поверхности
	}
	else
	{
		result.bHit = false;
		result.fDistance = max_dist;
		result.material_type = 0; // Воздух
	}

	return result;
}

Fvector CPathTracingSystem::CalculateDiffuseReflection(Fvector incident, Fvector normal, float roughness)
{
	// Реализация диффузного отражения с некоторой случайностью
	Fvector random_dir;

	// Генерируем случайное направление в полусфере
	float u1 = rand() / (float)RAND_MAX;
	float u2 = rand() / (float)RAND_MAX;

	float theta = 2.0f * PI * u1;
	float phi = acosf(2.0f * u2 - 1.0f);

	random_dir.x = sinf(phi) * cosf(theta);
	random_dir.y = sinf(phi) * sinf(theta);
	random_dir.z = cosf(phi);

	// Убеждаемся, что направление в той же полусфере, что и нормаль
	if (random_dir.dotproduct(normal) < 0)
	{
		random_dir.invert();
	}

	// Смешиваем с идеальным отражением на основе шероховатости
	Fvector ideal_reflection = CalculateSpecularReflection(incident, normal);

	Fvector result;
	result.lerp(ideal_reflection, random_dir, roughness);
	result.normalize_safe();

	return result;
}

Fvector CPathTracingSystem::CalculateSpecularReflection(Fvector incident, Fvector normal)
{
	// Идеальное зеркальное отражение
	Fvector reflection;
	float dot = incident.dotproduct(normal);
	reflection.mad(incident, normal, -2.0f * dot);
	reflection.normalize_safe();
	return reflection;
}

float CPathTracingSystem::CalculateMaterialReflectivity(const SRayHitInfo& hit)
{
	if (!hit.bHit)
		return 0.0f;

	// УМЕНЬШАЕМ базовую отражательную способность
	float distance_factor = 1.0f - (hit.fDistance / 80.0f); // Увеличили дистанцию затухания
	clamp(distance_factor, 0.05f, 1.0f);

	float base_reflectivity = 0.0f;

	switch (hit.material_type)
	{
	case 1:						  // Близкие поверхности
		base_reflectivity = 0.6f; // Уменьшили с 0.8f
		break;
	case 2:						  // Средние расстояния
		base_reflectivity = 0.4f; // Уменьшили с 0.6f
		break;
	case 3:						   // Далёкие поверхности
		base_reflectivity = 0.25f; // Уменьшили с 0.4f
		break;
	case 4:						  // Очень далекие поверхности
		base_reflectivity = 0.1f; // Уменьшили с 0.2f
		break;
	default:
		base_reflectivity = 0.0f;
		break;
	}

	return base_reflectivity * distance_factor;
}

float CPathTracingSystem::CalculateEnergyDecay(float distance, float reflectivity, float angle_factor)
{
	// Физически корректное затухание энергии
	float distance_decay = 1.0f / (1.0f + distance * 0.1f); // Обратно пропорционально расстоянию
	float material_decay = reflectivity;
	float incidence_decay = angle_factor; // Учет угла падения

	return distance_decay * material_decay * incidence_decay;
}

bool CPathTracingSystem::ShouldContinueTracing(float energy, u32 depth)
{
	return (energy > ENERGY_THRESHOLD) && (depth < MAX_PATH_DEPTH);
}

void CPathTracingSystem::TraceSinglePath(Fvector start, Fvector initial_dir, SPathTracingResult& path_result)
{
	std::vector<SPathSegment> active_segments;
	active_segments.reserve(MAX_PATH_DEPTH);

	// УМЕНЬШАЕМ начальную энергию и добавляем случайность
	float initial_energy = 0.5f + (rand() / (float)RAND_MAX) * 0.5f;
	active_segments.push_back(SPathSegment(start, initial_dir, initial_energy, 0, 0.0f));

	while (!active_segments.empty())
	{
		SPathSegment current = active_segments.back();
		active_segments.pop_back();

		if (!ShouldContinueTracing(current.fEnergy, current.iDepth))
			continue;

		SRayHitInfo hit = PerformRaycastWithNormal(current.vStart, current.vDirection, 100.0f);

		if (hit.bHit)
		{
			float segment_delay = hit.fDistance / 340.0f;
			float total_delay = current.fAccumulatedDelay + segment_delay;

			// СИЛЬНОЕ ЗАТУХАНИЕ ЭНЕРГИИ С КАЖДЫМ ОТРАЖЕНИЕМ
			float cos_angle = fabs(current.vDirection.dotproduct(hit.vNormal));
			float reflectivity = CalculateMaterialReflectivity(hit);

			// ОСНОВНОЕ ИСПРАВЛЕНИЕ: сильное затухание с расстоянием и глубиной
			float distance_decay = 1.0f / (1.0f + hit.fDistance * 0.2f); // Увеличили затухание с расстоянием
			float depth_decay = 1.0f / (1.0f + current.iDepth * 2.0f); // Сильное затухание с глубиной
			float material_decay = reflectivity * 0.7f;		 // Уменьшили влияние материала
			float incidence_decay = 0.3f + 0.7f * cos_angle; // Ослабили влияние угла

			float energy_decay = distance_decay * depth_decay * material_decay * incidence_decay;
			float segment_energy = current.fEnergy * energy_decay;

			// УМЕНЬШАЕМ вес энергии для каждого последующего отражения
			float energy_weight = 1.0f / (1.0f + current.iDepth * current.iDepth); // Квадратичное затухание
			path_result.fTotalEnergy += segment_energy * energy_weight;
			path_result.fTotalDelay += total_delay * segment_energy * energy_weight;
			path_result.iNumBounces = _max(path_result.iNumBounces, current.iDepth + 1);
			path_result.fAverageDistance += hit.fDistance * segment_energy * energy_weight;

			if (ShouldContinueTracing(segment_energy, current.iDepth + 1))
			{
				Fvector new_start;
				new_start.mad(hit.vPosition, hit.vNormal, RAY_OFFSET);

				Fvector specular_reflection = CalculateSpecularReflection(current.vDirection, hit.vNormal);
				Fvector diffuse_reflection =
					CalculateDiffuseReflection(current.vDirection, hit.vNormal, 0.5f); // Увеличили шероховатость

				// СИЛЬНО УМЕНЬШАЕМ энергию для следующих отражений
				active_segments.push_back(SPathSegment(new_start, specular_reflection, segment_energy * 0.4f,
													   current.iDepth + 1, total_delay)); // Уменьшили с 0.7f до 0.4f

				active_segments.push_back(SPathSegment(new_start, diffuse_reflection, segment_energy * 0.2f,
													   current.iDepth + 1, total_delay)); // Уменьшили с 0.3f до 0.2f
			}
		}
		else
		{
			// СИЛЬНО УМЕНЬШАЕМ энергию для лучей, ушедших в открытое пространство
			path_result.fTotalEnergy += current.fEnergy * 0.02f; // Уменьшили с 0.1f до 0.02f
		}
	}

	if (path_result.fTotalEnergy > 0)
	{
		path_result.fAverageDistance /= path_result.fTotalEnergy;
		path_result.fTotalDelay /= path_result.fTotalEnergy;

		// ДОПОЛНИТЕЛЬНОЕ ЗАТУХАНИЕ ДЛЯ ОТКРЫТЫХ ПРОСТРАНСТВ
		if (path_result.fAverageDistance > 15.0f)
		{
			float open_space_reduction = 1.0f - ((path_result.fAverageDistance - 15.0f) / 85.0f);
			clamp(open_space_reduction, 0.05f, 1.0f);
			path_result.fTotalEnergy *= open_space_reduction;
		}
	}
}

void CPathTracingSystem::PerformPathTracingAnalysis(Fvector start_pos, SPathTracingResult& result)
{
	ZeroMemory(&result, sizeof(result));

	std::vector<SPathTracingResult> path_results;
	path_results.reserve(PATHS_PER_DIRECTION * 32);

	u32 total_paths_traced = 0;
	u32 paths_with_energy = 0;

	for (u32 dir_idx = 0; dir_idx < 32; ++dir_idx)
	{
		Fvector initial_dir;

		if (dir_idx < 24)
		{
			initial_dir = BalancedHemisphereDirections[dir_idx];
		}
		else
		{
			initial_dir.random_dir();
		}

		for (u32 path_idx = 0; path_idx < PATHS_PER_DIRECTION; ++path_idx)
		{
			SPathTracingResult path_result;

			Fvector varied_dir = initial_dir;
			varied_dir.x += (rand() / (float)RAND_MAX - 0.5f) * 0.2f; // Увеличили вариативность
			varied_dir.y += (rand() / (float)RAND_MAX - 0.5f) * 0.2f;
			varied_dir.z += (rand() / (float)RAND_MAX - 0.5f) * 0.2f;
			varied_dir.normalize_safe();

			TraceSinglePath(start_pos, varied_dir, path_result);
			path_results.push_back(path_result);
			total_paths_traced++;

			if (path_result.fTotalEnergy > 0.001f)
				paths_with_energy++;
		}
	}

	// Агрегируем результаты
	float total_energy = 0.0f;
	float total_delay = 0.0f;
	float total_distance = 0.0f;
	u32 total_bounces = 0;
	u32 valid_paths = 0;

	for (u32 i = 0; i < path_results.size(); ++i)
	{
		if (path_results[i].fTotalEnergy > 0.001f)
		{
			total_energy += path_results[i].fTotalEnergy;
			total_delay += path_results[i].fTotalDelay;
			total_distance += path_results[i].fAverageDistance;
			total_bounces += path_results[i].iNumBounces;
			valid_paths++;
		}
	}

	if (valid_paths > 0)
	{
		result.fTotalEnergy = total_energy / valid_paths;
		result.fTotalDelay = total_delay / valid_paths;
		result.fAverageDistance = total_distance / valid_paths;
		result.iNumBounces = total_bounces / valid_paths;

		// Расчет распределения энергии
		float energy_variance = 0.0f;
		for (u32 i = 0; i < path_results.size(); ++i)
		{
			if (path_results[i].fTotalEnergy > 0.001f)
			{
				float diff = path_results[i].fTotalEnergy - result.fTotalEnergy;
				energy_variance += diff * diff;
			}
		}
		energy_variance /= valid_paths;

		result.fEnergyDistribution = 1.0f - (_min(energy_variance, 0.5f));
	}
	else
	{
		// СИЛЬНО УМЕНЬШАЕМ значения по умолчанию для открытых пространств
		result.fTotalEnergy = 0.002f; // Уменьшили с 0.01f
		result.fTotalDelay = 0.01f;	  // Уменьшили с 0.02f
		result.fAverageDistance = 50.0f;
		result.iNumBounces = 0;
		result.fEnergyDistribution = 0.05f; // Уменьшили с 0.1f
	}

	// ДОПОЛНИТЕЛЬНОЕ ЗАТУХАНИЕ: уменьшаем общую энергию
	result.fTotalEnergy *= 0.3f; // Глобальное уменьшение энергии

	// Ограничиваем значения - СИЛЬНО УМЕНЬШАЕМ МАКСИМУМЫ
	clamp(result.fTotalEnergy, 0.0f, 0.15f); // Уменьшили максимум с 1.0f до 0.15f
	clamp(result.fTotalDelay, 0.005f, 0.1f); // Уменьшили максимум с 0.2f до 0.1f
	clamp(result.fAverageDistance, 0.1f, 100.0f);
	clamp(result.fEnergyDistribution, 0.0f, 1.0f);

	// Логируем статистику
	Msg("[PathTracing] Traced %d paths, %d had energy, AvgEnergy=%.4f", total_paths_traced, paths_with_energy,
		result.fTotalEnergy);
}
