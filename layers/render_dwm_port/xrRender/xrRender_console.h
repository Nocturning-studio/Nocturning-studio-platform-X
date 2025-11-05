#ifndef xrRender_consoleH
#define xrRender_consoleH
#pragma once

#define $bit$ 1u << // Set current bit [0..x]

enum r__opt_flags_values
{
	R__USE_SPLIT_SCENE		= $bit$ 0u,
	R__USE_SUN_ZCUL			= $bit$ 1u,
	R__USE_TONEMAP			= $bit$ 2u,
	R__USE_NVDBT			= $bit$ 3u,
	R__USE_ADVANCED_MODE	= $bit$ 4u,		// SSAO, TAA/MLAA, Volume Lights, etc.
	R__USE_BUMP				= $bit$ 5u,		// static sun mode only
	R__USE_DYN_SHADOWS		= $bit$ 6u,		// static sun mode only
	R__USE_TEX_STAGING		= $bit$ 8u,		// DX11 only
	R__NEED_NVDSTENCIL		= $bit$ 9u,
	R__USE_TRANS_SHADOWS	= $bit$ 10u,
	R__USE_GBD_OPT			= $bit$ 11u,
	R__USE_VSM				= $bit$ 12u,
	R__USE_DYNAMIC_HUD		= $bit$ 13u,
};

enum r__adv_opt_flags_values
{
	R__USE_WET_SURFACES		= $bit$  0u,
	R__USE_VSMOKE			= $bit$  1u,	// DX11 only
	R__USE_SSR				= $bit$  2u,
	R__REP_SSR_TO_PLANAR	= $bit$  3u,	// DX11 only
	R__USE_VLIGHT			= $bit$  4u,
	R__USE_CAS				= $bit$  5u,
	R__USE_CSPECULAR		= $bit$  6u,
	R__USE_WATER_DIST		= $bit$  7u,
	R__NEED_MINMAX_SM		= $bit$  8u,	// DX11 only
	R__NEED_TESSELATION		= $bit$  9u,	// DX11 only
	R__USE_DOF				= $bit$ 10u,
	R__USE_SOFT_WATER		= $bit$ 11u,
	R__USE_SOFT_PARTICLES	= $bit$ 12u,
	R__USE_SUN_DETAILS		= $bit$ 13u,
	R__USE_HUD_SHADOWS		= $bit$ 14u,
	R__USE_SUN_IL			= $bit$ 15u,
	R__USE_PLANAR			= $bit$ 16u,
	R__USE_PLANAR_DETAILS	= $bit$ 17u,
	R__USE_SPOT_IL			= $bit$ 18u,
	R__USE_LONG_SHADOWS		= $bit$ 19u,
	R__PT_DOWNSAMPLE		= $bit$ 20u,
	R__FSR_16				= $bit$ 21u,
	R__USE_SSAO_PT_BLUR		= $bit$ 22u,
};

enum r__dbg_opt_flags_values
{
	R__DBG_DRAW_WIREFRAME	= $bit$ 0u,		// DX11 only
	R__DBG_FLAT_WATER		= $bit$ 1u,		
	R__DBG_NOALBEDO			= $bit$ 2u,		
	R__DBG_MSAA_HACK		= $bit$ 3u,		
};

extern ECORE_API BOOL	opt(r__opt_flags_values		flag);
extern ECORE_API BOOL	opt(r__adv_opt_flags_values flag);
extern ECORE_API BOOL	opt(r__dbg_opt_flags_values flag);

extern ECORE_API Flags32	r__adv_opt_flags;
extern ECORE_API Flags32	r__dbg_opt_flags;
extern ECORE_API Flags32	r__opt_flags;

extern ECORE_API Fvector4	r__free_vector;

extern ECORE_API Fvector4	r__sun_il_params_0;
extern ECORE_API Fvector4	r__sun_il_params_1;
extern ECORE_API Fvector4	r__sun_il_params_2;
extern ECORE_API Fvector4	r__sun_il_params_3;

extern ECORE_API float		r__cas_contrast;
extern ECORE_API float		r__cas_sharpening;

