#pragma once
#include "xrRender_console.h"

const float	SMAP_near_plane		= 0.1;
const u32	SMAP_adapt_min		= 32;
const u32	SMAP_adapt_optimal	= 768;
const u32	SMAP_adapt_max		= 1536;

const u32 TEX_material_LdotN	= 128;	// diffuse,		X, almost linear = small res
const u32 TEX_material_LdotH	= 256;	// specular,	Y
const u32 TEX_material_Count	= 4;	// Number of materials,	Z

const u32 TEX_jitter		= 64;
const u32 TEX_jitter_count	= 5;

const u32 BLOOM_size_X = 256;
const u32 BLOOM_size_Y = 256;
const u32 LUMINANCE_size = 16;

const float r__detail_density_min = 0.6f;

const FLOAT rgba_black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
const FLOAT rgba_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

// RT texture names

#define tex_rt_Planar_shadow	"$user$planarshadow"
#define tex_rt_Planar_color_ms	"$user$planarcolorms"
#define tex_rt_Planar_color		"$user$planarcolor"
#define tex_rt_Planar_depth		"$user$planardepth"
#define tex_rt_Position_IL		"$user$positionil"
#define tex_rt_Normal_IL		"$user$normalil"
#define tex_rt_Color_IL			"$user$albedoil"
//#define tex_rt_MSAADepth		"$user$msaadepth"
#define tex_rt_ZPrepass			"$user$zprepass"
#define tex_rt_Position_prev	"$user$positionprev"
#define tex_rt_Color_ms			"$user$msaaalbedo"
#define tex_rt_Position			"$user$position"
#define tex_rt_Accumulator		"$user$accum"
#define tex_rt_Accumulator_1	"$user$accum1"
#define tex_rt_Color			"$user$albedo"
#define tex_rt_Generic_0		"$user$generic0"
#define tex_rt_Generic_1		"$user$generic1"
#define tex_rt_Generic_0_ms		"$user$generic0ms"
#define tex_rt_Generic_1_ms		"$user$generic1ms"
#define tex_rt_Generic_2		"$user$generic2"
#define tex_rt_PPTemp			"$user$genericpp"
#define tex_rt_RSM				"$user$rsm"
#define tex_rt_RSM_copy			"$user$rsmcopy"
#define tex_rt_RSM_prev			"$user$rsmprev"
#define tex_rt_RSM_depth		"$user$rsmdepth"
//#define tex_rt_RSM_depth_prev	"$user$rsmdepthprev"
#define tex_rt_Generic			"$user$generic"
#define tex_rt_MLAA_0			"$user$mlaa0"
#define tex_rt_MLAA_1			"$user$mlaa1"
#define tex_rt_Depth_1			"$user$depth1"
#define tex_rt_smap_depth		"$user$smap_depth"
#define tex_rt_vsm_depth		"$user$vsm_depth"
#define tex_rt_vsm_depthms		"$user$vsm_depthms"
#define tex_rt_Bloom_1			"$user$bloom1"
#define tex_rt_Bloom_2			"$user$bloom2"
#define tex_rt_LUM_8			"$user$lum_t8"
#define tex_rt_LUM_64			"$user$lum_t64"
#define tex_rt_LUM_pool			"$user$luminance"
#define tex_t_noise_mipped_		"$user$jitter_mipped"
#define tex_t_noise_			"$user$jitter_"
#define tex_t_LUM_src			"$user$tonemap_src"
#define tex_t_LUM_dest			"$user$tonemap"
#define tex_t_envmap_0			"$user$env_s0"
#define tex_t_envmap_1			"$user$env_s1"
#define tex_t_material			"$user$material"
#define tex_t_sky_0				"$user$sky0"
#define tex_t_sky_1				"$user$sky1"

//#define tex_rt_Position_small		"$user$positionsmall"
//#define tex_rt_Accumulator_small	"$user$accumsmall"
//#define tex_rt_Color_small			"$user$albedosmall"
#define tex_rt_Generic_0_small		"$user$generic0small"
#define tex_rt_Generic_1_small		"$user$generic1small"

