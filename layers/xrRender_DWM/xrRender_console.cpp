#include	"stdafx.h"

#pragma		hdrstop

#pragma warning(disable:4482)

#include	"xrRender_console.h"
#include	"dxRenderDeviceRender.h"

#include	"../xrEngine/xr_ioconsole.h"
#include	"../xrEngine/xr_ioc_cmd.h"

Flags32 r__opt_flags = 
{ 
	r__opt_flags_values::R__USE_BUMP			|
	r__opt_flags_values::R__USE_SPLIT_SCENE		|
	r__opt_flags_values::R__USE_ADVANCED_MODE	|
	r__opt_flags_values::R__USE_NVDBT			|
	r__opt_flags_values::R__NEED_NVDSTENCIL		|
	r__opt_flags_values::R__USE_DYN_SHADOWS		|
	r__opt_flags_values::R__USE_TEX_STAGING		|
	r__opt_flags_values::R__USE_TONEMAP			|
	0
};

Flags32 r__adv_opt_flags =
{ 
	r__adv_opt_flags_values::R__REP_SSR_TO_PLANAR	|
	r__adv_opt_flags_values::R__USE_WATER_DIST		|
	r__adv_opt_flags_values::R__USE_SSR				|
	r__adv_opt_flags_values::R__USE_PLANAR_DETAILS	|
	r__adv_opt_flags_values::R__USE_CSPECULAR		|
	r__adv_opt_flags_values::R__PT_DOWNSAMPLE		|
	//r__adv_opt_flags_values::R__FSR_16				|
	0
};

Flags32 r__dbg_opt_flags =
{ 
	0
};

BOOL opt(r__opt_flags_values	 flag) { return r__opt_flags	.test(flag); }
BOOL opt(r__adv_opt_flags_values flag) { return r__adv_opt_flags.test(flag); }
BOOL opt(r__dbg_opt_flags_values flag) { return r__dbg_opt_flags.test(flag); }

u32 psPreset = 2; // options standart preset
xr_token psPreset_token[] =
{
	{ "ui_mm_rspec_new_0",	0 },
	{ "ui_mm_rspec_new_1",	1 },
	{ "ui_mm_rspec_new_2",	2 },
	{ "ui_mm_rspec_new_3",	3 },
	{ "ui_mm_rspec_new_4",	4 },
	{ "ui_mm_rspec_new_5",	5 },
	{ 0,					0 }
};

Fvector4 r__free_vector = { 0, 0, 0, 0 };

Fvector4 r__sun_il_params_0 = { 5, 3, 50, 1 };
Fvector4 r__sun_il_params_1 = { 3.25, 0.02, 50, 0.015 };
Fvector4 r__sun_il_params_2 = { 200, 0.01, 0.5, 20 };
Fvector4 r__sun_il_params_3 = { 0.05, 0.075, 0.1, 0.93 };

float	r__wallmark_ttl			= 50.f;
float	r__wallmark_shift_pp	= 0.0001f;
float	r__wallmark_shift_v		= 0.0001f;

float	r__geometry_lod			= 0.75;

float	r__detail_density		= 0.3f;
float	r__detail_l_ambient		= 0.9f;
float	r__detail_l_aniso		= 0.25f;

float	r__ssa_glod_start		= 256.f;
float	r__ssa_glod_end			= 64.f;
float	r__ssa_lod_a			= 64.f;
float	r__ssa_lod_b			= 48.f;

float	r__tonemap_middlegray	= 1.f;		
float	r__tonemap_adaptation	= 1.f;		
float	r__tonemap_low_lum		= 0.0001f;	
float	r__tonemap_amount		= 0.7f;		
float	r__tonemap_overbright	= 0.7f;		

float	r__bloom_kernel_g		= 3.f;	
float	r__bloom_kernel_b		= 0.7f;	
float	r__bloom_speed			= 100.f;			
float	r__bloom_kernel_scale	= 0.7f;
float	r__bloom_threshold		= 0.1f;

float	r__ssa_discard			= 3.5f; // Not cmd command
float	r__ssa_dontsort			= 32.f; // Not cmd command
float	r__ssa_hzb_vs_tex		= 96.f; // Not cmd command

int		r__lsleep_frames		= 10;

int		r__tf_aniso				= 8;
float	r__tf_mipbias			= 0.0f;

float	r__cas_contrast				= 0.1f;
float	r__cas_sharpening			= 1.0f;

float	r__smap_quality				= 1.f;