extern ECORE_API float		r__dtex_range;

extern ECORE_API float		r__wallmark_ttl;
extern ECORE_API float		r__wallmark_shift_pp;
extern ECORE_API float		r__wallmark_shift_v;

extern ECORE_API float		r__geometry_lod;

extern ECORE_API float		r__detail_density;
extern ECORE_API float		r__detail_l_ambient;
extern ECORE_API float		r__detail_l_aniso;

extern ECORE_API float		r__ssa_glod_start;
extern ECORE_API float		r__ssa_glod_end;
extern ECORE_API float		r__ssa_lod_a;		// dynamic & static objects lod
extern ECORE_API float		r__ssa_lod_b;		// dynamic & static objects lod

extern ECORE_API float		r__tonemap_middlegray;
extern ECORE_API float		r__tonemap_adaptation;
extern ECORE_API float		r__tonemap_low_lum;
extern ECORE_API float		r__tonemap_amount;
extern ECORE_API float		r__tonemap_overbright;

extern ECORE_API float		r__bloom_kernel_g;
extern ECORE_API float		r__bloom_kernel_b;
extern ECORE_API float		r__bloom_speed;
extern ECORE_API float		r__bloom_kernel_scale;
extern ECORE_API float		r__bloom_threshold;

extern ECORE_API float		r__ssa_discard;		// not cmd command
extern ECORE_API float		r__ssa_dontsort;	// not cmd command
extern ECORE_API float		r__ssa_hzb_vs_tex;	// not cmd command

extern ECORE_API int		r__lsleep_frames;

extern ECORE_API int		r__tf_aniso;
extern ECORE_API float		r__tf_mipbias;

extern ECORE_API float		r__smap_quality;

extern ECORE_API float		r__zfill_depth;

extern ECORE_API float		r__sun_near;
extern ECORE_API float		r__sun_depth_near_scale;
extern ECORE_API float		r__sun_depth_near_bias;		// DX9 only

extern ECORE_API float		r__sun_far;
extern ECORE_API float		r__sun_depth_far_scale;
extern ECORE_API float		r__sun_depth_far_bias;		// DX9 only

extern ECORE_API float		r__sun_lumscale;
extern ECORE_API float		r__sun_lumscale_hemi;
extern ECORE_API float		r__sun_lumscale_amb;

extern ECORE_API int		r__wait_sleep;

extern ECORE_API int		r__dhemi_count;
extern ECORE_API float		r__dhemi_sky_scale;
extern ECORE_API float		r__dhemi_light_scale;
extern ECORE_API float		r__dhemi_light_flow;

extern ECORE_API float		r__ls_depth_scale;
extern ECORE_API float		r__ls_depth_bias;

extern ECORE_API float		r__slight_fade;

extern ECORE_API Fvector3	r__dof;
extern ECORE_API float		r__dof_kernel;
extern ECORE_API float		r__dof_sky;

extern ECORE_API float		r__gloss;

extern ECORE_API float		r__ssaa_contrast;
extern ECORE_API float		r__ssaa_sharpness;

extern ECORE_API float		r__dbg_planar_h;
extern ECORE_API float		r__planar_bias_n; // normal bias
extern ECORE_API float		r__planar_bias_d; // depth bias

extern ECORE_API u32		r__msaa_reflections;

extern ECORE_API u32		r__ssaa;

extern ECORE_API u32		r__aa;
extern ECORE_API u32		r__taa_zprepass;
extern ECORE_API u32		r__taa_jitter_mode;
extern ECORE_API u32		r__smap_filter;		// full dynamic mode only
extern ECORE_API u32		r__smap_size;		// full dynamic mode only
extern ECORE_API u32		r__rain_smap_size;	// full dynamic mode only
extern ECORE_API u32		r__ssao_mode;
extern ECORE_API u32		r__parallax_mode;
extern ECORE_API u32		r__sunshafts_mode;

extern ECORE_API int		psSkeletonUpdate;

extern ECORE_API void		xrRender_initconsole();
extern ECORE_API void		xrRender_apply_tf();
#endif
