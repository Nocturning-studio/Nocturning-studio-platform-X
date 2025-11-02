///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Balanced hemisphere directions for initial sampling
 */
static const Fvector3 BalancedHemisphereDirections[] = {
	{0.000000, 0.000000, 1.000000},	  {0.577350, 0.577350, 0.577350},	{0.577350, -0.577350, 0.577350},
	{-0.577350, 0.577350, 0.577350},  {-0.577350, -0.577350, 0.577350}, {0.850651, 0.000000, 0.525731},
	{-0.850651, 0.000000, 0.525731},  {0.525731, 0.850651, 0.000000},	{0.525731, -0.850651, 0.000000},
	{0.000000, 0.525731, 0.850651},	  {0.000000, -0.525731, 0.850651},	{0.309017, 0.000000, 0.951057},
	{-0.309017, 0.000000, 0.951057},  {0.000000, 0.309017, 0.951057},	{0.000000, -0.309017, 0.951057},
	{0.809017, 0.309017, 0.500000},	  {0.809017, -0.309017, 0.500000},	{-0.809017, 0.309017, 0.500000},
	{-0.809017, -0.309017, 0.500000}, {0.500000, 0.809017, 0.309017},	{0.500000, -0.809017, 0.309017},
	{-0.500000, 0.809017, 0.309017},  {-0.500000, -0.809017, 0.309017}};

static const u32 BALANCED_DIRECTIONS_COUNT = sizeof(BalancedHemisphereDirections) / sizeof(Fvector3);
static const float SMART_RAY_DISTANCE = 10000.0f;

/**
 * @brief Structure for storing raycast hit information
 */
struct SRayHitInfo
{
	Fvector vPosition; ///< Hit position in world space
	Fvector vNormal;   ///< Surface normal at hit point
	float fDistance;   ///< Distance from ray origin to hit point
	u32 material_type; ///< Classified material type (0=air, 1-4=surfaces)
	bool bHit;		   ///< Whether ray hit anything

	SRayHitInfo() : fDistance(0), material_type(0), bHit(false)
	{
		vPosition.set(0, 0, 0);
		vNormal.set(0, 0, 0);
	}
};

/**
 * @brief Reflection analysis results
 */
struct SReflectionAnalysis
{
	float fPrimaryReflections;	 ///< Primary reflection intensity
	float fSecondaryReflections; ///< Secondary reflection intensity
	float fReflectionDelay;		 ///< Average reflection delay time
	float fReflectionEnergy;	 ///< Total reflection energy
	float fSurfaceComplexity;	 ///< Surface complexity factor (0-1)
};

/**
 * @brief Physical acoustic parameters
 */
struct SPhysicalAcoustics
{
	float fRoomVolume;		 ///< Room volume in cubic meters
	float fEchoStrength;	 ///< Echo strength (0-1) for EAX
	float fReverbTime;		 ///< Reverberation time in seconds for EAX
	float fCriticalDistance; ///< Critical distance in meters
	float fAdjustedRadius;	 ///< Adjusted radius accounting for openness for EAX
	float fEchoDelay;		 ///< Echo delay time in seconds for EAX

	SPhysicalAcoustics()
		: fRoomVolume(100.0f), fEchoStrength(0.3f), fReverbTime(1.0f), fCriticalDistance(5.0f), fAdjustedRadius(10.0f),
		  fEchoDelay(0.02f)
	{
	}
};

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

/**
 * @brief Pure EAX parameters structure - only what's needed for EAX API
 */
struct SEAXEnvironmentData
{
	// Core EAX parameters (mapped directly to EAX properties)
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

	// Timestamps and validation
	u32 dwFrameStamp;
	bool bDataValid;

	/// Default constructor
	SEAXEnvironmentData()
	{
		Reset();
	}

	/// Resets to default EAX values
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
		dwFlags = 0x3; // EAXLISTENERFLAGS_DECAYTIMESCALE | EAXLISTENERFLAGS_REFLECTIONSDELAYSCALE

		dwFrameStamp = 0;
		bDataValid = false;
	}

	/// Copies data from another instance
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