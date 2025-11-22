///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_pathtracing.h
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Sound_environment_common.h"
#include "Sound_environment_context.h"

class ENGINE_API CPathTracingSystem
{
  private:
	static const int MAX_BOUNCES = 3;	 // Глубина рекурсии
	static const float MAX_RAY_DIST;	 // Макс дистанция луча
	static const float ENERGY_THRESHOLD; // Порог отсечения

  public:
	void PerformPathTracingAnalysis(EnvironmentContext& ctx);

  private:
	void TraceRecursive(Fvector start, Fvector dir, int depth, float current_energy, EnvironmentContext& ctx);
	Fvector RandomHemisphereDir(Fvector normal);
};
