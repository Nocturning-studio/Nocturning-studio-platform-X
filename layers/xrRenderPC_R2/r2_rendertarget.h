#pragma once

class light;

#define DU_SPHERE_NUMVERTEX 92
#define DU_SPHERE_NUMFACES 180
#define DU_CONE_NUMVERTEX 18
#define DU_CONE_NUMFACES 32

class CRenderTarget : public IRender_Target
{
  private:
	u32 dwWidth;
	u32 dwHeight;
	u32 dwAccumulatorClearMark;

  public:
	u32 dwLightMarkerID;

	IBlender* b_occq;
	IBlender* b_accum_mask;
	IBlender* b_accum_direct_cascade;
	IBlender* b_accum_point;
	IBlender* b_accum_spot;
	IBlender* b_accum_reflected;
	IBlender* b_bloom;
	IBlender* b_distortion;
	IBlender* b_ambient_occlusion;
	IBlender* b_autoexposure;
	IBlender* b_combine;
	IBlender* b_contrast_adaptive_sharpening;
	IBlender* b_antialiasing;
	IBlender* b_tonemapping;
	IBlender* b_dof;
	IBlender* b_barrel_blur;
	IBlender* b_motion_blur;
	IBlender* b_sepia;
	IBlender* b_chromatic_abberation;
	IBlender* b_sharpen;
	IBlender* b_vignette;
	IBlender* b_frame_overlay;

#ifdef DEBUG
	struct dbg_line_t
	{
		Fvector P0, P1;
		u32 color;
	};
	xr_vector<std::pair<Fsphere, Fcolor>> dbg_spheres;
	xr_vector<dbg_line_t> dbg_lines;
	xr_vector<Fplane> dbg_planes;
#endif

	// Geometry Buffer
	ref_rt rt_GBuffer_Position;
	ref_rt rt_GBuffer_Normal;
	ref_rt rt_GBuffer_Albedo;

	// Accumulation Buffer
	ref_rt rt_Diffuse_Accumulator;
	ref_rt rt_Specular_Accumulator;

	ref_rt rt_Generic_0;		// 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	//	Igor: for volumetric lights
	ref_rt rt_Generic_2; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.

	ref_rt rt_Distortion_Mask;

	ref_rt rt_Bloom_1;	 // 32bit, dim/4	(r,g,b,?)
	ref_rt rt_Bloom_2;	 // 32bit, dim/4	(r,g,b,?)
	ref_rt rt_LUM_64;	 // 64bit, 64x64,	log-average in all components
	ref_rt rt_LUM_8;	 // 64bit, 8x8,		log-average in all components

	ref_rt rt_LUM_pool[4];	// 1xfp32,1x1,		exp-result -> scaler
	ref_texture t_LUM_src;	// source
	ref_texture t_LUM_dest; // destination & usage for current frame

	// ao
	ref_rt rt_ao_base;
	ref_rt rt_ao_blurred1;
	ref_rt rt_ao_blurred2;
	ref_rt rt_ao;

	// env
	ref_texture t_envmap_0; // env-0
	ref_texture t_envmap_1; // env-1

	ref_rt rt_Motion_Blur_Saved_Frame;

	// smap
	ref_rt rt_smap_surf;		   // 32bit,		color
	ref_rt rt_smap_depth;		   // 24(32) bit,	depth
	IDirect3DSurface9* rt_smap_ZB; //

	// Textures
	IDirect3DVolumeTexture9* t_material_surf;
	ref_texture t_material;

	IDirect3DTexture9* t_noise_surf[TEX_jitter_count];
	ref_texture t_noise[TEX_jitter_count];

  private:
	// OCCq
	ref_shader s_occq;

	// Accum
	ref_shader s_accum_mask;
	ref_shader s_accum_direct_cascade;
	ref_shader s_accum_direct_volumetric_cascade;
	ref_shader s_accum_point;
	ref_shader s_accum_spot;
	ref_shader s_accum_reflected;

	ref_geom g_accum_point;
	ref_geom g_accum_spot;
	ref_geom g_accum_omnipart;

	IDirect3DVertexBuffer9* g_accum_point_vb;
	IDirect3DIndexBuffer9* g_accum_point_ib;

	IDirect3DVertexBuffer9* g_accum_omnip_vb;
	IDirect3DIndexBuffer9* g_accum_omnip_ib;

	IDirect3DVertexBuffer9* g_accum_spot_vb;
	IDirect3DIndexBuffer9* g_accum_spot_ib;

	// Bloom
	ref_geom g_bloom_build;
	ref_geom g_bloom_filter;
	ref_shader s_bloom_dbg_1;
	ref_shader s_bloom_dbg_2;
	ref_shader s_bloom;

	// AO
	ref_shader s_ambient_occlusion;

	// AA
	ref_shader s_aa;

	// Luminance
	ref_shader s_autoexposure;
	float f_autoexposure_adapt;

	// Combine
	ref_geom g_combine;
	ref_geom g_combine_VP; // xy=p,zw=tc
	ref_geom g_combine_2UV;
	ref_geom g_combine_cuboid;
	ref_geom g_aa_blur;
	ref_geom g_aa_AA;

	ref_shader s_combine_dbg_0;
	ref_shader s_combine_dbg_1;
	ref_shader s_combine;
	ref_shader s_combine_volumetric;
	ref_shader s_contrast_adaptive_sharpening;
	ref_shader s_antialiasing;
	ref_shader s_barrel_blur;
	ref_shader s_dof;
	ref_shader s_distortion;
	ref_shader s_motion_blur;
	ref_shader s_chromatic_abberation;
	ref_shader s_sepia;
	ref_shader s_sharpen;
	ref_shader s_vignette;
	ref_shader s_frame_overlay;
	ref_shader s_tonemapping;
  public:
	ref_shader s_postprocess;
	ref_geom g_postprocess;
	ref_shader s_menu;
	ref_geom g_menu;

