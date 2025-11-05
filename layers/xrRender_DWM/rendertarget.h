#pragma once

#include "ColorMapManager.h"

class light;

#define	VOLUMETRIC_SLICES 100

struct CAS_const
{
	uint32_t const0[4], const1[4];
};

struct FSR_EASU_const
{
	uint32_t const0[4], const1[4], const2[4], const3[4];
};

struct FSR_RCAS_const
{
	uint32_t const0[4];
};

struct FSR_const
{
	FSR_EASU_const EASU_const;
	FSR_RCAS_const RCAS_const;
};

struct SSAA_params
{
	// scale
	float amount;
	
	// mip bias
	float mip;
	
	// constants
	CAS_const CAS_const;
	FSR_const FSR_const;

	// scaled screen size
	u32 w, h;
};

class CRenderTarget		: public IRender_Target
{
private:
	u32							dwWidth;
	u32							dwHeight;
	u32							dwAccumulatorClearMark;
	SSAA_params					SSAA;
public:
	enum	eStencilOptimizeMode
	{
		SO_Light = 0,	//	Default
		SO_Combine,		//	Default
	};

#ifdef DEBUG
	struct		dbg_line_t {
		Fvector	P0, P1;
		u32		color;
	};
	xr_vector<std::pair<Fsphere, Fcolor> >		dbg_spheres;
	xr_vector<dbg_line_t>										dbg_lines;
	xr_vector<Fplane>												dbg_planes;
#endif

	u32							dwLightMarkerID;
	// 
	IBlender*					b_occq;
	IBlender*					b_accum_mask;
	IBlender*					b_accum_direct;
	IBlender*					b_accum_point;
	IBlender*					b_accum_spot;
	IBlender*					b_accum_reflected;
	IBlender*					b_bloom;
	IBlender*					b_luminance;
	IBlender*					b_combine;
    IBlender*					b_sunshafts;
    IBlender*					b_ssao; // SSAO
	IBlender*					b_antialiasing; // Antialiasing
	IBlender*					b_taa;
	IBlender*					b_rsm;
	IBlender*					b_sspr;
    IBlender*					b_msaa_mark_edges;

	// SSAA render target
	ref_rt rt_SSAA_color;
	ref_rt rt_SSAA_distort;

	// SSAA depth stencil
	ref_rt rt_SSAA_depth;

	// MSAA & TXAA
	ref_rt	rt_Color_ms; // 64bit, r, g, b, ao map

	ref_rt	rt_Generic_0_ms; // 32bit, MSAA post-process, intermidiate results, etc.
	ref_rt	rt_Generic_1_ms; // 32bit, MSAA post-process, intermidiate results, etc.

	ref_rt rt_Generic_0_feedback; // TXAA feedback
	//ref_rt rt_Generic_1_feedback; // TXAA feedback

	ref_rt rt_Motion; // TXAA motions feedback
	ref_rt rt_Motion_ms; // TXAA motions feedback

	//Fmatrix m_txaa_vp_prev;

	// TAA not recursive
	ref_rt rt_TAA[TAA_FEEDBACK_SIZE];
	Fmatrix m_TAA[TAA_FEEDBACK_SIZE];

	ref_rt						rt_MSAA_depth;

	ref_rt						rt_Position_IL; // 16*4bit, light-w-space position
	ref_rt						rt_Normal_IL; // 16*4bit, light-w-space normal
	ref_rt						rt_Color_IL; // 8*4bit, light-space color

	ref_rt						rt_RSM; // filtered RSM
	ref_rt						rt_RSM_prev; // previous filtered RSM
	ref_rt						rt_RSM_copy; // copied RSM
	ref_rt						rt_RSM_depth; // depth for denoise
	//ref_rt						rt_RSM_depth_prev; // previous depth for denoise

	ref_rt						rt_SSPR; // Unordered adress view 2D texture

	ref_rt						rt_HBAO_plus_normal; // world-space normal

	ref_rt						rt_SSAO; // SSAO current data
	ref_rt						rt_SSAO_temp; // SSAO temporal data
	ref_rt						rt_SSAO_prev; // SSAO previous data
	ref_rt						rt_SSAO_small; // SSAO path trace result

	ref_rt						rt_Planar_color;
	ref_rt						rt_Planar_color_ms;
	ref_rt						rt_Planar_shadow;
	ref_rt						rt_Planar_depth;