#define tex_rt_HBAO_plus_normal		"$user$ssao_normal"	// DX11 only

#define tex_rt_SSAO					"$user$ssao_ssao"	// DX11 only
#define tex_rt_SSAO_temp			"$user$ssao_temp0"	// DX11 only
#define tex_rt_SSAO_prev			"$user$ssao_prev"	// DX11 only
#define tex_rt_SSAO_small			"$user$ssao_small"	// DX11 only

#define tex_rt_SSPR					"$user$sspr"				// DX11 only
#define tex_rt_smap_depth_minmax	"$user$smap_depth_minmax"	// DX11 only
#define tex_t_depth					"$user$depth"
#define tex_t_msaa_depth			"$user$msaadepth"

#define tex_rt_Generic_0_feedback	"$user$generic0feedbackrt"
//#define tex_rt_Generic_1_feedback	"$user$generic1feedbackrt"
#define tex_rt_Motion				"$user$motion"
#define tex_rt_Motion_ms			"$user$motionms"

#define tex_rt_SSAA_depth			"$user$ssaadepth"
#define tex_rt_SSAA_color			"$user$ssaacolor"
#define tex_rt_SSAA_distort			"$user$ssaadistort"

#define TAA_FEEDBACK_SIZE 7

struct rt_TAA_params_type
{
	LPCSTR shader_name;
	LPCSTR texture_name;
};

static rt_TAA_params_type rt_TAA_params[TAA_FEEDBACK_SIZE] =
{
	{"s_taa_0", "$user$taa_0"},
	{"s_taa_1", "$user$taa_1"},
	{"s_taa_2", "$user$taa_2"},
	{"s_taa_3", "$user$taa_3"},
	{"s_taa_4", "$user$taa_4"},
	{"s_taa_5", "$user$taa_5"},
	{"s_taa_6", "$user$taa_6"},
};

static LPCSTR dx10_msaa_id = "iSample";

// Antialiasing type defs

enum r__aa_values // for RImplementation.o.aa_mode
{
	AA_MLAA = 10,
	AA_FXAA = 20,
	AA_MSAA = 30, AA_MSAA2S, AA_MSAA4S, AA_MSAA8S, AA_TXAA2S, AA_TXAA4S, AA_MSAA_FXAA,
	AA_TAA = 40,
	AA_TAA_V2 = 50,
	AA_TXAA = 60,
};

enum r__ssaa_values // for RImplementation.o.ssaa
{
	USE_SSAA	= 1,
	SSAA2X		= 2,
	SSAA4X		= 4,
	USE_FSR		= 1000000,
	FSR_SSAA169	= 5050169,
	FSR_SSAA225	= 6035225,
	FSR_SSAA289	= 7025289,
	FSR_SSAA400	= 8015400,
};

// Sunshafts
enum r__sunshafts_mode_values
{ 
	SUNSHAFTS_SCREEN = 1, 
	SUNSHAFTS_VOLUME,
};

// SSAO
enum r__ssao_mode_values
{
	SSAO_SSAO = 1,
	SSAO_PATH_TRACING,
	SSAO_HBAO_PLUS,
};

// DEV
#define DEVX r__free_vector.x
#define DEVY r__free_vector.y
#define DEVZ r__free_vector.z
#define DEVW r__free_vector.w

#define TEX_TORCH_ATT	"internal\\internal_light_att"
#define TEX_TORCH_ATT2	"internal\\internal_light_torch_r2"
#define TEX_POINT_ATT	"internal\\internal_light_attpoint"
#define TEX_SPOT_ATT	"internal\\internal_light_attclip"

#define JITTER(a) tex_t_noise_ #a

IC	float	u_diffuse2s(float x, float y, float z) { float	v = (x + y + z) / 3.f;	return r__gloss * ((v < 1) ? powf(v, 2.f / 3.f) : v); }
IC	float	u_diffuse2s(Fvector3& c) { return u_diffuse2s(c.x, c.y, c.z); }