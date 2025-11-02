///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_common.h"
#include "Sound_environment_context.h"
#include "Sound_environment_geometry.h"
#include "Sound_environment_pathtracing.h"
#include "Sound_environment_calculations.h"
///////////////////////////////////////////////////////////////////////////////////

/**
 * @class CSoundEnvironment
 * @brief Main sound environment processor that analyzes 3D acoustic properties
 */
class ENGINE_API CSoundEnvironment
{
  private:
	SEAXEnvironmentData m_CurrentData;	   ///< Current frame's environment data
	SEAXEnvironmentData m_PrevData;		   ///< Previous frame's environment data
	u32 m_LastUpdatedFrame;				   ///< Last frame when environment was updated
	static const u32 UPDATE_INTERVAL = 10; ///< Minimum frames between updates

	class CGeometryAnalyzer m_GeometryAnalyzer; ///< Geometry analysis subsystem
	class CPathTracingSystem m_PathTracer;		///< Path tracing subsystem

	// Core analysis methods
	float PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type);
	void GatherRaycastData(EnvironmentContext& context);

	// Update methods
	bool NeedUpdate() const;
	void __stdcall MT_CALC();
	void CalculateEnvironmentData();

  public:
	CSoundEnvironment();
	~CSoundEnvironment();

	void Update(); ///< Main update method called each frame
};
///////////////////////////////////////////////////////////////////////////////////
