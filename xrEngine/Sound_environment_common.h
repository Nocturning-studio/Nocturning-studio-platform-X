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

	SPhysicalAcoustics()
		: fRoomVolume(100.0f), fEchoStrength(0.3f), fReverbTime(1.0f), fCriticalDistance(5.0f), fAdjustedRadius(10.0f)
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
 * @brief Main EAX environment data structure
 */
struct SEAXEnvironmentData
{
	SReflectionAnalysis sReflectionData; ///< Reflection analysis results for EAX
	SPhysicalAcoustics sPhysicalData;	 ///< Physical acoustic parameters for EAX

	// Core geometric parameters
	float fEnvironmentRadius;	///< Average environment radius in meters
	float fEnvironmentVariance; ///< Distance variance (0-1)
	float fEnvironmentDensity;	///< Environment density (0-1)

	// Acoustic properties
	float fFogDensity;	 ///< Fog/air density for EAX air absorption
	float fRoomSize;	 ///< Normalized room size (0-1)
	float fReflectivity; ///< Surface reflection coefficient (0-1)

	// Derived parameters for EAX
	float fOpenness;	 ///< Space openness degree (0-1) for EAX
	float fEnclosedness; ///< Space enclosedness degree (0-1) for EAX
	float fUniformity;	 ///< Surface distribution uniformity (0-1)

	// Timestamps and validation
	u32 dwFrameStamp; ///< Frame when data was updated
	bool bDataValid;  ///< Data validity flag

	/// Default constructor
	SEAXEnvironmentData()
	{
		Reset();
	}

	/// Resets to default values optimized for EAX
	void Reset()
	{
		fEnvironmentRadius = 10.0f;
		fEnvironmentVariance = 0.5f;
		fEnvironmentDensity = 0.5f;
		fFogDensity = 0.0f;
		fRoomSize = 0.5f;
		fReflectivity = 0.5f;
		fOpenness = 0.5f;
		fEnclosedness = 0.5f;
		fUniformity = 0.5f;

		// Reset physical parameters for EAX
		sPhysicalData.fRoomVolume = 100.0f;
		sPhysicalData.fEchoStrength = 0.3f;
		sPhysicalData.fReverbTime = 1.0f;
		sPhysicalData.fCriticalDistance = 5.0f;
		sPhysicalData.fAdjustedRadius = 10.0f;

		// Reset reflection analysis for EAX
		sReflectionData.fPrimaryReflections = 0.1f;
		sReflectionData.fSecondaryReflections = 0.05f;
		sReflectionData.fReflectionDelay = 0.02f;
		sReflectionData.fReflectionEnergy = 0.1f;
		sReflectionData.fSurfaceComplexity = 0.3f;

		dwFrameStamp = 0;
		bDataValid = false;
	}

	/// Copies data from another instance
	void CopyFrom(const SEAXEnvironmentData& other)
	{
		fEnvironmentRadius = other.fEnvironmentRadius;
		fEnvironmentVariance = other.fEnvironmentVariance;
		fEnvironmentDensity = other.fEnvironmentDensity;
		fFogDensity = other.fFogDensity;
		fRoomSize = other.fRoomSize;
		fReflectivity = other.fReflectivity;
		fOpenness = other.fOpenness;
		fEnclosedness = other.fEnclosedness;
		fUniformity = other.fUniformity;

		// Copy physical parameters for EAX
		sPhysicalData.fRoomVolume = other.sPhysicalData.fRoomVolume;
		sPhysicalData.fEchoStrength = other.sPhysicalData.fEchoStrength;
		sPhysicalData.fReverbTime = other.sPhysicalData.fReverbTime;
		sPhysicalData.fCriticalDistance = other.sPhysicalData.fCriticalDistance;
		sPhysicalData.fAdjustedRadius = other.sPhysicalData.fAdjustedRadius;

		// Copy reflection analysis for EAX
		sReflectionData.fPrimaryReflections = other.sReflectionData.fPrimaryReflections;
		sReflectionData.fSecondaryReflections = other.sReflectionData.fSecondaryReflections;
		sReflectionData.fReflectionDelay = other.sReflectionData.fReflectionDelay;
		sReflectionData.fReflectionEnergy = other.sReflectionData.fReflectionEnergy;
		sReflectionData.fSurfaceComplexity = other.sReflectionData.fSurfaceComplexity;

		dwFrameStamp = other.dwFrameStamp;
		bDataValid = other.bDataValid;
	}
};