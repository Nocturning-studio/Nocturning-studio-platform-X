///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_common.h
// Author: NSDeathman
///////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "xr_collide_defs.h" // Нужно для Fvector и типов движка

// Directions for raycasting
static const Fvector BalancedHemisphereDirections[] = {
	{0.000000f, 0.000000f, 1.000000f},	 {0.577350f, 0.577350f, 0.577350f},	  {0.577350f, -0.577350f, 0.577350f},
	{-0.577350f, 0.577350f, 0.577350f},	 {-0.577350f, -0.577350f, 0.577350f}, {0.850651f, 0.000000f, 0.525731f},
	{-0.850651f, 0.000000f, 0.525731f},	 {0.525731f, 0.850651f, 0.000000f},	  {0.525731f, -0.850651f, 0.000000f},
	{0.000000f, 0.525731f, 0.850651f},	 {0.000000f, -0.525731f, 0.850651f},  {0.309017f, 0.000000f, 0.951057f},
	{-0.309017f, 0.000000f, 0.951057f},	 {0.000000f, 0.309017f, 0.951057f},	  {0.000000f, -0.309017f, 0.951057f},
	{0.809017f, 0.309017f, 0.500000f},	 {0.809017f, -0.309017f, 0.500000f},  {-0.809017f, 0.309017f, 0.500000f},
	{-0.809017f, -0.309017f, 0.500000f}, {0.500000f, 0.809017f, 0.309017f},	  {0.500000f, -0.809017f, 0.309017f},
	{-0.500000f, 0.809017f, 0.309017f},	 {-0.500000f, -0.809017f, 0.309017f}};

static const u32 BALANCED_DIRECTIONS_COUNT = sizeof(BalancedHemisphereDirections) / sizeof(Fvector);
static const float SMART_RAY_DISTANCE = 200.0f;

/**
 * @brief Detailed sphere directions for comprehensive environment analysis
 */
static const Fvector3 DetailedSphereDirections[] = {
	// Основные оси (6 направлений)
	{1.000000, 0.000000, 0.000000},
	{-1.000000, 0.000000, 0.000000},
	{0.000000, 1.000000, 0.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, -1.000000},
	// Углы куба (8 направлений)
	{0.577350, 0.577350, 0.577350},
	{0.577350, 0.577350, -0.577350},
	{0.577350, -0.577350, 0.577350},
	{-0.577350, -0.577350, 0.577350},
	{-0.577350, -0.577350, -0.577350},
	// Дополнительные направления для лучшего покрытия (42 направления)
	{0.850651, 0.000000, 0.525731},
	{-0.850651, 0.000000, 0.525731},
	{0.525731, 0.850651, 0.000000},
	{0.525731, -0.850651, 0.000000},
	{0.000000, 0.525731, 0.850651},
	{0.000000, -0.525731, 0.850651},
	{0.309017, 0.000000, 0.951057},
	{-0.309017, 0.000000, 0.951057},
	{0.000000, 0.309017, 0.951057},
	{0.000000, -0.309017, 0.951057},
	{0.809017, 0.309017, 0.500000},
	{0.809017, -0.309017, 0.500000},
	{-0.809017, 0.309017, 0.500000},
	{-0.809017, -0.309017, 0.500000},
	{0.500000, 0.809017, 0.309017},
	{0.500000, -0.809017, 0.309017},
	{-0.500000, 0.809017, 0.309017},
	{-0.500000, -0.809017, 0.309017},
	{0.309017, 0.500000, 0.809017},
	{-0.309017, 0.500000, 0.809017},
	{0.309017, -0.500000, 0.809017},
	{-0.309017, -0.500000, 0.809017},
	{0.000000, 0.809017, 0.587785},
	{0.000000, -0.809017, 0.587785},
	{0.587785, 0.000000, 0.809017},
	{-0.587785, 0.000000, 0.809017},
	{0.688191, 0.500000, 0.525731},
	{-0.688191, 0.500000, 0.525731},
	{0.688191, -0.500000, 0.525731},
	{-0.688191, -0.500000, 0.525731},
	{0.500000, 0.688191, 0.525731},
	{-0.500000, 0.688191, 0.525731},
	{0.500000, -0.688191, 0.525731},
	{-0.500000, -0.688191, 0.525731},
	{0.525731, 0.500000, 0.688191},
	{-0.525731, 0.500000, 0.688191},
	{0.525731, -0.500000, 0.688191},
	{-0.525731, -0.500000, 0.688191},
	{0.362357, 0.262866, 0.894427},
	{-0.362357, 0.262866, 0.894427},
	{0.362357, -0.262866, 0.894427},
	{-0.362357, -0.262866, 0.894427},
	{0.262866, 0.362357, 0.894427},
	{-0.262866, 0.362357, 0.894427},
	{0.262866, -0.362357, 0.894427},
	{-0.262866, -0.362357, 0.894427},
	{0.425325, 0.688191, 0.587785},
	{-0.425325, 0.688191, 0.587785},
	{0.425325, -0.688191, 0.587785},
	{-0.425325, -0.688191, 0.587785}};
