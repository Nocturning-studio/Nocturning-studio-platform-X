///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_pathtracing.h
// Author: NSDeathman
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Sound_environment_common.h"

// struct SPathTracingResult удалена, так как она есть в Common.h

class ENGINE_API CPathTracingSystem
{
  private:
	static const u32 MAX_PATH_DEPTH = 6;
	static const u32 PATHS_PER_DIRECTION = 8;
	static const float ENERGY_THRESHOLD;
	static const float RAY_OFFSET;

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
	// SRayHitInfo теперь видна из common.h
	SRayHitInfo PerformRaycastWithNormal(Fvector start, Fvector dir, float max_dist);
	Fvector CalculateDiffuseReflection(Fvector incident, Fvector normal, float roughness = 0.3f);
	Fvector CalculateSpecularReflection(Fvector incident, Fvector normal);
	float CalculateMaterialReflectivity(const SRayHitInfo& hit);
	float CalculateEnergyDecay(float distance, float reflectivity, float angle_factor);
	bool ShouldContinueTracing(float energy, u32 depth);
};
