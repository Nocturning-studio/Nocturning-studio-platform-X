///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_geometry.cpp
// Author: NSDeathman
///////////////////////////////////////////////////////////////////////////////////
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

Fvector CGeometryAnalyzer::EstimateMainAxis(const std::vector<Fvector>& hit_points)
{
	Fvector avg_direction = {0, 0, 0};
	for (u32 i = 0; i < hit_points.size(); ++i)
	{
		Fvector dir;
		dir.sub(hit_points[i], Device.vCameraPosition);
		dir.normalize_safe();
		avg_direction.add(dir);
	}
	if (!hit_points.empty())
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

	// Using standard Detailed Directions (ensure extern is visible)
	extern const Fvector DetailedSphereDirections[];

	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		Fvector ray_dir = DetailedSphereDirections[i];
		collide::rq_result hit;
		BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start_pos, ray_dir, 200.0f, collide::rqtStatic, hit, NULL);

		if (bHit && hit.range > 0.1f && hit.range < 195.0f)
		{
			Fvector hit_point;
			hit_point.mad(start_pos, ray_dir, hit.range);
			hit_points.push_back(hit_point);

			if (_abs(ray_dir.y) < 0.3f)
				horizontal_distances.push_back(hit.range);
			else if (_abs(ray_dir.y) > 0.7f)
				vertical_distances.push_back(hit.range);
		}
	}

	if (hit_points.size() > 10)
	{
		// Simplified AABB volume approximation for stability
		Fvector min_b, max_b;
		min_b.set(1e10f, 1e10f, 1e10f);
		max_b.set(-1e10f, -1e10f, -1e10f);

		float avg_dist = 0;
		for (const Fvector& p : hit_points)
		{
			min_b.min(p);
			max_b.max(p);
			avg_dist += start_pos.distance_to(p);
		}
		avg_dist /= hit_points.size();

		float width = max_b.x - min_b.x;
		float height = max_b.y - min_b.y;
		float depth = max_b.z - min_b.z;

		result.fApproxVolume = width * height * depth;

		// Variance/Complexity
		float variance = 0.0f;
		for (const Fvector& p : hit_points)
		{
			float d = start_pos.distance_to(p);
			variance += (d - avg_dist) * (d - avg_dist);
		}
		variance /= hit_points.size();
		result.fComplexity = _min(1.0f, _sqrt(variance) / (avg_dist + 0.1f));

		// Verticality
		float avg_h = 0, avg_v = 0;
		for (float d : horizontal_distances)
			avg_h += d;
		for (float d : vertical_distances)
			avg_v += d;
		if (!horizontal_distances.empty())
			avg_h /= horizontal_distances.size();
		if (!vertical_distances.empty())
			avg_v /= vertical_distances.size();

		result.fVerticality = avg_v / (avg_h + 0.1f);
		clamp(result.fVerticality, 0.1f, 3.0f);
	}
	else
	{
		result.fApproxVolume = 100000.0f;
		result.fComplexity = 0.1f;
		result.fVerticality = 1.0f;
	}

	// Clamp volume to prevent validation errors
	result.fApproxVolume = _min(result.fApproxVolume, 200000.0f);
}
