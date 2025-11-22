///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_context.h
// Path tracing EAX - Environment Context
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"
#include "Sound_environment_common.h"

struct EnvironmentContext
{
	// === Input Data ===
	Fvector CameraPosition;
	SPathTracingResult PathTracingResult;
	SGeometryAnalysis GeometryAnalysis;

	// === Real-time Data (Snapshot) ===
	std::vector<float> RaycastDistances;
	u32 TotalHits;
	float TotalDistance;
	u32 MaterialHits[6];
	float MaterialDistances[6];

	// === Accumulation Data (Temporal Integration) ===
	u32 Accum_FrameCount;
	float Accum_TotalDistance;
	u32 Accum_TotalHits;
	u32 Accum_MaterialHits[6];

	// === Acoustic Parameters ===
	float PhysicalVolume;		 // m^3
	float MeanFreePath;			 // m
	float PhysicalReflectivity;	 // 0-1
	float GeometricOpenness;	 // 0-1 (1=Outdoor)
	float GeometricEnclosedness; // 0-1 (1=Bunker)
	float ReverbTime;			 // RT60 (Seconds)
	float EarlyReflectionLevel;
	float ReverbLevel;

	// === Psychoacoustics ===
	float PerceivedReflectivity;

	// === Final Output ===
	SEAXEnvironmentData EAXData;

	EnvironmentContext()
	{
		Reset();
		ResetAccumulation();
	}

	void Reset()
	{
		RaycastDistances.clear();
		TotalHits = 0;
		TotalDistance = 0.0f;
		ZeroMemory(MaterialHits, sizeof(MaterialHits));
		ZeroMemory(MaterialDistances, sizeof(MaterialDistances));

		PhysicalVolume = 100.0f;
		MeanFreePath = 5.0f;
		PhysicalReflectivity = 0.3f;
		GeometricOpenness = 0.5f;
		GeometricEnclosedness = 0.5f;
		ReverbTime = 1.0f;
	}

	void ResetAccumulation()
	{
		Accum_FrameCount = 0;
		Accum_TotalDistance = 0.0f;
		Accum_TotalHits = 0;
		ZeroMemory(Accum_MaterialHits, sizeof(Accum_MaterialHits));
	}

	// Math Helpers
	static float Lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}
	static float Clamp(float v, float min, float max)
	{
		return (v < min) ? min : (v > max) ? max : v;
	}
};
