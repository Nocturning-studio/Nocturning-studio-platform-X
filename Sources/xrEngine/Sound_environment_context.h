///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_context.h
// Created: 22.10.2025
// Modified: 21.11.2025 (Integrated Physics Update)
// Author: NSDeathman
// Path tracing EAX - Environment Context
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"
#include "Sound_environment_common.h"

struct EnvironmentContext
{
	// Input data
	Fvector CameraPosition;
	SPathTracingResult PathTracingResult;
	SGeometryAnalysis GeometryAnalysis;

	// Raw raycast data
	std::vector<float> RaycastDistances;
	u32 TotalHits;
	float TotalDistance;
	u32 MaterialHits[6];
	float MaterialDistances[6];

	// New Physical acoustic parameters
	float PhysicalVolume;		 // m^3
	float MeanFreePath;			 // m
	float PhysicalReflectivity;	 // 0-1
	float GeometricOpenness;	 // 0-1
	float GeometricEnclosedness; // 0-1
	float ReverbTime;			 // Seconds (RT60)
	float EarlyReflectionLevel;	 // 0-1
	float ReverbLevel;			 // 0-1

	// Timing
	float ReflectionDelay;
	float EchoDelay;

	// Psychoacoustic corrections
	float PerceivedReflectivity;
	float PerceivedReverbTime;
	float PerceivedEarlyReflections;

	// Final EAX parameters
	SEAXEnvironmentData EAXData;

	EnvironmentContext()
	{
		Reset();
	}

	void Reset()
	{
		RaycastDistances.clear();
		RaycastDistances.reserve(DETAILED_DIRECTIONS_COUNT);
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
		EarlyReflectionLevel = 0.1f;
		ReverbLevel = 0.1f;
		ReflectionDelay = 0.01f;
		EchoDelay = 0.02f;
		PerceivedReflectivity = 0.3f;
		PerceivedReverbTime = 1.0f;
		PerceivedEarlyReflections = 0.1f;
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
