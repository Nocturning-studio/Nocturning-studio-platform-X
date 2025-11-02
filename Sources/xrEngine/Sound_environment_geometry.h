///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_common.h"
///////////////////////////////////////////////////////////////////////////////////
struct SGeometryAnalysis
{
	float fApproxVolume;	 // Приблизительный объем пространства (м³)
	float fSurfaceArea;		 // Приблизительная площадь поверхностей
	float fAvgCeilingHeight; // Средняя высота потолка
	float fAvgWallDistance;	 // Среднее расстояние до стен
	float fComplexity;		 // Сложность геометрии (0-1)
	float fSymmetry;		 // Симметричность помещения
	float fVerticality;		 // Вертикальная протяженность
	Fvector vMainAxis;		 // Основная ось помещения
};
///////////////////////////////////////////////////////////////////////////////////
class ENGINE_API CGeometryAnalyzer
{
  private:
	static const u32 MAX_SAMPLE_POINTS = 128;
	static const u32 VOXEL_RESOLUTION = 16;

	struct SVoxelGrid
	{
		bool bOccupied[VOXEL_RESOLUTION][VOXEL_RESOLUTION][VOXEL_RESOLUTION];
		Fvector vMinBounds;
		Fvector vMaxBounds;
		Fvector vVoxelSize;
	};

  public:
	Fvector EstimateMainAxis(const std::vector<Fvector>& hit_points);
	void AnalyzeEnvironmentGeometry(Fvector start_pos, SGeometryAnalysis& result);
};
///////////////////////////////////////////////////////////////////////////////////