float	r__zfill_depth				= 0.25f;

float	r__gloss					= 3.f;

//float	r__ssaa_contrast			= 0.2f;
//float	r__ssaa_sharpness			= 1.0f;

float	r__dbg_planar_h				= -7.2f;
float	r__planar_bias_n			= 0.05;
float	r__planar_bias_d			= 0.0003;

float	r__sun_near					= 20.f;
float	r__sun_depth_near_scale		= 1.f;
float	r__sun_depth_near_bias		= 0.00001f;

float	r__sun_far					= 100.f;
float	r__sun_depth_far_scale		= 1.f;
float	r__sun_depth_far_bias		= -0.00002f;

float	r__sun_lumscale				= 1.f;
float	r__sun_lumscale_hemi		= 1.f;
float	r__sun_lumscale_amb			= 1.f;

int		r__wait_sleep				= 1;

int		r__dhemi_count				= 5;
float	r__dhemi_sky_scale			= 0.08f;
float	r__dhemi_light_scale		= 0.2f;
float	r__dhemi_light_flow			= 0.1f;
float	r__dhemi_smooth				= 1.f;

float	r__ls_depth_scale			= 1.00001f;
float	r__ls_depth_bias			= -0.0003f;

float	r__slight_fade				= 0.5f;

Fvector3	r__dof			= { -1.25f, 1.4f, 600.f }; // .x - min, .y - focus, .z - max
float		r__dof_kernel	= 7.f;
float		r__dof_sky		= 250.f;

u32 r__taa_jitter_mode = taa_8x;
xr_token r__taa_jitter_mode_token[] =
{
	{ "opt_1x",		taa_1x },
	{ "opt_2x",		taa_2x },
	{ "opt_4x",		taa_4x },
	{ "opt_8x",		taa_8x },
	{ "opt_16x",	taa_16x },
	{ 0,				 0 }
};

u32 r__msaa_reflections = 1;
xr_token r__msaa_reflections_token[] =
{
	{ "opt_off",		1 },
	{ "opt_msaa2x",		2 },
	{ "opt_msaa4x",		4 },
	{ "opt_msaa8x",		8 },
	{ 0,				0 }
};

u32 r__ssaa = 0;
xr_token r__ssaa_token[] =
{
	{ "opt_off",			0			},
	{ "opt_ssaa_2x",		SSAA2X		},
	{ "opt_ssaa_4x",		SSAA4X		},
	{ "opt_ultra_quality",	FSR_SSAA169	},
	{ "opt_quality",		FSR_SSAA225	},
	{ "opt_balanced",		FSR_SSAA289	},
	{ "opt_performance",	FSR_SSAA400	},
	{ 0,					0			}
};

u32 r__aa = 0;
xr_token r__aa_token[] =
{
	{ "opt_off",		0			 },
	{ "opt_mlaa",		AA_MLAA		 },
	//{ "opt_fxaa",		AA_FXAA		 },
	{ "opt_msaa_fxaa",	AA_MSAA_FXAA },
	{ "opt_msaa2x",		AA_MSAA2S	 },
	{ "opt_msaa4x",		AA_MSAA4S	 },
	{ "opt_msaa8x",		AA_MSAA8S	 },
	{ "opt_taa",		AA_TAA		 },
	//{ "opt_taa_new",	AA_TAA_V2	 },
#ifdef __GFSDK_DX11__
	{ "opt_txaa1x",		AA_TXAA		 },
	{ "opt_txaa2x",		AA_TXAA2S	 },
	{ "opt_txaa4x",		AA_TXAA4S	 },
#endif
	{ 0,				0			 }
};

u32 r__smap_filter = 0;
xr_token r__smap_filter_token[] =
{
	{ "opt_sh_hw2x2",			0 },
	{ "opt_sh_hw2x2_jitter",	1 },
	{ "opt_sh_hw2x2_pcf7x7",	2 },
	{ "opt_sh_hw2x2_pcss",		3 },
	//{ "opt_variance_mapping",	4 },
	{ 0,						0 }
};

u32 r__smap_size = 512;
u32 r__rain_smap_size = 1024;
xr_token r__smap_size_token[] =
{
	//{ "512",			512 },
	{ "1024",			1024 },
	{ "2048",			2048 },
	//{ "3072",			3072 },
	{ "4096",			4096 },
	{ 0,				0 }
};