	// Antialiasing
	ref_rt						rt_MLAA_0; // R8
	ref_rt						rt_MLAA_1; // R8G8

	// Depth
	ref_texture					t_depth;	// HW depth texture

	ref_rt						rt_Generic; // for some effect, temp, fullsize
	ref_rt						rt_PPTemp; // DWM-DO: temp, for pp 2, fullsize, on alltime

	ref_rt						rt_Position, rt_Position_prev; // 64bit, e-space,	.xy - normal, .z - depth, .w - hemi\mtl
	ref_rt						rt_Depth_1;
	ref_rt						rt_Color; // 64/32bit,fat	(r,g,b,specular-gloss)	(or decompressed MET-8-8-8-8)
	
	ref_rt						rt_Accumulator;		// 64bit (r,g,b,specular) or 32bit (r, g, b)
	ref_rt						rt_Accumulator_1;	// 32bit (r, g, b)

	ref_rt						rt_Generic_0; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	ref_rt						rt_Generic_1; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	
	//	Igor: for volumetric lights
	ref_rt						rt_Generic_2;		// 32bit		(r,g,b)				// post-process, intermidiate results, etc.

	ref_rt						rt_Bloom_1;			// 32bit, dim/4	(r,g,b,?)
	ref_rt						rt_Bloom_2;			// 32bit, dim/4	(r,g,b,?)
	ref_rt						rt_LUM_64;			// 64bit, 64x64,	log-average in all components
	ref_rt						rt_LUM_8;			// 64bit, 8x8,		log-average in all components

	ref_rt						rt_LUM_pool[CHWCaps::MAX_GPUS*2]	;	// 1xfp32,1x1,		exp-result -> scaler
	ref_texture					t_LUM_src		;	// source
	ref_texture					t_LUM_dest		;	// destination & usage for current frame

	// env
	ref_texture					t_envmap_0		;	// env-0
	ref_texture					t_envmap_1		;	// env-1

	// smap
	ref_rt						rt_smap_depth;	// 24(32) bit,	depth 
	ref_rt						rt_vsm_depth;	// 32.32 bit,	depth
	ref_rt						rt_vsm_depthms;	// 32.32 bit,	depth
	ref_rt						rt_smap_depth_minmax;	//	is used for min/max sm

	//	Igor: for async screenshots
	ID3DTexture2D*			t_ss_async;				//32bit		(r,g,b,a) is situated in the system memory

	// Textures
	ID3DTexture3D*			t_material_surf;
	ref_texture					t_material;

	ID3DTexture2D*			t_noise_surf	[TEX_jitter_count];
	ref_texture					t_noise				[TEX_jitter_count];
	ID3DTexture2D*			t_noise_surf_mipped;
	ref_texture					t_noise_mipped;
private:

	ref_geom					g_simple_quad;

	// OCCq
	ref_shader					s_occq;

	// Accum
	ref_shader					s_accum_mask	;
	ref_shader					s_accum_direct	;
	ref_shader					s_accum_point	;
	ref_shader					s_accum_spot	;
	ref_shader					s_accum_reflected;
	ref_shader					s_accum_volume;


	// SSAO
    ref_shader					s_ssao;

    ref_shader					s_sunshafts;

	// Antialiasing
	ref_shader                  s_antialiasing;
	ref_shader                  s_taa;

	ref_shader                  s_rsm;

	ref_shader                  s_update_prev_zbuf;

	ref_shader					s_sspr;

	ref_shader					s_msaa_mark_edges;

	//	generate min/max
	ref_shader					s_create_minmax_sm;

	//	DX11 Rain
	ref_shader					s_rain;

	ref_shader					s_ssaa;

	ref_geom						g_accum_point	;
	ref_geom						g_accum_spot	;
	ref_geom						g_accum_omnipart;
	ref_geom						g_accum_volumetric;

	ID3DBuffer*				g_accum_point_vb;
	ID3DBuffer*				g_accum_point_ib;

	ID3DBuffer*				g_accum_omnip_vb;
	ID3DBuffer*				g_accum_omnip_ib;

	ID3DBuffer*				g_accum_spot_vb	;
	ID3DBuffer*				g_accum_spot_ib	;

	ID3DBuffer*				g_accum_volumetric_vb;
	ID3DBuffer*				g_accum_volumetric_ib;

