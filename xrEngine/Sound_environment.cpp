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
#include "Sound_environment.h"
///////////////////////////////////////////////////////////////////////////////////

CSoundEnvironment::CSoundEnvironment() : m_LastUpdatedFrame(0)
{
	ZeroMemory(&m_CurrentData, sizeof(m_CurrentData));
	ZeroMemory(&m_PrevData, sizeof(m_PrevData));
	m_CurrentData.Reset();
}

CSoundEnvironment::~CSoundEnvironment()
{
}

/**
 * @brief Enhanced raycast with improved material classification
 */
float CSoundEnvironment::PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type)
{
	collide::rq_result hit;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, hit, ViewEntity);

	if (bHit && hit.range > 0.1f)
	{
		if (hit.range < 2.0f)
			material_type = 1; // Very close - high reflectivity (concrete, metal)
		else if (hit.range < 6.0f)
			material_type = 2; // Close - medium reflectivity (wood, plaster)
		else if (hit.range < 15.0f)
			material_type = 3; // Medium - low reflectivity (fabric, vegetation)
		else if (hit.range < 30.0f)
			material_type = 4; // Far - very low reflectivity
		else
			material_type = 5; // Very far - minimal reflectivity

		return hit.range;
	}

	material_type = 0; // Air/sky - no reflection
	return max_dist;
}

/**
 * @brief Helper method to gather raycast data for EnvironmentContext
 */
void CSoundEnvironment::GatherRaycastData(EnvironmentContext& context)
{
	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		Fvector3 RayDirection = DetailedSphereDirections[i];
		u32 material_type = 0;

		float distance =
			PerformDetailedRaycast(context.CameraPosition, RayDirection, SMART_RAY_DISTANCE, material_type);
		context.RaycastDistances.push_back(distance);
		context.TotalDistance += distance;

		if (material_type > 0)
		{
			context.TotalHits++;
			context.MaterialHits[material_type]++;
			context.MaterialDistances[material_type] += distance;

			if (distance < 3.0f)
				context.CloseHits++;
			else if (distance < 10.0f)
				context.MediumHits++;
			else if (distance > 20.0f)
				context.FarHits++;
		}
	}
}

/**
 * @brief Clean main environment analysis using component-based architecture
 */
void CSoundEnvironment::CalculateEnvironmentData()
{
	EnvironmentContext context;
	context.CameraPosition = Device.vCameraPosition;

	// Run analysis subsystems
	m_PathTracer.PerformPathTracingAnalysis(context.CameraPosition, context.PathTracingResult);
	m_GeometryAnalyzer.AnalyzeEnvironmentGeometry(context.CameraPosition, context.GeometryAnalysis);

	// Gather raycast data
	GatherRaycastData(context);

	// Execute calculation pipeline - each component focuses on one specific aspect
	EnvironmentCalculations::CalculateGeometricProperties(context);
	EnvironmentCalculations::CalculateReflectivity(context);
	EnvironmentCalculations::CalculateContinuousMultipliers(context);
	EnvironmentCalculations::CalculateEchoParameters(context);
	EnvironmentCalculations::CalculateReverbParameters(context);
	EnvironmentCalculations::CalculateReflectionParameters(context);
	EnvironmentCalculations::MapToEAXParameters(context);

	// Finalize data
	context.EAXData.dwFrameStamp = Device.dwFrame;
	context.EAXData.bDataValid = true;

	m_PrevData.CopyFrom(m_CurrentData);
	m_CurrentData.CopyFrom(context.EAXData);

	// Debug output
	Msg("[EAX Architecture] Room=%d, Reflections=%d, Reverb=%d", context.EAXData.lRoom, context.EAXData.lReflections,
		context.EAXData.lReverb);
	Msg("[EAX Architecture] DecayTime=%.2fs, ReflectionsDelay=%.3fs", context.EAXData.flDecayTime,
		context.EAXData.flReflectionsDelay);

	// Update sound system with pure EAX data
	if (::Sound)
		::Sound->commit_eax(&m_CurrentData);
}

/**
 * @brief Thread-safe environment calculation method
 */
void __stdcall CSoundEnvironment::MT_CALC()
{
	CalculateEnvironmentData();
}

/**
 * @brief Determines if environment needs updating this frame
 */
bool CSoundEnvironment::NeedUpdate() const
{
	if (!psSoundFlags.test(ss_EAX))
		return false;

	if (Device.dwFrame < (m_LastUpdatedFrame + UPDATE_INTERVAL))
		return false;

	return true;
}

/**
 * @brief Main update method called each frame
 */
void CSoundEnvironment::Update()
{
	if (NeedUpdate())
	{
		Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CSoundEnvironment::MT_CALC));
		m_LastUpdatedFrame = Device.dwFrame;
	}
}