u32 r__ssao_mode = 0;
xr_token r__ssao_mode_token[] =
{
	{ "opt_off",		0				},
	{ "opt_ssao",		SSAO_SSAO		},
	{ "opt_ssao_pt",	SSAO_PATH_TRACING		},
#ifdef __GFSDK_DX11__
	{ "opt_hbao_plus",	SSAO_HBAO_PLUS	},
#endif
	{ 0,				0				}
};

u32 r__parallax_mode = 1;
xr_token r__parallax_mode_token[] =
{
	{ "opt_par_mode_off",			0 },
	{ "opt_par_mode_simple",		1 },
	{ "opt_par_mode_steep",			2 },
	{ "opt_par_mode_steep_binary",	3 },
	{ 0,								0 }
};

u32 r__sunshafts_mode = 0;
xr_token r__sunshafts_mode_token[] =
{
	{ "opt_off",			0 },
	{ "opt_screen_space",	SUNSHAFTS_SCREEN },
	{ "opt_volumetric",		SUNSHAFTS_VOLUME },
	{ 0,				0 }
};

#ifndef _EDITOR

#include "SamplerStateCache.h"

//-----------------------------------------------------------------------
class CCC_tf_Aniso		: public CCC_Integer
{
public:
	void	apply	()	{
		if (0==HW.pDevice)	return	;
		int	val = *value;	clamp(val,1,16);

		SSManager.SetMaxAnisotropy(val);

	}
	CCC_tf_Aniso(LPCSTR N, int*	v) : CCC_Integer(N, v, 1, 16)		{ };
	virtual void Execute	(LPCSTR args)
	{
		CCC_Integer::Execute	(args);
		apply					();
	}
	virtual void	Status	(TStatus& S)
	{	
		CCC_Integer::Status		(S);
		apply					();
	}
};
class CCC_tf_MipBias: public CCC_Float
{
public:
	void	apply	()	{
		if (0==HW.pDevice)	return	;

		SSManager.SetMipBias(*value);

	}

	CCC_tf_MipBias(LPCSTR N, float*	v) : CCC_Float(N, v, -0.5f, +0.5f)	{ };
	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute	(args);
		apply				();
	}
	virtual void	Status	(TStatus& S)
	{	
		CCC_Float::Status	(S);
		apply				();
	}
};
class CCC_Screenshot : public IConsole_Command
{
public:
	CCC_Screenshot(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		if (g_dedicated_server)
			return;

		string_path	name;	name[0]=0;
		sscanf		(args,"%s",	name);
		LPCSTR		image	= xr_strlen(name)?name:0;
		::Render->Screenshot(IRender_interface::SM_NORMAL,image);
	}
};

class CCC_RestoreQuadIBData : public IConsole_Command
{
public:
	CCC_RestoreQuadIBData(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		RCache.RestoreQuadIBData();
	}
};

class CCC_ModelPoolStat : public IConsole_Command
{
public:
	CCC_ModelPoolStat(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		RImplementation.Models->dump();
	}
};

//-----------------------------------------------------------------------
class	CCC_Preset		: public CCC_Token
{
public:
	CCC_Preset(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N,V,T)	{}	;

	virtual void	Execute	(LPCSTR args)	{
		CCC_Token::Execute	(args);
		string_path		_cfg;
		string_path		cmd;
		
		switch	(*value)	{
			case 0:		xr_strcpy(_cfg, "rspec_new_0.ltx");	break;
			case 1:		xr_strcpy(_cfg, "rspec_new_1.ltx");	break;
			case 2:		xr_strcpy(_cfg, "rspec_new_2.ltx");	break;
			case 3:		xr_strcpy(_cfg, "rspec_new_3.ltx");	break;
			case 4:		xr_strcpy(_cfg, "rspec_new_4.ltx");	break;
			case 5:		xr_strcpy(_cfg, "rspec_new_5.ltx");	break;
		}
		FS.update_path			(_cfg,"$game_config$",_cfg);
		strconcat				(sizeof(cmd),cmd,"cfg_load", " ", _cfg);
		Console->Execute		(cmd);
	}
};


class CCC_memory_stats : public IConsole_Command
{
protected	:

public		:

	CCC_memory_stats(LPCSTR N) :	IConsole_Command(N)	{ bEmptyArgsHandled = true; };