	// Bloom
	ref_geom				g_bloom_build;
	ref_geom				g_bloom_filter;
	ref_shader				s_bloom;
	float					f_bloom_factor;

	// Luminance
	ref_shader				s_luminance;
	float					f_luminance_adapt;

	// Combine
	ref_geom					g_combine;
	ref_geom					g_combine_VP;		// xy=p,zw=tc
	ref_geom					g_combine_2UV;
	ref_geom					g_combine_cuboid;
	ref_geom					g_aa_blur;
	ref_geom					g_aa_AA;
	ref_shader					s_combine_dbg_1;
	ref_shader					s_combine_dbg_Accumulator;
	ref_shader					s_combine;
	//ref_shader				s_combine_volumetric;
public:
	ref_shader					s_postprocess, s_postprocess_ssaa;
	ref_geom					g_postprocess;
	ref_shader					s_menu, s_menu_ssaa;
	ref_geom					g_menu;
private:
	float						im_noise_time;
	u32							im_noise_shift_w;
	u32							im_noise_shift_h;
	
	float						param_blur;
	float						param_gray;
	float						param_duality_h;
	float						param_duality_v;
	float						param_noise;
	float						param_noise_scale;
	float						param_noise_fps;
	u32							param_color_base;
	u32							param_color_gray;
	Fvector						param_color_add;

	//	Color mapping
	float						param_color_map_influence;
	float						param_color_map_interpolate;
	ColorMapManager				color_map_manager;

public:
								CRenderTarget			();
								~CRenderTarget			();

	void						CRenderTargetDefferedCreate();
	void						CRenderTargetDefferedDelete();

	// SSAA
	void						SSAA_create		();
	void						enable_SSAA		();
	void						disable_SSAA	();
	IC const SSAA_params&		get_SSAA_params	()	{ return SSAA; }
	void						phase_amd_cas();
	void						phase_amd_cas_port();
	void						phase_amd_fsr_port();

	void						build_textures();
	void						destroy_textures();

	void						accum_point_geom_create	();
	void						accum_point_geom_destroy();
	void						accum_omnip_geom_create	();
	void						accum_omnip_geom_destroy();
	void						accum_spot_geom_create	();
	void						accum_spot_geom_destroy	();
	//	Igor: used for volumetric lights
	void						accum_volumetric_geom_create();
	void						accum_volumetric_geom_destroy();


	void						u_stencil_optimize		(eStencilOptimizeMode eSOM = SO_Light);


	void						u_compute_texgen_screen	(Fmatrix&	dest);
	void						u_compute_texgen_jitter	(Fmatrix&	dest);
	void						u_setrt					(u32 W, u32 H, ID3DRenderTargetView* _1, ID3DRenderTargetView* _2 = NULL, ID3DRenderTargetView* _3 = NULL, ID3DRenderTargetView* _4 = NULL);
	void						u_setrt					(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL);
	void						u_setzb					(ID3DDepthStencilView* zb);
	void						u_setzb					(const ref_rt& _1);
	void						u_calc_tc_noise			(Fvector2& p0, Fvector2& p1);
	void						u_calc_tc_duality_ss	(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);
	BOOL						u_need_PP				();
	bool						u_need_CM				();
	BOOL						u_DBT_enable			(float zMin, float zMax);
	void						u_DBT_disable			();

	//void						prepare_sq_vertex		(u32& bias, ref_geom& geom); ! NOT SAFE
	void						prepare_sq_vertex		(float w, float h, u32& bias, ref_geom& geom);
	// rt is only needed to determine its size
	void						prepare_sq_vertex		(const ref_rt& rt, u32& bias, ref_geom& geom);

	void						phase_scene_prepare		();
	void						phase_scene_begin		();
	void						phase_scene_end			();
	
	void						phase_occq				();
	void						phase_wallmarks			();
	void						phase_smap_direct		(light* L);
	void						phase_smap_rain			(light* L);
	void						phase_smap_direct_tsh	(light* L);
	void						phase_smap_spot_clear	();
	void						phase_smap_spot			(light* L);
	void						phase_smap_spot_tsh		(light* L);
	void						phase_accumulator		();

	BOOL						enable_scissor			(light* L);		// true if intersects near plane
	void						enable_dbt_bounds		(light* L);
	
