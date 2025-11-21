///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_geometry.h
// Author: NSDeathman
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Sound_environment_common.h"
#include <vector>

// struct SGeometryAnalysis удалена, так как она есть в Common.h

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