	virtual void	Execute	(LPCSTR args)
	{
		u32 m_base = 0;
		u32 c_base = 0;
		u32 m_lmaps = 0; 
		u32 c_lmaps = 0;

		dxRenderDeviceRender::Instance().ResourcesGetMemoryUsage( m_base, c_base, m_lmaps, c_lmaps );

		Msg		("memory usage  mb \t \t video    \t managed      \t system \n" );

		float vb_video		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_vertex][D3DPOOL_DEFAULT]/1024/1024;
		float vb_managed	= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_vertex][D3DPOOL_MANAGED]/1024/1024;
		float vb_system		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_vertex][D3DPOOL_SYSTEMMEM]/1024/1024;
		Msg		("vertex buffer      \t \t %f \t %f \t %f ",	vb_video, vb_managed, vb_system);

		float ib_video		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_index][D3DPOOL_DEFAULT]/1024/1024; 
		float ib_managed	= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_index][D3DPOOL_MANAGED]/1024/1024; 
		float ib_system		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_index][D3DPOOL_SYSTEMMEM]/1024/1024; 
		Msg		("index buffer      \t \t %f \t %f \t %f ",	ib_video, ib_managed, ib_system);
		
		float textures_managed = (float)(m_base+m_lmaps)/1024/1024;
		Msg		("textures          \t \t %f \t %f \t %f ",	0.f, textures_managed, 0.f);

		float rt_video		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_rtarget][D3DPOOL_DEFAULT]/1024/1024;
		float rt_managed	= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_rtarget][D3DPOOL_MANAGED]/1024/1024;
		float rt_system		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_rtarget][D3DPOOL_SYSTEMMEM]/1024/1024;
		Msg		("R-Targets         \t \t %f \t %f \t %f ",	rt_video, rt_managed, rt_system);									

		Msg		("\nTotal             \t \t %f \t %f \t %f ",	vb_video+ib_video+rt_video,
																textures_managed + vb_managed+ib_managed+rt_managed,
																vb_system+ib_system+rt_system);
	}

};


//#include "render_pixel_calculator.h"
class CCC_BuildSSA : public IConsole_Command
{
public:
	CCC_BuildSSA(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) 
	{

		//	TODO: DX11: Implement pixel calculator

	}
};

class CCC_DofFar : public CCC_Float
{
public:
	CCC_DofFar(LPCSTR N, float* V, float _min=0.0f, float _max=10000.0f) 
		: CCC_Float( N, V, _min, _max){}

	virtual void Execute(LPCSTR args) 
	{
		float v = float(atof(args));

		if (v< r__dof.y+0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value greater or equal to r__dof_focus+0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r__dof_focus");
		}
		else
		{
			CCC_Float::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(r__dof);
		}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void	Save	(IWriter *F)	{;}
};

class CCC_DofNear : public CCC_Float
{
public:
	CCC_DofNear(LPCSTR N, float* V, float _min=0.0f, float _max=10000.0f) 
		: CCC_Float( N, V, _min, _max){}

	virtual void Execute(LPCSTR args) 
	{
		float v = float(atof(args));

		if (v> r__dof.y-0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value less or equal to r__dof_focus-0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r__dof_focus");
		}
		else
		{
			CCC_Float::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(r__dof);
		}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void	Save	(IWriter *F)	{;}
};

class CCC_DofFocus : public CCC_Float
{
public:
	CCC_DofFocus(LPCSTR N, float* V, float _min=0.0f, float _max=10000.0f) 
		: CCC_Float( N, V, _min, _max){}

	virtual void Execute(LPCSTR args) 
	{
		float v = float(atof(args));

		if (v> r__dof.z-0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value less or equal to r__dof_far-0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r__dof_far");
		}
		else if (v< r__dof.x+0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value greater or equal to r__dof_far-0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r__dof_near");
		}
		else{
			CCC_Float::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(r__dof);
			}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void	Save	(IWriter *F)	{;}
};

class CCC_Dof : public CCC_Vector3
{
public:
	CCC_Dof(LPCSTR N, Fvector* V, const Fvector _min, const Fvector _max) : 
	  CCC_Vector3(N, V, _min, _max) {;}

