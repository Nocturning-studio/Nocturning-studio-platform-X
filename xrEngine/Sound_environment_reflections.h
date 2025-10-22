#pragma once
#include "Sound_environment_common.h"
#include "xr_collide_defs.h"
///////////////////////////////////////////////////////////////////////////////////
// Advanced Reflection Analysis System
///////////////////////////////////////////////////////////////////////////////////

class ENGINE_API CAdvancedReflectionAnalyzer
{
  private:
	static const u32 PRIMARY_RAYS = 32;
	static const u32 SECONDARY_RAYS_PER_BOUNCE = 3;
	static const u32 MAX_REFLECTION_BOUNCES = 3;
	static const float MAX_REFLECTION_DISTANCE;

  public:
	void AnalyzeReflections(Fvector start_pos, SReflectionAnalysis& result);
	float TraceReflectionPath(Fvector start, Fvector dir, u32 max_bounces, float energy_decay, float& total_delay);
	Fvector CalculateReflectedDirection(Fvector incident, Fvector normal);
	float EstimateSurfaceReflectivity(collide::rq_result& hit);
};

///////////////////////////////////////////////////////////////////////////////////