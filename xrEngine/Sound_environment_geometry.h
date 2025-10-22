#pragma once
#include "stdafx.h"
#include "Sound_environment_common.h"
#include "Sound_environment_reflections.h"
#include "Sound_environment_common.h"

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
	void AnalyzeEnvironmentGeometry(Fvector start_pos, SGeometryAnalysis& result);
	float CalculateVolumeMonteCarlo(Fvector center, float estimated_radius);
	void BuildVoxelApproximation(Fvector center, float radius, SVoxelGrid& grid);
	Fvector EstimateMainAxis(const std::vector<Fvector>& hit_points);
	float CalculateSurfaceComplexity(const SVoxelGrid& grid);
};
