///////////////////////////////////////////////////////////////////////////////////
// Created: 10.12.2024
// Author: NSDeathman
// Improved EAX calculations
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_common.h"
#include "Sound_environment_reflections.h"
#include "Sound_environment_geometry.h"
#include "Sound_environment_pathtracing.h"
#include "xr_collide_defs.h"
///////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CSoundEnvironment;
///////////////////////////////////////////////////////////////////////////////////

class ENGINE_API CSoundEnvironment
{
  private:
	SEAXEnvironmentData m_CurrentData;
	SEAXEnvironmentData m_PrevData;
	CAdvancedReflectionAnalyzer m_ReflectionAnalyzer;

	u32 m_LastUpdatedFrame;
	static const u32 UPDATE_INTERVAL = 10;

	// ���������� ������������ ������
	class CPhysicalAcousticModel
	{
	  private:
		static const float SOUND_SPEED;			// �������� ����� �/�
		static const float AIR_ABSORPTION_COEF; // ����������� ���������� �������

	  public:
		// ���������� ����������� ��� �� ������ ���������
		float CalculateEchoStrength(float volume, float reflectivity, float openness);

		// ������ ������� ������������ (���������� ������� ������)
		float CalculateReverbTime(float volume, float absorption);

		// ������ ������������ ����������
		float CalculateCriticalDistance(float volume);
	};

	CPhysicalAcousticModel m_AcousticModel;

	// ����� ������� ������� ���������
	class CGeometryAnalyzer m_GeometryAnalyzer;
	SGeometryAnalysis m_CurrentGeometry;

	// ������������������� �����������
	float PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type, bool bCollectInfo = false);
	void AnalyzeSpatialDistribution(Fvector center, std::vector<float>& distances, SEAXEnvironmentData& result);

	class CPathTracingSystem m_PathTracer;
	SPathTracingResult m_PathTracingResult;

  public:
	void __stdcall MT_CALC();
	void CalculateEnhancedEnvironmentData();
	void Update();
	bool NeedUpdate() const;

	float PerformSmartRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type);

	const SEAXEnvironmentData& GetEnvironmentData() const
	{
		return m_CurrentData;
	}

	CSoundEnvironment();
	~CSoundEnvironment();
};