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
 * @brief Performs detailed raycast with enhanced material detection
 */
float CSoundEnvironment::PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type)
{
	collide::rq_result hit;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, hit, ViewEntity);

	if (bHit && hit.range > 0.1f)
	{
		if (hit.range < 3.0f)
			material_type = 1; // Close surfaces - high reflection
		else if (hit.range < 10.0f)
			material_type = 2; // Medium distance
		else if (hit.range < 25.0f)
			material_type = 3; // Far surfaces
		else
			material_type = 4; // Very far surfaces

		return hit.range;
	}

	material_type = 0; // Air/sky
	return max_dist;
}

/**
 * @brief Analyzes spatial distribution of hit distances to classify environment type
 */
void CSoundEnvironment::AnalyzeSpatialDistribution(Fvector center, std::vector<float>& distances,
												   SEAXEnvironmentData& result)
{
	float min_dist = 10000.0f, max_dist = 0.0f;
	float sum = 0.0f, sum_sq = 0.0f;

	for (u32 i = 0; i < distances.size(); ++i)
	{
		float dist = distances[i];
		min_dist = _min(min_dist, dist);
		max_dist = _max(max_dist, dist);
		sum += dist;
		sum_sq += dist * dist;
	}

	float mean = sum / distances.size();
	float variance = (sum_sq / distances.size()) - (mean * mean);
	float std_dev = sqrtf(variance);

	// Space type classification based on distribution
	float coherence = 1.0f - (std_dev / mean);
	clamp(coherence, 0.0f, 1.0f);

	// Determine geometry type
	if (max_dist - min_dist < 5.0f && coherence > 0.8f)
	{
		result.fRoomSize = mean / 30.0f;
	}
	else if (std_dev > mean * 0.5f)
	{
		result.fRoomSize = (mean + std_dev) / 40.0f;
	}
	else
	{
		result.fRoomSize = mean / 100.0f;
	}

	clamp(result.fRoomSize, 0.05f, 1.0f);
}

/**
 * @brief Main environment analysis method that combines all subsystems
 */
/**
 * @brief Main environment analysis method that combines all subsystems for EAX
 */