	virtual void	Execute	(LPCSTR args)
	{
		Fvector v;
		if (3!=sscanf(args,"%f,%f,%f",&v.x,&v.y,&v.z))	
			InvalidSyntax(); 
		else if ( (v.x > v.y-0.1f) || (v.z < v.y+0.1f))
		{
			InvalidSyntax();
			Msg("x <= y - 0.1");
			Msg("y <= z - 0.1");
		}
		else
		{
			CCC_Vector3::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(r__dof);
		}
	}
	virtual void	Status	(TStatus& S)
	{	
		xr_sprintf	(S,"%f,%f,%f",value->x,value->y,value->z);
	}
	virtual void	Info	(TInfo& I)
	{	
		xr_sprintf(I,"vector3 in range [%f,%f,%f]-[%f,%f,%f]",min.x,min.y,min.z,max.x,max.y,max.z);
	}

};

class CCC_DumpResources : public IConsole_Command
{
public:
	CCC_DumpResources(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		RImplementation.Models->dump();
		dxRenderDeviceRender::Instance().Resources->Dump(false);
	}
};

//-----------------------------------------------------------------------
void xrRender_initconsole()
{
	CMD3(CCC_Preset, "_preset", &psPreset, psPreset_token);

	CMD4(CCC_Integer, "rs_skeleton_update", &psSkeletonUpdate, 2, 128);

#ifdef	DEBUG
	CMD1(CCC_DumpResources,		"dump_resources");
#endif	//	 DEBUG

	CMD4(CCC_Float, "r__dtex_range", &r__dtex_range, 5, 175);

	CMD1(CCC_Screenshot, "screenshot");

	//	Igor: just to test bug with rain/particles corruption
	CMD1(CCC_RestoreQuadIBData, "r_restore_quad_ib_data");

	// DEVX / DEVY / DEVZ / DEVW
	CMD4(CCC_Float, "reserved1", &r__free_vector.x, -100000, 100000);
	CMD4(CCC_Float, "reserved2", &r__free_vector.y, -100000, 100000);
	CMD4(CCC_Float, "reserved3", &r__free_vector.z, -100000, 100000);
	CMD4(CCC_Float, "reserved4", &r__free_vector.w, -100000, 100000);

#ifdef DEBUG
	CMD1(CCC_BuildSSA,	"build_ssa"				);
	CMD4(CCC_Integer,	"r__lsleep_frames",		&r__lsleep_frames,		4,		30 );
	CMD4(CCC_Float,		"r__ssa_glod_start",	&r__ssa_glod_start,		128,	512 );
	CMD4(CCC_Float,		"r__ssa_glod_end",		&r__ssa_glod_end,		16,		96 );
	CMD1(CCC_ModelPoolStat,"stat_models"		);
#endif // DEBUG

	CMD4(CCC_Float, "r__gi_samples",		&r__sun_il_params_0.x,	1,		50	);
	CMD4(CCC_Float, "r__gi_saturation",		&r__sun_il_params_0.y,	0,		4	);
	CMD4(CCC_Float, "r__gi_brightness",		&r__sun_il_params_0.z,	0,		100	);
	CMD4(CCC_Float, "r__gi_intensity",		&r__sun_il_params_0.w,	0,		2	);
	
	CMD4(CCC_Float, "r__gi_fade_power",		&r__sun_il_params_1.x,	0,		8	);
	CMD4(CCC_Float, "r__gi_fade_min",		&r__sun_il_params_1.y,	0,		0.1	);
	CMD4(CCC_Float, "r__gi_fade_max",		&r__sun_il_params_1.z,	1,		100	);
	CMD4(CCC_Float, "r__gi_size",			&r__sun_il_params_1.w,	0.0001,	1	);
																
	CMD4(CCC_Float, "r__gi_far_plane",		&r__sun_il_params_2.x,	50,		200);
	CMD4(CCC_Float, "r__gi_near_plane",		&r__sun_il_params_2.y,	0.0001,	VIEWPORT_NEAR);
	CMD4(CCC_Float, "r__gi_normal_bias",	&r__sun_il_params_2.z,	0,		1);
	CMD4(CCC_Float, "r__gi_jitter_size",	&r__sun_il_params_2.w,	1,		1024);

	CMD4(CCC_Float, "r__gi_spatial_filter_depth_threshold",		&r__sun_il_params_3.x,	0,		1);
	CMD4(CCC_Float, "r__gi_spatial_filter_normal_threshold",	&r__sun_il_params_3.y,	0,		1);
	CMD4(CCC_Float, "r__gi_temporal_filter_depth_threshold",	&r__sun_il_params_3.z,	0,		1);
	CMD4(CCC_Float, "r__gi_temporal_filter_expectation",		&r__sun_il_params_3.w,	0,		1);

	CMD4(CCC_Float,		"r__wallmark_ttl",		&r__wallmark_ttl,		0.1f,	5.f * 60.f);
	CMD4(CCC_Float,		"r__wallmark_shift_pp", &r__wallmark_shift_pp,	0.0f,	1.f);
	CMD4(CCC_Float,		"r__wallmark_shift_v",	&r__wallmark_shift_v,	0.0f,	1.f);

	CMD4(CCC_Float,		"r__geometry_lod",		&r__geometry_lod,		0.2f,	1.2f);

	CMD4(CCC_Float,		"r__detail_density",	&r__detail_density,		0.19f,	r__detail_density_min);
	CMD4(CCC_Float,		"r__detail_l_ambient",	&r__detail_l_ambient,	0.5f,	0.95f);
	CMD4(CCC_Float,		"r__detail_l_aniso",	&r__detail_l_aniso,		0.1f,	0.5f);

	CMD2(CCC_tf_Aniso,		"r__tf_aniso",		&r__tf_aniso);
	CMD2(CCC_tf_MipBias,	"r__tf_mipbias",	&r__tf_mipbias);

	CMD3(CCC_Mask,		"r__use_split_scene",	&r__opt_flags,		r__opt_flags_values::R__USE_SPLIT_SCENE);
	CMD3(CCC_Mask,		"r__use_bump",			&r__opt_flags,		r__opt_flags_values::R__USE_BUMP);
	CMD3(CCC_Mask,		"r__use_advanced_mode",	&r__opt_flags,		r__opt_flags_values::R__USE_ADVANCED_MODE);
	CMD3(CCC_Mask,		"r__use_sun_zcul",		&r__opt_flags,		r__opt_flags_values::R__USE_SUN_ZCUL);
	CMD3(CCC_Mask,		"r__use_nvdbt",			&r__opt_flags,		r__opt_flags_values::R__USE_NVDBT);
	CMD3(CCC_Mask,		"r__need_nvstencil",	&r__opt_flags,		r__opt_flags_values::R__NEED_NVDSTENCIL);
	CMD3(CCC_Mask,		"r__use_dyn_shadows",	&r__opt_flags,		r__opt_flags_values::R__USE_DYN_SHADOWS);
	CMD3(CCC_Mask,		"r__use_trans_shadows",	&r__opt_flags,		r__opt_flags_values::R__USE_TRANS_SHADOWS);
	CMD3(CCC_Mask,		"r__use_tex_staging",	&r__opt_flags,		r__opt_flags_values::R__USE_TEX_STAGING);
	CMD3(CCC_Mask,		"r__use_sun_vsm",		&r__opt_flags,		r__opt_flags_values::R__USE_VSM);
	CMD3(CCC_Mask,		"r__use_dynamic_hud",	&r__opt_flags,		r__opt_flags_values::R__USE_DYNAMIC_HUD);


	CMD3(CCC_Mask,		"r__use_wet_surfaces",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_WET_SURFACES);
	CMD3(CCC_Mask,		"r__use_ssr",			&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_SSR);
	CMD3(CCC_Mask,		"r__use_vlight",		&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_VLIGHT);
	CMD3(CCC_Mask,		"r__use_water_distort",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_WATER_DIST);
	CMD3(CCC_Mask,		"r__use_dof",			&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_DOF);
	CMD3(CCC_Mask,		"r__use_soft_water",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_SOFT_WATER);
	CMD3(CCC_Mask,		"r__use_soft_particles",&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_SOFT_PARTICLES);
	CMD3(CCC_Mask,		"r__use_sun_details",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_SUN_DETAILS);
	CMD3(CCC_Mask,		"r__use_long_shadows",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_LONG_SHADOWS);
	CMD3(CCC_Mask,		"r__use_hud_shadows",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_HUD_SHADOWS);
	CMD3(CCC_Mask,		"r__use_sun_gi",		&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_SUN_IL);
	CMD3(CCC_Mask,		"r__use_spot_gi",		&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_SPOT_IL);
	CMD3(CCC_Mask,		"r__use_planar",		&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_PLANAR);
	CMD3(CCC_Mask,		"r__use_planar_details",&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_PLANAR_DETAILS);
	CMD3(CCC_Mask,		"r__use_cas",			&r__adv_opt_flags,  r__adv_opt_flags_values::R__USE_CAS);
	CMD3(CCC_Mask,		"r__use_cspec",			&r__adv_opt_flags,  r__adv_opt_flags_values::R__USE_CSPECULAR);
	CMD3(CCC_Mask,		"r__pt_downsample",		&r__adv_opt_flags,  r__adv_opt_flags_values::R__PT_DOWNSAMPLE);
	CMD3(CCC_Mask,		"r__fsr_16",			&r__adv_opt_flags,  r__adv_opt_flags_values::R__FSR_16);


	CMD3(CCC_Mask,		"r__need_tesselation",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__NEED_TESSELATION);
	CMD3(CCC_Mask,		"r__rep_ssr_to_planar",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__REP_SSR_TO_PLANAR);
	CMD3(CCC_Mask,		"r__use_vsmoke",		&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_VSMOKE);
	//CMD3(CCC_Mask,		"r__need_minmax_sm",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__NEED_MINMAX_SM);
	
	//CMD3(CCC_Mask,		"r__use_ssao_pt_blur",	&r__adv_opt_flags,	r__adv_opt_flags_values::R__USE_SSAO_PT_BLUR);
	
	CMD3(CCC_Mask,		"r__dbg_tesselation",	&r__dbg_opt_flags,	r__dbg_opt_flags_values::R__DBG_DRAW_WIREFRAME);
	CMD3(CCC_Mask,		"r__dbg_flat_water",	&r__dbg_opt_flags,	r__dbg_opt_flags_values::R__DBG_FLAT_WATER);
	CMD3(CCC_Mask,		"r__dbg_noalbedo",		&r__dbg_opt_flags,	r__dbg_opt_flags_values::R__DBG_NOALBEDO);

	CMD3(CCC_Mask,		"r__dbg_msaa_nv_8_and_9_series_hack",		&r__dbg_opt_flags,	r__dbg_opt_flags_values::R__DBG_MSAA_HACK);
	
	CMD3(CCC_Token,		"r__msaa_reflections",	&r__msaa_reflections,	r__msaa_reflections_token);

	CMD3(CCC_Token,		"r__taa_jitter_mode",	&r__taa_jitter_mode,	r__taa_jitter_mode_token);
	
	CMD3(CCC_Token,		"r__ssaa",				&r__ssaa,				r__ssaa_token);

	CMD3(CCC_Token,		"r__aa",				&r__aa,					r__aa_token);
	CMD3(CCC_Token,		"r__smap_filter",		&r__smap_filter,		r__smap_filter_token);
	CMD3(CCC_Token,		"r__smap_size",			&r__smap_size,			r__smap_size_token);
	CMD3(CCC_Token,		"r__rain_smap_size",	&r__rain_smap_size,		r__smap_size_token);
	CMD3(CCC_Token,		"r__ssao_mode",			&r__ssao_mode,			r__ssao_mode_token);
	CMD3(CCC_Token,		"r__parallax_mode",		&r__parallax_mode,		r__parallax_mode_token);
	CMD3(CCC_Token,		"r__sunshafts_mode",	&r__sunshafts_mode,		r__sunshafts_mode_token);

	CMD4(CCC_Float,		"r__ssa_lod_a",			&r__ssa_lod_a,		16,		96);
	CMD4(CCC_Float,		"r__ssa_lod_b",			&r__ssa_lod_b,		32,		64);

	CMD3(CCC_Mask,		"r__use_tonemap",			&r__opt_flags,				r__opt_flags_values::R__USE_TONEMAP);
	CMD4(CCC_Float,		"r__tonemap_middlegray",	&r__tonemap_middlegray,		0.0f,			2.0f);
	CMD4(CCC_Float,		"r__tonemap_adaptation",	&r__tonemap_adaptation,		0.01f,			10.0f);
	CMD4(CCC_Float,		"r__tonemap_lowlum",		&r__tonemap_low_lum,		0.0001f,		1.0f);
	CMD4(CCC_Float,		"r__tonemap_amount",		&r__tonemap_amount,			0.0000f,		1.0f);

	CMD4(CCC_Float,		"r__bloom_kernel_scale",	&r__bloom_kernel_scale,		0.5f,			2.f);
	CMD4(CCC_Float,		"r__bloom_kernel_g",		&r__bloom_kernel_g,			1.f,			7.f);
	CMD4(CCC_Float,		"r__bloom_kernel_b",		&r__bloom_kernel_b,			0.01f,			1.f);
	CMD4(CCC_Float,		"r__bloom_threshold",		&r__bloom_threshold,		0.f,			1.f);
	CMD4(CCC_Float,		"r__bloom_speed",			&r__bloom_speed,			0.f,			100.f);

	CMD4(CCC_Float,		"r__cas_contrast",			&r__cas_contrast,	0.f,	1.f);
	CMD4(CCC_Float,		"r__cas_sharpening",		&r__cas_sharpening,	0.f,	1.f);

	CMD4(CCC_Float,		"r__smap_quality",		&r__smap_quality,	0.5f,	1.f);

	CMD4(CCC_Float,		"r__zfill_depth",		&r__zfill_depth,	0.001f,	0.75f);

	CMD4(CCC_Float,		"r__gloss",				&r__gloss,			0.0f,	10.f);
	
	//CMD4(CCC_Float,		"r__ssaa_contrast",		&r__ssaa_contrast,	0.0f,	1.0f);
	//CMD4(CCC_Float,		"r__ssaa_sharpness",	&r__ssaa_sharpness,	0.0f,	1.0f);
	
	CMD4(CCC_Float,		"r__dbg_planar_h",		&r__dbg_planar_h,	-1000.0f,	1000.f);
	CMD4(CCC_Float,		"r__planar_bias_n",		&r__planar_bias_n,	-0.1f,		0.1f);
	CMD4(CCC_Float,		"r__planar_bias_d",		&r__planar_bias_d,	-0.1f,		0.1f);

	CMD4(CCC_Float,		"r__sun_near",			&r__sun_near,		1.f,	50.f);
	CMD4(CCC_Float,		"r__sun_far",			&r__sun_far,		51.f,	180.f);
	
	CMD4(CCC_Float,		"r__sun_depth_far_scale",		&r__sun_depth_far_scale,	 0.5f,	1.5f);
	CMD4(CCC_Float,		"r__sun_depth_near_scale",		&r__sun_depth_near_scale,	 0.5f,	1.5f);
	

	CMD4(CCC_Float,		"r__sun_depth_far_bias",		&r__sun_depth_far_bias,		-0.5f,	0.5f);
	CMD4(CCC_Float,		"r__sun_depth_near_bias",		&r__sun_depth_near_bias,	-0.5f,	0.5f);


	CMD4(CCC_Float,		"r__sun_lumscale",		&r__sun_lumscale,		-1.f,	3.f);
	CMD4(CCC_Float,		"r__sun_lumscale_hemi",	&r__sun_lumscale_hemi,	 0.f,	3.f);
	CMD4(CCC_Float,		"r__sun_lumscale_amb",	&r__sun_lumscale_amb,	 0.f,	3.f);

	CMD4(CCC_Integer,	"r__wait_sleep",		&r__wait_sleep,		0,		1);

	CMD4(CCC_Integer,	"r__dhemi_count",		&r__dhemi_count,		4,		25);
	CMD4(CCC_Float,		"r__dhemi_sky_scale",	&r__dhemi_sky_scale,	0.0f,	100.f);
	CMD4(CCC_Float,		"r__dhemi_light_scale",	&r__dhemi_light_scale,	0,		100.f);
	CMD4(CCC_Float,		"r__dhemi_light_flow",	&r__dhemi_light_flow,	0,		1.f);
	CMD4(CCC_Float,		"r__dhemi_smooth",		&r__dhemi_smooth,		0.f,	10.f);

	CMD4(CCC_Float,		"r__ls_depth_scale",	&r__ls_depth_scale,		 0.5,	1.5);
	CMD4(CCC_Float,		"r__ls_depth_bias",		&r__ls_depth_bias,		-0.5,	0.5);

	CMD4(CCC_Float,		"r__slight_fade",		&r__slight_fade,		0.2f,	1.f);

	Fvector	tw_min = {-10000, -10000, 0};
	Fvector	tw_max = { 10000,10000,10000 };

	CMD4(CCC_Dof,		"r__dof",			&r__dof,					tw_min,			tw_max);
	CMD4(CCC_DofNear,	"r__dof_near",		&r__dof.x,					tw_min.x,		tw_max.x);
	CMD4(CCC_DofFocus,	"r__dof_focus",		&r__dof.y,					tw_min.y,		tw_max.y);
	CMD4(CCC_DofFar,	"r__dof_far",		&r__dof.z,					tw_min.z,		tw_max.z);
	CMD4(CCC_Float,		"r__dof_kernel",	&r__dof_kernel,				0.0f,			10.f);
	CMD4(CCC_Float,		"r__dof_sky",		&r__dof_sky,				-10000.f,		10000.f);

	CMD1(CCC_memory_stats,	"render_memory_stats");
}

void xrRender_apply_tf()
{
	Console->Execute("r__tf_aniso");
	Console->Execute("r__tf_mipbias");
}

#pragma warning(default:4482)

#endif
