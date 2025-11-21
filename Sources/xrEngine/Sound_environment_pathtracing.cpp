///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_pathtracing.cpp
// Author: NSDeathman
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "igame_level.h"
#include "xr_area.h"
#include "xr_object.h"
#include "xr_collide_defs.h"
#endif
#include "Sound_environment_pathtracing.h"

const float CPathTracingSystem::ENERGY_THRESHOLD = 0.001f;
const float CPathTracingSystem::RAY_OFFSET = 0.02f;

// Теперь SRayHitInfo известен компилятору, ошибки C3646 не будет
SRayHitInfo CPathTracingSystem::PerformRaycastWithNormal(Fvector start, Fvector dir, float max_dist)
{
	SRayHitInfo result;
	collide::rq_result RQ;
	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, RQ, NULL);

	if (bHit && RQ.range > 0.01f)
	{
		result.bHit = true;
		result.fDistance = RQ.range;
		result.vPosition.mad(start, dir, RQ.range);
		result.vNormal.set(-dir.x, -dir.y, -dir.z);
		result.vNormal.normalize_safe();

		if (result.fDistance < 2.0f)
			result.material_type = MATERIAL_STONE;
		else if (result.fDistance < 8.0f)
			result.material_type = MATERIAL_METAL;
		else if (result.fDistance < 20.0f)
			result.material_type = MATERIAL_WOOD;
		else
			result.material_type = MATERIAL_SOFT;
	}
	else
	{
		result.bHit = false;
		result.fDistance = max_dist;
		result.material_type = MATERIAL_AIR;
	}
	return result;
}

Fvector CPathTracingSystem::CalculateSpecularReflection(Fvector incident, Fvector normal)
{
	Fvector reflection;
	float dot = incident.dotproduct(normal);
	reflection.mad(incident, normal, -2.0f * dot);
	reflection.normalize_safe();
	return reflection;
}

// Заглушки для методов, которые были объявлены в хедере, но могли потеряться
Fvector CPathTracingSystem::CalculateDiffuseReflection(Fvector incident, Fvector normal, float roughness)
{
	// Простая заглушка для примера
	Fvector result;
	result.random_dir(normal, PI_DIV_2);
	return result;
}
float CPathTracingSystem::CalculateMaterialReflectivity(const SRayHitInfo& hit)
{
	return 0.5f; // Заглушка
}
float CPathTracingSystem::CalculateEnergyDecay(float distance, float reflectivity, float angle_factor)
{
	return 0.5f; // Заглушка
}
bool CPathTracingSystem::ShouldContinueTracing(float energy, u32 depth)
{
	return energy > ENERGY_THRESHOLD && depth < MAX_PATH_DEPTH;
}

void CPathTracingSystem::TraceSinglePath(Fvector start, Fvector initial_dir, SPathTracingResult& path_result)
{
	Fvector curr_pos = start;
	Fvector curr_dir = initial_dir;
	float energy = 1.0f;
	float accumulated_dist = 0.0f;

	for (u32 d = 0; d < 2; ++d)
	{
		SRayHitInfo hit = PerformRaycastWithNormal(curr_pos, curr_dir, 50.0f);

		if (hit.bHit)
		{
			accumulated_dist += hit.fDistance;

			float absorption = 0.3f;
			if (hit.material_type == MATERIAL_SOFT)
				absorption = 0.7f;
			energy *= (1.0f - absorption);

			if (d == 0)
				path_result.fEarlyReflections += energy * 0.5f;
			else
				path_result.fLateReverberation += energy * 0.3f;

			path_result.fTotalEnergy += energy;
			path_result.iNumBounces++;

			curr_pos = hit.vPosition;
			curr_dir = CalculateSpecularReflection(curr_dir, hit.vNormal);
		}
		else
		{
			break;
		}
	}
}

void CPathTracingSystem::PerformPathTracingAnalysis(Fvector start_pos, SPathTracingResult& result)
{
	ZeroMemory(&result, sizeof(result));

	const u32 SAMPLES = 16;
	// Используем extern, чтобы сослаться на массив из common.h или другого cpp
	// В X-Ray обычно это делается через #include "Sound_environment_common.h", который уже есть

	for (u32 i = 0; i < SAMPLES; ++i)
	{
		Fvector dir = BalancedHemisphereDirections[i % BALANCED_DIRECTIONS_COUNT];
		TraceSinglePath(start_pos, dir, result);
	}

	if (SAMPLES > 0)
	{
		result.fTotalEnergy /= SAMPLES;
		result.fEarlyReflections /= SAMPLES;
		result.fLateReverberation /= SAMPLES;
	}

	clamp(result.fEarlyReflections, 0.0f, 1.0f);
	clamp(result.fLateReverberation, 0.0f, 1.0f);
}