void CSoundEnvironment::CalculateEnvironmentData()
{
	SEAXEnvironmentData newData;
	Fvector RayPosition = Device.vCameraPosition;

	// Run all analysis subsystems
	m_PathTracer.PerformPathTracingAnalysis(RayPosition, m_PathTracingResult);
	m_GeometryAnalyzer.AnalyzeEnvironmentGeometry(RayPosition, m_CurrentGeometry);

	// Collect raycast data in all directions
	std::vector<float> distances;
	distances.reserve(DETAILED_DIRECTIONS_COUNT);

	u32 TotalHits = 0;
	u32 CloseHits = 0;
	u32 FarHits = 0;
	float TotalDistance = 0.0f;

	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		Fvector3 RayDirection = DetailedSphereDirections[i];
		u32 material_type = 0;

		float distance = PerformDetailedRaycast(RayPosition, RayDirection, SMART_RAY_DISTANCE, material_type);
		distances.push_back(distance);
		TotalDistance += distance;

		if (material_type > 0)
		{
			TotalHits++;
			if (distance < 3.0f)
				CloseHits++;
			if (distance > 15.0f)
				FarHits++;
		}
	}

	// Calculate basic environment parameters
	newData.fEnvironmentRadius = TotalDistance / DETAILED_DIRECTIONS_COUNT;
	newData.fEnvironmentDensity = float(TotalHits) / DETAILED_DIRECTIONS_COUNT;

	// Calculate EAX openness parameter
	float basic_openness = float(FarHits) / DETAILED_DIRECTIONS_COUNT;
	float path_tracing_openness = 1.0f - (m_PathTracingResult.fTotalEnergy * 5.0f);
	float geometry_openness = 1.0f - (m_CurrentGeometry.fComplexity * 0.2f + newData.fEnvironmentDensity * 0.8f);

	newData.fOpenness = (basic_openness * 0.4f + path_tracing_openness * 0.4f + geometry_openness * 0.2f);
	clamp(newData.fOpenness, 0.0f, 1.0f);

	// Calculate EAX enclosedness parameter
	float basic_enclosedness = float(CloseHits) / DETAILED_DIRECTIONS_COUNT;
	float path_tracing_enclosedness = m_PathTracingResult.fTotalEnergy * 2.0f;
	float geometry_enclosedness = m_CurrentGeometry.fComplexity * 0.3f + newData.fEnvironmentDensity * 0.7f;

	newData.fEnclosedness =
		(basic_enclosedness * 0.4f + path_tracing_enclosedness * 0.3f + geometry_enclosedness * 0.3f);
	clamp(newData.fEnclosedness, 0.0f, 1.0f);

	// Ensure EAX parameters are complementary
	if (newData.fOpenness + newData.fEnclosedness > 1.0f)
	{
		float sum = newData.fOpenness + newData.fEnclosedness;
		newData.fOpenness /= sum;
		newData.fEnclosedness /= sum;
	}

	// EAX environment variance and uniformity
	newData.fFogDensity = g_pGamePersistent->Environment().CurrentEnv->fog_density;

	float Variance = 0.0f;
	float AvgDistance = TotalDistance / DETAILED_DIRECTIONS_COUNT;
	for (u32 i = 0; i < distances.size(); ++i)
	{
		float Diff = distances[i] - AvgDistance;
		Variance += Diff * Diff;
	}

	newData.fEnvironmentVariance = sqrtf(Variance / distances.size()) / newData.fEnvironmentRadius;
	clamp(newData.fEnvironmentVariance, 0.0f, 1.0f);
	newData.fUniformity = 1.0f - newData.fEnvironmentVariance;

	// EAX reflectivity calculation
	newData.fReflectivity = 0.05f + 0.3f * newData.fEnclosedness * m_PathTracingResult.fEnergyDistribution;
	clamp(newData.fReflectivity, 0.02f, 0.4f);

	// EAX PHYSICAL ACOUSTIC PARAMETERS
	newData.sPhysicalData.fRoomVolume = m_CurrentGeometry.fApproxVolume;

	// Adjusted radius for EAX environment size
	newData.sPhysicalData.fAdjustedRadius = newData.fEnvironmentRadius * (1.0f + newData.fOpenness);
	clamp(newData.sPhysicalData.fAdjustedRadius, 1.0f, 100.0f);

	// EAX echo strength calculation
	float volume_cubic_meters = m_CurrentGeometry.fApproxVolume;
	float normalized_volume = volume_cubic_meters / 1000.0f;
	clamp(normalized_volume, 0.1f, 50.0f);

	float volumeFactor = 0.0f;
	if (normalized_volume < 1.0f)
	{
		volumeFactor = normalized_volume * 0.3f;
	}
	else if (normalized_volume < 5.0f)
	{
		volumeFactor = 0.3f + (normalized_volume - 1.0f) / 4.0f * 0.3f;
	}
	else
	{
		volumeFactor = 0.6f - (normalized_volume - 5.0f) / 45.0f * 0.4f;
	}
	clamp(volumeFactor, 0.05f, 0.6f);

	float path_tracing_echo = m_PathTracingResult.fTotalEnergy * (1.0f + m_PathTracingResult.iNumBounces * 0.1f);
	float opennessFactor = 1.0f - newData.fOpenness * 0.9f;

	// Final EAX echo strength
	newData.sPhysicalData.fEchoStrength =
		volumeFactor * newData.fReflectivity * opennessFactor * path_tracing_echo * 0.5f;
	clamp(newData.sPhysicalData.fEchoStrength, 0.005f, 0.3f);

	// EAX reverb time calculation
	float base_reverb = 0.0f;
	if (normalized_volume < 1.0f)
	{
		base_reverb = 0.3f + normalized_volume * 0.4f;
	}
	else if (normalized_volume < 5.0f)
	{
		base_reverb = 0.7f + (normalized_volume - 1.0f) / 4.0f * 0.8f;
	}
	else
	{
		base_reverb = 1.5f - (normalized_volume - 5.0f) / 45.0f * 0.8f;
	}

	float path_tracing_reverb = 1.0f + m_PathTracingResult.iNumBounces * 0.15f;

	// Final EAX reverb time
	newData.sPhysicalData.fReverbTime = base_reverb * path_tracing_reverb * (0.4f + 0.6f * (1.0f - newData.fOpenness));
	clamp(newData.sPhysicalData.fReverbTime, 0.1f, 2.5f);

	// EAX critical distance
	newData.sPhysicalData.fCriticalDistance = 2.0f + m_CurrentGeometry.fAvgWallDistance / 4.0f;
	clamp(newData.sPhysicalData.fCriticalDistance, 2.0f, 12.0f);

	// EAX REFLECTION DATA
	newData.sReflectionData.fReflectionEnergy = m_PathTracingResult.fTotalEnergy * 0.5f;
	newData.sReflectionData.fPrimaryReflections = m_PathTracingResult.fTotalEnergy * 0.3f;
	newData.sReflectionData.fSecondaryReflections = m_PathTracingResult.fTotalEnergy * 0.1f;
	newData.sReflectionData.fReflectionDelay = m_PathTracingResult.fTotalDelay;
	newData.sReflectionData.fSurfaceComplexity = m_PathTracingResult.fEnergyDistribution;

	// Clamp EAX reflection parameters
	clamp(newData.sReflectionData.fReflectionEnergy, 0.005f, 0.25f);
	clamp(newData.sReflectionData.fPrimaryReflections, 0.0f, 0.3f);
	clamp(newData.sReflectionData.fSecondaryReflections, 0.0f, 0.1f);
	clamp(newData.sReflectionData.fReflectionDelay, 0.005f, 0.05f);
	clamp(newData.sReflectionData.fSurfaceComplexity, 0.0f, 0.5f);

	// EAX OPEN SPACE CORRECTION
	if (newData.fOpenness > 0.4f)
	{
		float open_factor = (newData.fOpenness - 0.4f) / 0.6f;

		// Strong reduction of EAX effects in open spaces
		newData.sPhysicalData.fEchoStrength *= (1.0f - open_factor * 0.9f);
		newData.sPhysicalData.fReverbTime *= (1.0f - open_factor * 0.7f);
		newData.sReflectionData.fReflectionEnergy *= (1.0f - open_factor * 0.95f);
		newData.sReflectionData.fPrimaryReflections *= (1.0f - open_factor * 0.95f);
		newData.sReflectionData.fSecondaryReflections *= (1.0f - open_factor * 0.95f);
		newData.fReflectivity *= (1.0f - open_factor * 0.8f);
	}

	// Finalize EAX data
	newData.bDataValid = true;
	newData.dwFrameStamp = Device.dwFrame;

	m_PrevData.CopyFrom(m_CurrentData);
	m_CurrentData.CopyFrom(newData);

	// Debug output for EAX parameters
	Msg("[EAX Analysis] Radius=%.1f, AdjRadius=%.1f, Openness=%.3f, Enclosed=%.3f", newData.fEnvironmentRadius,
		newData.sPhysicalData.fAdjustedRadius, newData.fOpenness, newData.fEnclosedness);
	Msg("[EAX Analysis] Echo=%.4f, Reverb=%.2f, Reflections=%.4f", newData.sPhysicalData.fEchoStrength,
		newData.sPhysicalData.fReverbTime, newData.sReflectionData.fReflectionEnergy);

	// Update sound system with EAX data
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
