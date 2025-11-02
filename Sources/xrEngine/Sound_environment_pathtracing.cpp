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
#include "../xrCDB/xrCDB.h"
#endif
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_pathtracing.h"
///////////////////////////////////////////////////////////////////////////////////
const float CPathTracingSystem::ENERGY_THRESHOLD = 0.001f;
const float CPathTracingSystem::RAY_OFFSET = 0.01f;
///////////////////////////////////////////////////////////////////////////////////
SRayHitInfo CPathTracingSystem::PerformRaycastWithNormal(Fvector start, Fvector dir, float max_dist)
{
	SRayHitInfo result;

	collide::rq_result RQ;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, RQ, ViewEntity);

	if (bHit && RQ.range > 0.01f)
	{
		result.bHit = true;
		result.fDistance = RQ.range;
		result.vPosition.mad(start, dir, RQ.range);

		// Simplified normal calculation
		result.vNormal.set(-dir.x, -dir.y, -dir.z);
		result.vNormal.normalize_safe();

		// Material type based on distance
		if (result.fDistance < 2.0f)
			result.material_type = 1;
		else if (result.fDistance < 8.0f)
			result.material_type = 2;
		else if (result.fDistance < 20.0f)
			result.material_type = 3;
		else
			result.material_type = 4;
	}
	else
	{
		result.bHit = false;
		result.fDistance = max_dist;
		result.material_type = 0;
	}

	return result;
}

Fvector CPathTracingSystem::CalculateDiffuseReflection(Fvector incident, Fvector normal, float roughness)
{
	Fvector random_dir;

	// Generate random direction in hemisphere
	float u1 = rand() / (float)RAND_MAX;
	float u2 = rand() / (float)RAND_MAX;

	float theta = 2.0f * PI * u1;
	float phi = acosf(2.0f * u2 - 1.0f);

	random_dir.x = sinf(phi) * cosf(theta);
	random_dir.y = sinf(phi) * sinf(theta);
	random_dir.z = cosf(phi);

	if (random_dir.dotproduct(normal) < 0)
	{
		random_dir.invert();
	}

	Fvector ideal_reflection = CalculateSpecularReflection(incident, normal);
	Fvector result;
	result.lerp(ideal_reflection, random_dir, roughness);
	result.normalize_safe();

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

float CPathTracingSystem::CalculateMaterialReflectivity(const SRayHitInfo& hit)
{
	if (!hit.bHit)
		return 0.0f;

	float distance_factor = 1.0f - (hit.fDistance / 80.0f);
	clamp(distance_factor, 0.05f, 1.0f);

	float base_reflectivity = 0.0f;

	switch (hit.material_type)
	{
	case 1:
		base_reflectivity = 0.6f;
		break;
	case 2:
		base_reflectivity = 0.4f;
		break;
	case 3:
		base_reflectivity = 0.25f;
		break;
	case 4:
		base_reflectivity = 0.1f;
		break;
	default:
		base_reflectivity = 0.0f;
		break;
	}

	return base_reflectivity * distance_factor;
}

float CPathTracingSystem::CalculateEnergyDecay(float distance, float reflectivity, float angle_factor)
{
	float distance_decay = 1.0f / (1.0f + distance * 0.1f);
	float material_decay = reflectivity;
	float incidence_decay = angle_factor;

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

			float cos_angle = fabs(current.vDirection.dotproduct(hit.vNormal));
			float reflectivity = CalculateMaterialReflectivity(hit);

			float distance_decay = 1.0f / (1.0f + hit.fDistance * 0.2f);
			float depth_decay = 1.0f / (1.0f + current.iDepth * 2.0f);
			float material_decay = reflectivity * 0.7f;
			float incidence_decay = 0.3f + 0.7f * cos_angle;

			float energy_decay = distance_decay * depth_decay * material_decay * incidence_decay;
			float segment_energy = current.fEnergy * energy_decay;

			float energy_weight = 1.0f / (1.0f + current.iDepth * current.iDepth);
			path_result.fTotalEnergy += segment_energy * energy_weight;
			path_result.fTotalDelay += total_delay * segment_energy * energy_weight;
			path_result.iNumBounces = _max(path_result.iNumBounces, current.iDepth + 1);
			path_result.fAverageDistance += hit.fDistance * segment_energy * energy_weight;

			if (ShouldContinueTracing(segment_energy, current.iDepth + 1))
			{
				Fvector new_start;
				new_start.mad(hit.vPosition, hit.vNormal, RAY_OFFSET);

				Fvector specular_reflection = CalculateSpecularReflection(current.vDirection, hit.vNormal);
				Fvector diffuse_reflection = CalculateDiffuseReflection(current.vDirection, hit.vNormal, 0.5f);

				active_segments.push_back(SPathSegment(new_start, specular_reflection, segment_energy * 0.4f,
													   current.iDepth + 1, total_delay));

				active_segments.push_back(SPathSegment(new_start, diffuse_reflection, segment_energy * 0.2f,
													   current.iDepth + 1, total_delay));
			}
		}
		else
		{
			path_result.fTotalEnergy += current.fEnergy * 0.02f;
		}
	}

	if (path_result.fTotalEnergy > 0)
	{
		path_result.fAverageDistance /= path_result.fTotalEnergy;
		path_result.fTotalDelay /= path_result.fTotalEnergy;

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
			varied_dir.x += (rand() / (float)RAND_MAX - 0.5f) * 0.2f;
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

	// Aggregate results
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

		// Calculate energy distribution
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
		result.fTotalEnergy = 0.002f;
		result.fTotalDelay = 0.01f;
		result.fAverageDistance = 50.0f;
		result.iNumBounces = 0;
		result.fEnergyDistribution = 0.05f;
	}

	// Global energy reduction
	result.fTotalEnergy *= 0.3f;

	// Clamp values
	clamp(result.fTotalEnergy, 0.0f, 0.15f);
	clamp(result.fTotalDelay, 0.005f, 0.1f);
	clamp(result.fAverageDistance, 0.1f, 100.0f);
	clamp(result.fEnergyDistribution, 0.0f, 1.0f);
}
