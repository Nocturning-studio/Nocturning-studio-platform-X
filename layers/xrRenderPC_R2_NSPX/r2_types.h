#pragma once

// depth-stencil buffer
#define r2_RT_ZB "$user$zb"

// Geometry Buffer
#define r2_RT_GBuffer_Position "$user$gbuffer_position"
#define r2_RT_GBuffer_Normal "$user$gbuffer_normal"
#define r2_RT_GBuffer_Albedo "$user$gbuffer_albedo"

// Light Accumulating
#define r2_RT_Diffuse_Accumulator "$user$accumulator_diffuse"
#define r2_RT_Specular_Accumulator "$user$accumulator_specular"

// Environment
#define r2_T_envs0 "$user$env_s0"
#define r2_T_envs1 "$user$env_s1"
#define r2_T_sky0 "$user$sky0"
#define r2_T_sky1 "$user$sky1"

// Output textures
#define r2_RT_generic0 "$user$generic0"
#define r2_RT_generic2 "$user$generic2"	//	Igor: for volumetric lights

#define r2_RT_distortion_mask "$user$distortion"

// Motion blur previous frame
#define r2_RT_mblur_saved_frame "$user$mblur_saved_frame"

#define r2_RT_bloom1 "$user$bloom1"
#define r2_RT_bloom2 "$user$bloom2"

// Ambient occlusion
#define r2_RT_ao "$user$ao"

// Autoexposure
#define r2_RT_autoexposure_t64 "$user$lum_t64"
#define r2_RT_autoexposure_t8 "$user$lum_t8"
#define r2_RT_autoexposure_src "$user$autoexposure_src" // --- prev-frame-result
#define r2_RT_autoexposure_cur "$user$autoexposure"	 // --- result
#define r2_RT_autoexposure_pool "$user$autoexposure"		 // --- pool

// Shadow map
#define r2_RT_smap_surf "$user$smap_surf"	// --- directional
#define r2_RT_smap_depth "$user$smap_depth" // ---directional

#define r2_material "$user$material" // ---

#define r2_jitter "$user$jitter_"

#define r2_sunmask "sunmask"

#define JITTER(a) r2_jitter #a

const float SMAP_near_plane = .1f;

const u32 SMAP_adapt_min = 32;
const u32 SMAP_adapt_optimal = 768;
const u32 SMAP_adapt_max = 1536;

const u32 TEX_material_LdotN = 128; // diffuse,		X, almost linear = small res
const u32 TEX_material_LdotH = 256; // specular,	Y
const u32 TEX_jitter = 64;
const u32 TEX_jitter_count = 4;

const u32 BLOOM_size_X = 256;
const u32 BLOOM_size_Y = 256;
const u32 LUMINANCE_size = 16;

// deffer
#define SE_R2_NORMAL_HQ 0 // high quality/detail
#define SE_R2_NORMAL_LQ 1 // low quality
#define SE_R2_SHADOW 2	  // shadow generation

// spot
#define SE_L_FILL 0
#define SE_L_UNSHADOWED 1
#define SE_L_NORMAL 2	  // typical, scaled
#define SE_L_FULLSIZE 3	  // full texture coverage
#define SE_L_TRANSLUENT 4 // with opacity/color mask

// mask
#define SE_MASK_SPOT 0
#define SE_MASK_POINT 1
#define SE_MASK_DIRECT 2

// sun
#define SE_SUN_NEAR 0
#define SE_SUN_MIDDLE 1
#define SE_SUN_FAR 2

extern float ps_r2_gloss_factor;

IC float u_diffuse2s(float x, float y, float z)
{
	float v = (x + y + z) / 3.f;
	return ps_r2_gloss_factor * ((v < 1) ? powf(v, 2.f / 3.f) : v);
}

IC float u_diffuse2s(Fvector3& c)
{
	return u_diffuse2s(c.x, c.y, c.z);
}
