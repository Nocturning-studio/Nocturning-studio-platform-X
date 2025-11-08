#pragma once

//#define FEATURE_LEVELS_DEBUG

namespace RenderCreationParams
{
struct RendererSupport
{
#ifdef FEATURE_LEVELS_DEBUG
	D3D_FEATURE_LEVEL level;
#endif
};

enum
{
	R_R1, // xrRender_R1
	R_R2, // xrRender_R2
	R_R4 // xrRender_R4
};

static u32 base_value = R_R1;

static const D3D_FEATURE_LEVEL levels[] = {
#ifdef USE_DX11_3
	D3D_FEATURE_LEVEL_11_1,
#endif
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
};

static const UINT count = sizeof levels / sizeof levels[0];

#ifdef USE_DX11_3
static const D3D_FEATURE_LEVEL levels12[] = {
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
};

static const UINT count12 = sizeof levels12 / sizeof levels12[0];
#endif
} // namespace RenderCreationParams