///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX - Environment Context
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Sound_environment_common.h"
#include "Sound_environment_geometry.h"
#include "Sound_environment_pathtracing.h"
///////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Environment analysis context - stores all intermediate calculations
 * Similar to LightContext in Unreal Engine 4
 */
struct EnvironmentContext
{
	// Input data
	Fvector CameraPosition;
	SPathTracingResult PathTracingResult;
	SGeometryAnalysis GeometryAnalysis;

	// Raw raycast data
	std::vector<float> RaycastDistances;
	u32 TotalHits;
	u32 CloseHits;
	u32 MediumHits;
	u32 FarHits;
	float TotalDistance;
	u32 MaterialHits[6];
	float MaterialDistances[6];

	// Intermediate calculations
	float Openness;
	float Enclosedness;
	float Reflectivity;
	float EnvironmentRadius;
	float EnvironmentDensity;
	float EnvironmentVariance;
	float Uniformity;

	// Continuous multipliers
	float OpenSpaceMultiplier;
	float EnclosedMultiplier;
	float ComplexityMultiplier;
	float OpenReductionFactor;
	float EnclosedBoostFactor;
	float ComplexityBoostFactor;

	// Physical acoustic parameters
	float RoomVolume;
	float NormalizedVolume;
	float VolumeFactor;
	float BaseReverb;
	float EchoDelay;

	float ReflectionEnergy;
	float PrimaryReflections;
	float SecondaryReflections;
	float ReflectionDelay;
	float SurfaceComplexity;

	// Final EAX parameters
	SEAXEnvironmentData EAXData;

	// Constructor
	EnvironmentContext()
		: TotalHits(0), CloseHits(0), MediumHits(0), FarHits(0), TotalDistance(0), Openness(0.5f), Enclosedness(0.5f),
		  Reflectivity(0.5f), EnvironmentRadius(10.0f), EnvironmentDensity(0.5f), EnvironmentVariance(0.5f),
		  Uniformity(0.5f), OpenSpaceMultiplier(0.0f), EnclosedMultiplier(0.0f), ComplexityMultiplier(0.0f),
		  OpenReductionFactor(1.0f), EnclosedBoostFactor(1.0f), ComplexityBoostFactor(1.0f), RoomVolume(100.0f),
		  NormalizedVolume(0.1f), VolumeFactor(0.3f), BaseReverb(1.0f), EchoDelay(0.02f), ReflectionEnergy(0.1f),
		  PrimaryReflections(0.05f), SecondaryReflections(0.02f), ReflectionDelay(0.01f), SurfaceComplexity(0.3f)
	{
		ZeroMemory(MaterialHits, sizeof(MaterialHits));
		ZeroMemory(MaterialDistances, sizeof(MaterialDistances));
		RaycastDistances.reserve(DETAILED_DIRECTIONS_COUNT);
	}

	// Helper functions for continuous transitions
	static float SmoothStep(float x)
	{
		return x * x * (3.0f - 2.0f * x);
	}
	static float ExponentialDecay(float x, float strength)
	{
		return expf(-x * strength);
	}
	static float GaussianBell(float x, float center, float width)
	{
		float exponent = -((x - center) * (x - center)) / (2.0f * width * width);
		return expf(exponent);
	}
	static float LogisticGrowth(float x, float steepness, float midpoint)
	{
		return 1.0f / (1.0f + expf(-steepness * (x - midpoint)));
	}
	static float InversePower(float x, float power, float scale)
	{
		return 1.0f / (1.0f + powf(x / scale, power));
	}
	static float InterpolateLinear(float a, float b, float t)
	{
		return ((a) + ((b) - (a)) * (t));
	}
};