  private:
	float im_noise_time;
	u32 im_noise_shift_w;
	u32 im_noise_shift_h;

	float param_blur;
	float param_gray;
	float param_duality_h;
	float param_duality_v;
	float param_noise;
	float param_noise_scale;
	float param_noise_fps;
	u32 param_color_base;
	u32 param_color_gray;
	u32 param_color_add;

	//	Igor: used for volumetric lights
	bool m_bHasActiveVolumetric;

  public:
	CRenderTarget();
	~CRenderTarget();
	void accum_point_geom_create();
	void accum_point_geom_destroy();
	void accum_omnip_geom_create();
	void accum_omnip_geom_destroy();
	void accum_spot_geom_create();
	void accum_spot_geom_destroy();

	void u_stencil_optimize(BOOL common_stencil = TRUE);
	void u_compute_texgen_screen(Fmatrix& dest);
	void u_compute_texgen_jitter(Fmatrix& dest);
	void u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, IDirect3DSurface9* zb);
	void u_setrt(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2, IDirect3DSurface9* _3,
				 IDirect3DSurface9* zb);
	void u_calc_tc_noise(Fvector2& p0, Fvector2& p1);
	void u_calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);
	BOOL u_need_PP();
	BOOL u_DBT_enable(float zMin, float zMax);
	void u_DBT_disable();

	void enable_anisotropy_filtering();
	void disable_anisotropy_filtering();
	void clear_gbuffer();
	void create_gbuffer();
	void phase_occq();
	void phase_wallmarks();
	void phase_smap_direct(light* L, u32 sub_phase);
	void phase_smap_direct_tsh(light* L, u32 sub_phase);
	void phase_smap_spot_clear();
	void phase_smap_spot(light* L);
	void phase_smap_spot_tsh(light* L);
	void phase_accumulator();
	void phase_vol_accumulator();

	bool need_to_render_sunshafts();

	BOOL enable_scissor(light* L); // true if intersects near plane
	void enable_dbt_bounds(light* L);

	void draw_volume(light* L);
	void accum_direct_cascade(u32 sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias);
	void accum_direct_volumetric(u32 sub_phase, const u32 Offset, const Fmatrix& mShadow);
	void accum_point(light* L);
	void accum_spot(light* L);

	void accum_reflected(light* L);

	void phase_bloom();

	void phase_create_ao();
	void phase_vertical_filter();
	void phase_horizontal_filter();
	void phase_upscale();
	void phase_filtering();
	void phase_ao();

	void phase_autoexposure();
	void phase_tonemapping();

	void phase_combine();
	void phase_combine_volumetric();

	void phase_contrast_adaptive_sharpening();

	void photo_grid();
	void cinema_borders();
	void watermark();
	void draw_overlays();

	void antialiasing_phase_rgaa();
	void antialiasing_phase_fxaa();
	void antialiasing_phase_dlaa();
	void select_antialiasing_type();
	int select_antialiasing_iterations_count();
	void phase_antialiasing();

	void phase_create_distortion_mask();
	void phase_distortion();

	void depth_of_field_pass_poisson_filter();
	void depth_of_field_pass_vertical();
	void depth_of_field_pass_horizontal();
	void calculate_depth_of_field();
	void phase_depth_of_field();

	void calculate_barrel_blur();
	void phase_barrel_blur();

	void motion_blur_phase_save_frame();
	void motion_blur_phase_combine();
	void phase_motion_blur();

	void phase_chromatic_abberation();

	void phase_sepia();

	void phase_sharpen();

	void phase_vignette();

	void phase_pp();

	virtual void set_blur(float f)
	{
		param_blur = f;
	}
	virtual void set_gray(float f)
	{
		param_gray = f;
	}
	virtual void set_duality_h(float f)
	{
		param_duality_h = _abs(f);
	}
	virtual void set_duality_v(float f)
	{
		param_duality_v = _abs(f);
	}
	virtual void set_noise(float f)
	{
		param_noise = f;
	}
	virtual void set_noise_scale(float f)
	{
		param_noise_scale = f;
	}
	virtual void set_noise_fps(float f)
	{
		param_noise_fps = _abs(f) + EPS_S;
	}
	virtual void set_color_base(u32 f)
	{
		param_color_base = f;
	}
	virtual void set_color_gray(u32 f)
	{
		param_color_gray = f;
	}
	virtual void set_color_add(u32 f)
	{
		param_color_add = f;
	}

	virtual u32 get_width()
	{
		return dwWidth;
	}
	virtual u32 get_height()
	{
		return dwHeight;
	}

	//	Need to reset stencil only when marker overflows.
	//	Don't clear when render for the first time
	void reset_light_marker(bool bResetStencil = false);
	void increment_light_marker();

#ifdef DEBUG
	IC void dbg_addline(Fvector& P0, Fvector& P1, u32 c)
	{
		dbg_lines.push_back(dbg_line_t());
		dbg_lines.back().P0 = P0;
		dbg_lines.back().P1 = P1;
		dbg_lines.back().color = c;
	}
	IC void dbg_addplane(Fplane& P0, u32 c)
	{
		dbg_planes.push_back(P0);
	}
#else
	IC void dbg_addline(Fvector& P0, Fvector& P1, u32 c)
	{
	}
	IC void dbg_addplane(Fplane& P0, u32 c)
	{
	}
#endif
};