static const u32 DETAILED_DIRECTIONS_COUNT = sizeof(DetailedSphereDirections) / sizeof(Fvector3);


// Material types constants
enum EMaterialType
{
	MATERIAL_AIR = 0,
	MATERIAL_STONE = 1,
	MATERIAL_METAL = 2,
	MATERIAL_WOOD = 3,
	MATERIAL_SOFT = 4,
	MATERIAL_GLASS = 5
};

// Structure for storing raycast hit information
// ПЕРЕНЕСЕНО СЮДА из Pathtracing.h во избежание ошибок C3646
struct SRayHitInfo
{
	Fvector vPosition;
	Fvector vNormal;
	float fDistance;
	u32 material_type;
	bool bHit;

	SRayHitInfo() : fDistance(0), material_type(0), bHit(false)
	{
		vPosition.set(0, 0, 0);
		vNormal.set(0, 0, 0);
	}
};

struct SPathTracingResult
{
	float fTotalEnergy;
	float fTotalDelay;
	u32 iNumBounces;
	float fAverageDistance;
	float fEnergyDistribution;

	float fEarlyReflections;
	float fLateReverberation;
	float fDiffuseEnergy;
	float fSpecularEnergy;

	SPathTracingResult()
		: fTotalEnergy(0), fTotalDelay(0), iNumBounces(0), fAverageDistance(0), fEnergyDistribution(0),
		  fEarlyReflections(0), fLateReverberation(0), fDiffuseEnergy(0), fSpecularEnergy(0)
	{
	}
};

struct SGeometryAnalysis
{
	float fApproxVolume;
	float fSurfaceArea;
	float fAvgCeilingHeight;
	float fAvgWallDistance;
	float fComplexity;
	float fSymmetry;
	float fVerticality;
	float fCompactness;
	float fIrregularity;
	Fvector vMainAxis;
};

struct SEAXEnvironmentData
{
	LONG lRoom;
	LONG lRoomHF;
	float flRoomRolloffFactor;
	float flDecayTime;
	float flDecayHFRatio;
	LONG lReflections;
	float flReflectionsDelay;
	LONG lReverb;
	float flReverbDelay;
	float flEnvironmentSize;
	float flEnvironmentDiffusion;
	float flAirAbsorptionHF;
	DWORD dwFlags;

	u32 dwFrameStamp;
	bool bDataValid;

	SEAXEnvironmentData()
	{
		Reset();
	}

	void Reset()
	{
		lRoom = -1000;
		lRoomHF = -100;
		flRoomRolloffFactor = 0.0f;
		flDecayTime = 1.49f;
		flDecayHFRatio = 0.83f;
		lReflections = -2602;
		flReflectionsDelay = 0.007f;
		lReverb = 200;
		flReverbDelay = 0.011f;
		flEnvironmentSize = 7.5f;
		flEnvironmentDiffusion = 1.0f;
		flAirAbsorptionHF = -5.0f;
		dwFlags = 0x3;
		dwFrameStamp = 0;
		bDataValid = false;
	}

	void CopyFrom(const SEAXEnvironmentData& other)
	{
		lRoom = other.lRoom;
		lRoomHF = other.lRoomHF;
		flRoomRolloffFactor = other.flRoomRolloffFactor;
		flDecayTime = other.flDecayTime;
		flDecayHFRatio = other.flDecayHFRatio;
		lReflections = other.lReflections;
		flReflectionsDelay = other.flReflectionsDelay;
		lReverb = other.lReverb;
		flReverbDelay = other.flReverbDelay;
		flEnvironmentSize = other.flEnvironmentSize;
		flEnvironmentDiffusion = other.flEnvironmentDiffusion;
		flAirAbsorptionHF = other.flAirAbsorptionHF;
		dwFlags = other.dwFlags;
		dwFrameStamp = other.dwFrameStamp;
		bDataValid = other.bDataValid;
	}
};