	void						draw_volume				(light* L);
	void						accum_direct_cascade	(u32	sub_phase, float radius_n, Fmatrix& xform, Fmatrix& xform_prev, float fBias );
	void						accum_direct_vsm		(sun::cascade shadow);
	void						accum_direct_volumetric	(const u32 Offset, const Fmatrix& mShadow, const Fvector L_clr);
	void						accum_direct_reflective	(const u32 Offset, const Fmatrix& mShadow, const Fmatrix& mShadowP, const Fvector L_clr, const float cascede_scale);
	void						accum_point				(light* L);
	void						accum_spot				(light* L);
	void						accum_reflected			(light* L);
	void						accum_volumetric		(light* L);//	Igor: for volumetric lights
	void						phase_bloom				();
	void						phase_luminance			();

	void						phase_combine				();
	void						phase_combine_color			();
	void						phase_pp					();


	void						phase_combine_volumetric(Fvector4& sun_direction, Fvector4& sun_color);
	void						phase_vol_accumulator();

	// RSM
	void						accum_spot_reflective(light* L);
	void						phase_rsm_accumulator();
	void						phase_rsm_filter();
	bool						need_to_render_sun_il();
	bool						need_to_render_spot_il(Fcolor color);

	//void						shadow_direct			(light* L, u32 dls_phase);

	//	Generates min/max sm
	void						create_minmax_SM();
	bool						use_minmax_sm_this_frame();

	void						phase_rain();
	void						draw_rain(light& RainSetup);

	void						phase_sunshafts_screen(Fvector4& sun_direction, Fvector4& sun_color);
	bool						need_to_render_sunshafts(r__sunshafts_mode_values type);

	void						TXAA_rt_create(u32 s, u32 w, u32 h);

	// GFSDK
#ifdef __GFSDK_DX11__
	void						phase_hbao_plus();
	void						resolve_txaa(void);
	void						motion_txaa(void);
	void						feedback_txaa(void);
#endif

	// SSAO
	void						phase_ssao();
	void						phase_ssao_path_tracing();

	void						msaa_mark_edges();
	void						resolve_msaa(void);
	void						resolve_ssaa(void);
	void						resolve_fxaa(void);

	void						phase_FXAA				();
	void						phase_TAA				();
	void						phase_TAA_V2			();
	void						phase_MLAA				();

	void						phase_sspr				();

	virtual void				set_blur				(float	f)		{ param_blur=f;						}
	virtual void				set_gray				(float	f)		{ param_gray=f;						}
	virtual void				set_duality_h			(float	f)		{ param_duality_h=_abs(f);			}
	virtual void				set_duality_v			(float	f)		{ param_duality_v=_abs(f);			}
	virtual void				set_noise				(float	f)		{ param_noise=f;					}
	virtual void				set_noise_scale			(float	f)		{ param_noise_scale=f;				}
	virtual void				set_noise_fps			(float	f)		{ param_noise_fps=_abs(f)+EPS_S;	}
	virtual void				set_color_base			(u32	f)		{ param_color_base=f;				}
	virtual void				set_color_gray			(u32	f)		{ param_color_gray=f;				}
	virtual void				set_color_add			(const Fvector	&f)		{ param_color_add=f;		}
	
	virtual u32					get_width				()				{ return dwWidth;					}
	virtual u32					get_height				()				{ return dwHeight;					}

	virtual void				set_cm_imfluence	(float	f)		{ param_color_map_influence = f;							}
	virtual void				set_cm_interpolate	(float	f)		{ param_color_map_interpolate = f;							}
	virtual void				set_cm_textures		(const shared_str &tex0, const shared_str &tex1) {color_map_manager.SetTextures(tex0, tex1);}

	//	Need to reset stencil only when marker overflows.
	//	Don't clear when render for the first time
	void						reset_light_marker( bool bResetStencil = false);
	void						increment_light_marker();

	void						DoAsyncScreenshot		();

#ifdef DEBUG
	IC void						dbg_addline				(Fvector& P0, Fvector& P1, u32 c)					{
		dbg_lines.push_back		(dbg_line_t());
		dbg_lines.back().P0		= P0;
		dbg_lines.back().P1		= P1;
		dbg_lines.back().color	= c;
	}
	IC void						dbg_addplane			(Fplane& P0,  u32 c)								{
		dbg_planes.push_back(P0);
	}
#else
	IC void						dbg_addline				(Fvector& P0, Fvector& P1, u32 c)					{}
	IC void						dbg_addplane			(Fplane& P0,  u32 c)								{}
#endif
};