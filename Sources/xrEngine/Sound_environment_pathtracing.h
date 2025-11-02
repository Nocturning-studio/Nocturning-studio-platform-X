///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_common.h"
#include "xr_collide_defs.h"
///////////////////////////////////////////////////////////////////////////////////
struct SPathTracingResult
{
	float fTotalEnergy;
	float fTotalDelay;
	u32 iNumBounces;
	float fAverageDistance;
	float fEnergyDistribution;

	SPathTracingResult() : fTotalEnergy(0), fTotalDelay(0), iNumBounces(0), fAverageDistance(0), fEnergyDistribution(0)
	{
	}
};

class ENGINE_API CPathTracingSystem
{
  private:
	static const u32 MAX_PATH_DEPTH = 6;	  ///< Maximum number of reflections
	static const u32 PATHS_PER_DIRECTION = 8; ///< Paths per initial direction
	static const float ENERGY_THRESHOLD;	  ///< Energy threshold for stopping tracing
	static const float RAY_OFFSET;			  ///< Surface offset for new rays

	struct SPathSegment
	{
		Fvector vStart;
		Fvector vDirection;
		float fEnergy;
		u32 iDepth;
		float fAccumulatedDelay;

		SPathSegment(Fvector start, Fvector dir, float energy, u32 depth, float delay)
			: vStart(start), vDirection(dir), fEnergy(energy), iDepth(depth), fAccumulatedDelay(delay)
		{
		}
	};

  public:
	void PerformPathTracingAnalysis(Fvector start_pos, SPathTracingResult& result);

  private:
	void TraceSinglePath(Fvector start, Fvector initial_dir, SPathTracingResult& path_result);
	SRayHitInfo PerformRaycastWithNormal(Fvector start, Fvector dir, float max_dist);
	Fvector CalculateDiffuseReflection(Fvector incident, Fvector normal, float roughness = 0.3f);
	Fvector CalculateSpecularReflection(Fvector incident, Fvector normal);
	float CalculateMaterialReflectivity(const SRayHitInfo& hit);
	float CalculateEnergyDecay(float distance, float reflectivity, float angle_factor);
	bool ShouldContinueTracing(float energy, u32 depth);
};
