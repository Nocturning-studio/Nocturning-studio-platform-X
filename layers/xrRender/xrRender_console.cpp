///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "xrRender_console.h"
///////////////////////////////////////////////////////////////////////////////////
/*-------------------------------------------------------------------------------*/
// Render common tokens
/*-------------------------------------------------------------------------------*/
u32 ps_Preset = 3;
xr_token qpreset_token[] = {{"Minimum", 0}, {"Low", 1},		{"Default", 2}, {"High", 3},
							{"Maximum", 4}, {"Extreme", 5}, {"Ultra", 6},	{0, 0}};

u32 ps_EffPreset = 2;
xr_token qeffpreset_token[] = {
	{"st_opt_eff_disabled", 0}, {"st_opt_eff_default", 1}, {"st_opt_eff_cinematic", 2}, {0, 0}};

u32 ps_r_cubemap_size = 2048;
xr_token cubemap_size_token[] = {{"1024", 1024}, {"2048", 2048}, {"3072", 3072}, 
					   {"4096", 4096}, {"6144", 6144}, {"8192", 8192}, {0, 0}};

u32 ps_r1_aa = 0;
xr_token r1_aa_token[] = {
	{"st_opt_disabled", 0},  
	{"st_opt_msaa_2x", MSAA_2X}, 
	{"st_opt_msaa_4x", MSAA_4X}, 
	{"st_opt_msaa_8x", MSAA_8X},
	{"st_opt_csaa_4x", CSAA_4X},
	{"st_opt_csaa_8x", CSAA_8X},
	{"st_opt_ssaa_2x", SSAA_2X}, 
	{"st_opt_ssaa_4x", SSAA_4X},
	{0, 0},
};

u32 ps_r1_aa_transluency = 0;
D3DFORMAT trans_ssaa_fmt = (D3DFORMAT)MAKEFOURCC('S', 'S', 'A', 'A');
D3DFORMAT trans_atoc_fmt = (D3DFORMAT)MAKEFOURCC('A', 'T', 'O', 'C');
xr_token r1_aa_transluency_token[] = {
	{"st_opt_disabled", 0}, 
	{"st_opt_ssaa", trans_ssaa_fmt}, // õâàòèò êîììåíòèðîâàòü ýòîò ðåæèì, îí ìíå íóæåí
	{"st_opt_atoc", trans_atoc_fmt},
	{0, 0},
};

u32 ps_vignette_mode = 2;
xr_token vignette_mode_token[] = {{"st_opt_disabled", 0}, {"st_opt_static", 1}, {"st_opt_dynamic", 2}, {0, 0}};

u32 ps_r2_gbuffer_opt = 0;
xr_token r2_gbuffer_opt_token[] = {{"st_opt_disabled", 0}, {"st_opt_partial", 1}, {"st_opt_full", 2}, {0, 0}};

Flags32 ps_r2_ls_flags = {};

/*-------------------------------------------------------------------------------*/
// R2a/R2/R2.5 specific tokens
/*-------------------------------------------------------------------------------*/
//u32 ps_r2_aa = 1;
//xr_token aa_token[] = {{"st_opt_disabled", 0}, {"st_opt_rgaa", 1}, {"st_opt_dlaa", 2}, {"st_opt_fxaa", 3}, {0, 0}};

u32 ps_r2_aa_quality = 2;
xr_token aa_quality_token[] = {
	{"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {"st_opt_ultra", 4}, {0, 0}};

u32 ps_r2_aa_transluency = 1;
xr_token aa_transluency_token[] = {{"st_opt_disabled", 0}, {"st_opt_fxaa", 1}, {0, 0}};

u32 ps_r2_ao = 2;
xr_token ao_token[] = {{"st_opt_disabled", 0}, {"st_opt_ssao", 1}, {"st_opt_ssao_plus", 2}, {"st_opt_hbao_plus", 3}, {0, 0}};

u32 ps_r2_ao_quality = 2;
xr_token ao_quality_token[] = {
	{"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {"st_opt_ultra", 4}, {0, 0}};

u32 ps_r2_bloom_quality = 2;
xr_token bloom_quality_token[] = {
	{"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {"st_opt_ultra", 4}, {0, 0}};

u32 ps_r2_sun_quality = 1;
xr_token sun_quality_token[] = {{"st_opt_low", 1},	   {"st_opt_medium", 2}, {"st_opt_high", 3},
								{"st_opt_extreme", 4}, {"st_opt_ultra", 5},	 {0, 0}};

u32 ps_r2_shadow_filtering = 1;
xr_token shadow_filter_token[] = {
	{"st_opt_disable", 0}, {"st_opt_min", 1}, {"st_opt_mid", 2}, {"st_opt_max", 3}, {0, 0}};

u32 ps_r2_sun_shafts = 2;
xr_token qsun_shafts_token[] = {
	{"st_opt_disabled", 0}, {"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {0, 0}};

u32 ps_r2_bump_mode = 2;
xr_token bump_mode_token[] = {
	{"st_opt_normal_mapping", 1}, {"st_opt_parallax_mapping", 2}, {"st_opt_parallax_occlusion_mapping", 3}, {0, 0}};

u32 ps_r2_bump_quality = 1;
xr_token bump_quality_token[] = {
	{"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {"st_opt_ultra", 4}, {0, 0}};

u32 ps_r2_dof_quality = 2;
xr_token dof_quality_token[] = {
	{"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {"st_opt_ultra", 4}, {0, 0}};

u32 ps_r2_mblur_quality = 2;
xr_token mblur_quality_token[] = {
	{"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {"st_opt_ultra", 4}, {0, 0}};

u32 ps_r2_fog_quality = 2;
xr_token fog_quality_token[] = {
	{"st_opt_low", 1}, {"st_opt_medium", 2}, {"st_opt_high", 3}, {"st_opt_ultra", 4}, {0, 0}};

u32 ps_r2_debug_render = 0;
xr_token debug_render_token[] = {{"disabled", 0},
								 {"gbuffer_albedo", 1},
								 {"gbuffer_position", 2},
								 {"gbuffer_normal", 3},
								 {"gbuffer_glossiness", 4},
								 {"gbuffer_lightmap_ao", 5},
								 {"gbuffer_baked_ao", 6},
								 {"accumulator_diffuse", 7},
								 {"accumulator_specular", 8},
								 {"real_time_ao", 9},
								 {0, 0}};

u32 ps_r2_rt_format = 0;
xr_token ps_rt_format[] = {{"st_opt_rgba_32", 1}, {"st_opt_rgba_64", 2}, {0, 0}};

/*-------------------------------------------------------------------------------*/
// Render common values
/*-------------------------------------------------------------------------------*/
int ps_r_LightSleepFrames = 10;

float ps_r_hdr_white_intensity = 1.2f;
float ps_r_hdr_gamma = 1.0f;
float ps_r_hdr_brightness = 1.2f;

float ps_r_Detail_l_ambient = 0.9f;
float ps_r_Detail_l_aniso = 0.25f;
float ps_r_Detail_density = 0.3f;

int ps_r__detail_radius = 49;
u32 dm_size = 24;
u32 dm_cache1_line = 12;  // dm_size*2/dm_cache1_count
u32 dm_cache_line = 49;	  // dm_size+1+dm_size
u32 dm_cache_size = 2401; // dm_cache_line*dm_cache_line
float dm_fade = 47.5;	  // float(2*dm_size)-.5f;
u32 dm_current_size = 24;
u32 dm_current_cache1_line = 12;  // dm_current_size*2/dm_cache1_count
u32 dm_current_cache_line = 49;	  // dm_current_size+1+dm_current_size
u32 dm_current_cache_size = 2401; // dm_current_cache_line*dm_current_cache_line
float dm_current_fade = 47.5;	  // float(2*dm_current_size)-.5f;
float ps_current_detail_density = 0.6;
float ps_current_detail_scale = 1.f;

float ps_r_Tree_w_rot = 10.0f;
float ps_r_Tree_w_speed = 1.00f;
float ps_r_Tree_w_amp = 0.005f;
Fvector ps_r_Tree_Wave = {.1f, .01f, .11f};
float ps_r_Tree_SBC = 1.5f; // scale bias correct

float ps_r_WallmarkTTL = 300.f;
float ps_r_WallmarkSHIFT = 0.0001f;
float ps_r_WallmarkSHIFT_V = 0.0001f;

float ps_r_GLOD_ssa_start = 256.f;
float ps_r_GLOD_ssa_end = 64.f;
float ps_r_LOD = 1.f;
float ps_r_ssaDISCARD = 3.5f;
float ps_r_ssaDONTSORT = 32.f;
float ps_r_ssaHZBvsTEX = 96.f;

float ps_pps_u = 0.0f;
float ps_pps_v = 0.0f;

int ps_r_tf_Anisotropic = 4;

int ps_r_thread_wait_sleep = 0;

// Render common flags
Flags32 ps_render_flags = {RFLAG_LENS_FLARES | RFLAG_EXP_MT_CALC | RFLAG_EXP_HW_OCC};

/*-------------------------------------------------------------------------------*/
// R1-specific values
/*-------------------------------------------------------------------------------*/
float ps_r1_ssaLOD_A = 64.f;
float ps_r1_ssaLOD_B = 48.f;
float ps_r1_tf_Mipbias = 0.0f;
float ps_r1_lmodel_lerp = 0.1f;
float ps_r1_dlights_clip = 30.f;
int ps_r1_GlowsPerFrame = 16;

// R1-specific flags
Flags32 ps_r1_flags = {R1FLAG_DLIGHTS};

/*-------------------------------------------------------------------------------*/
// R2/R2a/R2.5-specific values
/*-------------------------------------------------------------------------------*/
float ps_r2_ssaLOD_A = 48.f;
float ps_r2_ssaLOD_B = 32.f;
float ps_r2_detalization_distance = 75.f;

float ps_r2_tf_Mipbias = 0.0f;

float ps_r2_df_parallax_h = 0.02f;

float ps_r2_autoexposure_middlegray = 1.f;
float ps_r2_autoexposure_adaptation = 1.f;
float ps_r2_autoexposure_low_lum = 0.0001f;
float ps_r2_autoexposure_amount = 0.25f;

float ps_r2_bloom_kernel_g = 4.5f;
float ps_r2_bloom_kernel_b = 1.0f;
float ps_r2_bloom_speed = 0.01f;
float ps_r2_bloom_kernel_scale = 2.0f;
float ps_r2_bloom_threshold = 0.9f;
float ps_r2_bloom_factor = 0.05f;

float ps_r2_fxaa_subpix = 0.6f;
float ps_r2_fxaa_edge_treshold = 0.063f;
float ps_r2_fxaa_edge_treshold_min = 0.0f;

float ps_r2_mblur = 0.5f;

Fvector3 ps_r2_dof = Fvector3().set(-1.25f, 1.4f, 600.f); //	x - min (0), y - focus (1.4), z - max (100)
float ps_r2_dof_sky = 300;								  //	distance to sky
float ps_r2_dof_kernel_size = 5.0f;

float ps_r2_ls_depth_scale = 1.00001f;
float ps_r2_ls_depth_bias = -0.0001f;
float ps_r2_ls_squality = 1.0f;

float ps_r2_sun_tsm_bias = -0.05f;
float ps_r2_sun_near = 12.f;
float ps_r2_sun_far = 150.f;
float ps_r2_sun_depth_far_scale = 1.00000f;
float ps_r2_sun_depth_far_bias = -0.00004f;
float ps_r2_sun_depth_near_scale = 1.00001f;
float ps_r2_sun_depth_near_bias = -0.00004f;
float ps_r2_sun_lumscale = 1.0f;
float ps_r2_sun_lumscale_hemi = 1.0f;
float ps_r2_sun_lumscale_amb = 1.0f;

float ps_r2_gmaterial = 0.f;

float ps_r2_zfill = 0.1f;

float ps_r2_dhemi_sky_scale = 0.08f;
float ps_r2_dhemi_light_scale = 0.2f;
float ps_r2_dhemi_light_flow = 0.1f;
float ps_r2_dhemi_scale = 1.f;
int ps_r2_dhemi_count = 5;

float ps_r2_lt_smooth = 1.f;

float ps_r2_slight_fade = 1.f;

float ps_r2_gloss_factor = 1.0f;

float ps_cas_contrast = 0.1f;
float ps_cas_sharpening = 0.1f;

// R2-specific flags
Flags32 ps_r2_lighting_flags = {R2FLAG_SUN | R2FLAG_EXP_DONT_TEST_UNSHADOWED};

Flags32 ps_r2_postprocess_flags = {R2FLAG_AUTOEXPOSURE | R2FLAG_DOF | R2FLAG_MBLUR};

Flags32 ps_r2_overlay_flags = {
	0,
};

/*-------------------------------------------------------------------------------*/
// Methods
/*-------------------------------------------------------------------------------*/
#ifndef _EDITOR
///////////////////////////////////////////////////////////////////////////////////
#include "..\xrEngine\xr_ioconsole.h"
#include "..\xrEngine\xr_ioc_cmd.h"
///////////////////////////////////////////////////////////////////////////////////
#define MAX_CONDITIONS_TOKEN_SIZE 32
class CCC_ConditionsToken : public CCC_Token
{
  protected:
	xr_token tokens_mem[MAX_CONDITIONS_TOKEN_SIZE];
	bool conditions[MAX_CONDITIONS_TOKEN_SIZE];

  public:
	CCC_ConditionsToken(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T)
	{
		memset(tokens_mem, 0, sizeof(tokens_mem));
		memset(conditions, 1, sizeof(conditions));
	};

	void SetCondition(int key, bool condition)
	{
		for (int i = 0; tokens[i].name && i < MAX_CONDITIONS_TOKEN_SIZE; i++)
		{
			if (key == tokens[i].id)
			{
				conditions[i] = condition;
				break;
			}
		}
	}

	void ApplyConditions()
	{
		memset(tokens_mem, 0, sizeof(tokens_mem));

		for (int i = 0, j = 0; tokens[i].name && j < MAX_CONDITIONS_TOKEN_SIZE; i++)
		{
			if (conditions[i] == true)
			{
				tokens_mem[j] = tokens[i];
				j++;
			}
		}

		tokens = tokens_mem;
	};
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_tf_Aniso : public CCC_Integer
{
  public:
	void apply()
	{
		if (0 == HW.pDevice11)
			return;
		int val = *value;
		clamp(val, 1, 16);
		//for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
		//	CHK_DX(HW.pDevice11->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, val));
		SSManager.SetMaxAnisotropy(val);
	}
	CCC_tf_Aniso(LPCSTR N, int* v) : CCC_Integer(N, v, 1, 16){};
	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
		apply();
	}
	virtual void Status(TStatus& S)
	{
		CCC_Integer::Status(S);
		apply();
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_tf_MipBias : public CCC_Float
{
  public:
	void apply()
	{
#pragma message(Reminder("fix mip bias options"))
		//if (0 == HW.pDevice11)
		//	return;
		//for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
		//	CHK_DX(HW.pDevice11->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)value)));
	}

	CCC_tf_MipBias(LPCSTR N, float* v) : CCC_Float(N, v, -0.5f, +0.5f){};
	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);
		apply();
	}
	virtual void Status(TStatus& S)
	{
		CCC_Float::Status(S);
		apply();
	}
};
///////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
class CCC_R2GM : public CCC_Float
{
  public:
	CCC_R2GM(LPCSTR N, float* v) : CCC_Float(N, v, 0.f, 4.f)
	{
		*v = 0;
	};
	virtual void Execute(LPCSTR args)
	{
		if (0 == xr_strcmp(args, "on"))
		{
			ps_r2_ls_flags.set(R2FLAG_GLOBALMATERIAL, TRUE);
		}
		else if (0 == xr_strcmp(args, "off"))
		{
			ps_r2_ls_flags.set(R2FLAG_GLOBALMATERIAL, FALSE);
		}
		else
		{
			CCC_Float::Execute(args);
			if (ps_r2_ls_flags.test(R2FLAG_GLOBALMATERIAL))
			{
				static LPCSTR name[4] = {"oren", "blin", "phong", "metal"};
				float mid = *value;
				int m0 = iFloor(mid) % 4;
				int m1 = (m0 + 1) % 4;
				float frc = mid - float(iFloor(mid));
				Msg("* material set to [%s]-[%s], with lerp of [%f]", name[m0], name[m1], frc);
			}
		}
	}
};
#endif
///////////////////////////////////////////////////////////////////////////////////
class CCC_Screenshot : public IConsole_Command
{
  public:
	CCC_Screenshot(LPCSTR N) : IConsole_Command(N){};
	virtual void Execute(LPCSTR args)
	{
		string_path name;
		name[0] = 0;
		sscanf(args, "%s", name);
		LPCSTR image = xr_strlen(name) ? name : 0;
		::Render->Screenshot(IRender_interface::SM_NORMAL, image);
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_ModelPoolStat : public IConsole_Command
{
  public:
	CCC_ModelPoolStat(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	};
	virtual void Execute(LPCSTR args)
	{
		RImplementation.Models->dump();
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_Preset : public CCC_Token
{
  public:
	CCC_Preset(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T){};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);
		string_path _cfg;
		string_path cmd;

		switch (*value)
		{
		case 0:
			strcpy(_cfg, "rspec_minimum.ltx");
			break;
		case 1:
			strcpy(_cfg, "rspec_low.ltx");
			break;
		case 2:
			strcpy(_cfg, "rspec_default.ltx");
			break;
		case 3:
			strcpy(_cfg, "rspec_high.ltx");
			break;
		case 4:
			strcpy(_cfg, "rspec_maximum.ltx");
			break;
		case 5:
			strcpy(_cfg, "rspec_extreme.ltx");
			break;
		case 6:
			strcpy(_cfg, "rspec_ultra.ltx");
			break;
		}
		FS.update_path(_cfg, "$game_config$", _cfg);
		strconcat(sizeof(cmd), cmd, "cfg_load", " ", _cfg);
		Console->Execute(cmd);
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_EffPreset : public CCC_Token
{
  public:
	CCC_EffPreset(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T){};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);
		string_path _cfg;
		string_path cmd;

		switch (*value)
		{
		case 0:
			strcpy(_cfg, "eff_disabled.ltx");
			break;
		case 1:
			strcpy(_cfg, "eff_default.ltx");
			break;
		case 2:
			strcpy(_cfg, "eff_cinematic.ltx");
			break;
		}
		FS.update_path(_cfg, "$game_config$", _cfg);
		strconcat(sizeof(cmd), cmd, "cfg_load", " ", _cfg);
		Console->Execute(cmd);
	}
};
///////////////////////////////////////////////////////////////////////////////////
bool detail_opt_changed = false;
class CCC_detail_radius : public CCC_Integer
{
  public:
	void apply()
	{
		dm_current_size = iFloor((float)ps_r__detail_radius / 4) * 2;
		dm_current_cache1_line = dm_current_size * 2 / 4; // assuming cache1_count = 4
		dm_current_cache_line = dm_current_size + 1 + dm_current_size;
		dm_current_cache_size = dm_current_cache_line * dm_current_cache_line;
		dm_current_fade = float(2 * dm_current_size) - .5f;
		detail_opt_changed = true;
	}
	CCC_detail_radius(LPCSTR N, int* V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max){};
	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
		apply();
	}
	virtual void Status(TStatus& S)
	{
		CCC_Integer::Status(S);
	}
};
class CCC_detail_opt : public CCC_Mask
{
  public:
	CCC_detail_opt(LPCSTR N, Flags32* V, u32 M) : CCC_Mask(N, V, M)
	{
	}
	void Execute(LPCSTR args)
	{
		CCC_Mask::Execute(args);
		detail_opt_changed = true;
	}
};

///////////////////////////////////////////////////////////////////////////////////
#if RENDER == R_R2
///////////////////////////////////////////////////////////////////////////////////
#include "r_pixel_calculator.h"
///////////////////////////////////////////////////////////////////////////////////
class CCC_BuildSSA : public IConsole_Command
{
  public:
	CCC_BuildSSA(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	};
	virtual void Execute(LPCSTR args)
	{
#pragma message(Reminder("Fix r_pixel_calculator (Build SSA)"))
		//r_pixel_calculator c;
		//c.run();
	}
};
///////////////////////////////////////////////////////////////////////////////////
#endif // RENDER==R_R2
///////////////////////////////////////////////////////////////////////////////////
class CCC_DofFar : public CCC_Float
{
  public:
	CCC_DofFar(LPCSTR N, float* V, float _min = 0.0f, float _max = 10000.0f) : CCC_Float(N, V, _min, _max)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		float v = float(atof(args));

		if (v < ps_r2_dof.y + 0.1f)
		{
			char pBuf[256];
			_snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value greater or equal to r2_dof_focus+0.1");
			Msg("~ Invalid syntax in call to '%s'", cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_focus");
		}
		else
		{
			CCC_Float::Execute(args);
			if (g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
		}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void Save(IWriter* F)
	{
		;
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_DofNear : public CCC_Float
{
  public:
	CCC_DofNear(LPCSTR N, float* V, float _min = 0.0f, float _max = 10000.0f) : CCC_Float(N, V, _min, _max)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		float v = float(atof(args));

		if (v > ps_r2_dof.y - 0.1f)
		{
			char pBuf[256];
			_snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value less or equal to r2_dof_focus-0.1");
			Msg("~ Invalid syntax in call to '%s'", cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_focus");
		}
		else
		{
			CCC_Float::Execute(args);
			if (g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
		}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void Save(IWriter* F)
	{
		;
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_DofFocus : public CCC_Float
{
  public:
	CCC_DofFocus(LPCSTR N, float* V, float _min = 0.0f, float _max = 10000.0f) : CCC_Float(N, V, _min, _max)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		float v = float(atof(args));

		if (v > ps_r2_dof.z - 0.1f)
		{
			char pBuf[256];
			_snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value less or equal to r2_dof_far-0.1");
			Msg("~ Invalid syntax in call to '%s'", cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_far");
		}
		else if (v < ps_r2_dof.x + 0.1f)
		{
			char pBuf[256];
			_snprintf(pBuf, sizeof(pBuf) / sizeof(pBuf[0]), "float value greater or equal to r2_dof_far-0.1");
			Msg("~ Invalid syntax in call to '%s'", cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_near");
		}
		else
		{
			CCC_Float::Execute(args);
			if (g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
		}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void Save(IWriter* F)
	{
		;
	}
};
///////////////////////////////////////////////////////////////////////////////////
class CCC_Dof : public CCC_Vector3
{
  public:
	CCC_Dof(LPCSTR N, Fvector* V, const Fvector _min, const Fvector _max) : CCC_Vector3(N, V, _min, _max)
	{
		;
	}

	virtual void Execute(LPCSTR args)
	{
		Fvector v;
		if (3 != sscanf(args, "%f,%f,%f", &v.x, &v.y, &v.z))
			InvalidSyntax();
		else if ((v.x > v.y - 0.1f) || (v.z < v.y + 0.1f))
		{
			InvalidSyntax();
			Msg("x <= y - 0.1");
			Msg("y <= z - 0.1");
		}
		else
		{
			CCC_Vector3::Execute(args);
			if (g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
		}
	}
	virtual void Status(TStatus& S)
	{
		sprintf(S, "%f,%f,%f", value->x, value->y, value->z);
	}
	virtual void Info(TInfo& I)
	{
		sprintf(I, "vector3 in range [%f,%f,%f]-[%f,%f,%f]", min.x, min.y, min.z, max.x, max.y, max.z);
	}
};
///////////////////////////////////////////////////////////////////////////////////
void xrRender_initconsole()
{
	Fvector tw_min, tw_max;

	// Render common commands
	CMD3(CCC_Preset, "_preset", &ps_Preset, qpreset_token);
	CMD3(CCC_EffPreset, "eff_preset", &ps_EffPreset, qeffpreset_token);

	CMD1(CCC_Screenshot, "screenshot");

	CMD4(CCC_Integer, "r_lsleep_frames", &ps_r_LightSleepFrames, 4, 30);
	CMD4(CCC_Float, "r_ssa_glod_start", &ps_r_GLOD_ssa_start, 128, 512);
	CMD4(CCC_Float, "r_ssa_glod_end", &ps_r_GLOD_ssa_end, 16, 96);
	CMD4(CCC_Float, "r_wallmark_shift_pp", &ps_r_WallmarkSHIFT, 0.0f, 1.f);
	CMD4(CCC_Float, "r_wallmark_shift_v", &ps_r_WallmarkSHIFT_V, 0.0f, 1.f);
	CMD4(CCC_Float, "r_wallmark_ttl", &ps_r_WallmarkTTL, 1.0f, 5.f * 60.f);
	CMD1(CCC_ModelPoolStat, "stat_models");

	CMD3(CCC_Token, "r_cubemap_size", &ps_r_cubemap_size, cubemap_size_token);

	CMD4(CCC_Float, "r_geometry_lod", &ps_r_LOD, 0.1f, 1.2f);

	//CMD4(CCC_Float, "r_detail_density", &ps_r_Detail_density, .2f, 0.6f);

	CMD4(CCC_Float, "r_detail_l_ambient", &ps_r_Detail_l_ambient, .5f, .95f);
	CMD4(CCC_Float, "r_detail_l_aniso", &ps_r_Detail_l_aniso, .1f, .5f);

	CMD4(CCC_Float, "r_detail_density", &ps_current_detail_density, 0.1f, 1.0f);
	CMD4(CCC_Float, "r_detail_scale", &ps_current_detail_scale, 0.2f, 3.0f);
	CMD4(CCC_detail_radius, "r_detail_radius", &ps_r__detail_radius, 32, 256);
	CMD3(CCC_detail_opt, "r_detail_dx11_opt", &ps_render_flags, RFLAG_DETAILS_DX11_OPT);

	CMD3(CCC_Mask, "r_png_screenshots", &ps_render_flags, RFLAG_PNG_SCREENSHOTS);

	CMD4(CCC_Float, "r_d_tree_w_amp", &ps_r_Tree_w_amp, .001f, 1.f);
	CMD4(CCC_Float, "r_d_tree_w_rot", &ps_r_Tree_w_rot, .01f, 100.f);
	CMD4(CCC_Float, "r_d_tree_w_speed", &ps_r_Tree_w_speed, 1.0f, 10.f);

	tw_min.set(EPS, EPS, EPS);
	tw_max.set(2, 2, 2);
	CMD4(CCC_Vector3, "r_d_tree_wave", &ps_r_Tree_Wave, tw_min, tw_max);

	CMD2(CCC_tf_Aniso, "r_tf_aniso", &ps_r_tf_Anisotropic); //	{1..16}

	CMD3(CCC_Mask, "r_lens_flares", &ps_render_flags, RFLAG_LENS_FLARES);

	CMD3(CCC_Mask, "r_sepia", &ps_render_flags, RFLAG_SEPIA);
	CMD3(CCC_Token, "r_vignette_mode", &ps_vignette_mode, vignette_mode_token);
	CMD3(CCC_Mask, "r_chromatic_abberation", &ps_render_flags, RFLAG_CHROMATIC_ABBERATION);
	CMD4(CCC_Float, "r_hdr_white_intensity", &ps_r_hdr_white_intensity, 0, 2);
	CMD4(CCC_Float, "r_hdr_gamma", &ps_r_hdr_gamma, 1, 1.75);
	CMD4(CCC_Float, "r_hdr_brightness", &ps_r_hdr_brightness, 1, 1.75);

	CMD3(CCC_Mask, "r_mt", &ps_render_flags, RFLAG_EXP_MT_CALC);

	CMD3(CCC_Token, "r2_gbuffer_opt", &ps_r2_gbuffer_opt, r2_gbuffer_opt_token);

	CMD4(CCC_Integer, "r_wait_sleep", &ps_r_thread_wait_sleep, 0, 1);

	CMD3(CCC_Mask, "r_hardware_occlusion_culling", &ps_render_flags, RFLAG_EXP_HW_OCC);

	CMD4(CCC_Float, "r_pps_u", &ps_pps_u, -1.f, +1.f);
	CMD4(CCC_Float, "r_pps_v", &ps_pps_v, -1.f, +1.f);

	// R1-specific commands
	CMD4(CCC_Float, "r1_ssa_lod_a", &ps_r1_ssaLOD_A, 16, 96);
	CMD4(CCC_Float, "r1_ssa_lod_b", &ps_r1_ssaLOD_B, 16, 64);
	CMD4(CCC_Float, "r1_lmodel_lerp", &ps_r1_lmodel_lerp, 0, 0.333f);
	CMD2(CCC_tf_MipBias, "r1_tf_mipbias", &ps_r1_tf_Mipbias); //	{-3 +3}
	CMD3(CCC_Mask, "r1_dlights", &ps_r1_flags, R1FLAG_DLIGHTS);
	CMD4(CCC_Float, "r1_dlights_clip", &ps_r1_dlights_clip, 10.f, 150.f);
	CMD4(CCC_Float, "r1_dlights_clip", &ps_r1_dlights_clip, 10.f, 150.f);
	CMD4(CCC_Integer, "r1_glows_per_frame", &ps_r1_GlowsPerFrame, 2, 32);

	// R2/R2a/R2.5-specific commands
	CMD3(CCC_Mask, "r2_soft_water", &ps_r2_postprocess_flags, R2FLAG_SOFT_WATER);
	CMD3(CCC_Mask, "r2_soft_particles", &ps_r2_postprocess_flags, R2FLAG_SOFT_PARTICLES);
	CMD3(CCC_Token, "r2_fog_quality", &ps_r2_fog_quality, fog_quality_token);

	CMD3(CCC_Token, "r2_aa_type", &ps_r2_aa, aa_token);
	CMD3(CCC_Token, "r2_aa_quality", &ps_r2_aa_quality, aa_quality_token);
	CMD3(CCC_Token, "r2_aa_transluency", &ps_r2_aa_transluency, aa_transluency_token);
	CMD4(CCC_Float, "r2_fxaa_subpix", &ps_r2_fxaa_subpix, 0.0f, 1.0f);
	CMD4(CCC_Float, "r2_fxaa_treshold", &ps_r2_fxaa_edge_treshold, 0.0f, 1.0f);
	CMD4(CCC_Float, "r2_fxaa_treshold_min", &ps_r2_fxaa_edge_treshold_min, 0.0f, 1.0f);

	CMD3(CCC_Token, "r2_ao_type", &ps_r2_ao, ao_token);
	CMD3(CCC_Token, "r2_ao_quality", &ps_r2_ao_quality, ao_quality_token);

	CMD3(CCC_Mask, "r2_autoexposure", &ps_r2_postprocess_flags, R2FLAG_AUTOEXPOSURE);
	CMD4(CCC_Float, "r2_autoexposure_middlegray", &ps_r2_autoexposure_middlegray, 0.0f, 2.0f);
	CMD4(CCC_Float, "r2_autoexposure_adaptation", &ps_r2_autoexposure_adaptation, 0.01f, 10.0f);
	CMD4(CCC_Float, "r2_autoexposure_lowlum", &ps_r2_autoexposure_low_lum, 0.0001f, 1.0f);
	CMD4(CCC_Float, "r2_autoexposure_amount", &ps_r2_autoexposure_amount, 0.0000f, 1.0f);

	CMD3(CCC_Mask, "r2_bloom", &ps_r2_postprocess_flags, R2FLAG_BLOOM);
	CMD3(CCC_Token, "r2_bloom_quality", &ps_r2_bloom_quality, bloom_quality_token);
	CMD4(CCC_Float, "r2_bloom_kernel_scale", &ps_r2_bloom_kernel_scale, 0.5f, 2.0f);
	CMD4(CCC_Float, "r2_bloom_kernel_g", &ps_r2_bloom_kernel_g, 1.0f, 7.0f);
	CMD4(CCC_Float, "r2_bloom_kernel_b", &ps_r2_bloom_kernel_b, 0.01f, 1.0f);
	CMD4(CCC_Float, "r2_bloom_threshold", &ps_r2_bloom_threshold, 0.0f, 1.0f);
	CMD4(CCC_Float, "r2_bloom_speed", &ps_r2_bloom_speed, 0.0f, 100.0f);
	CMD4(CCC_Float, "r2_bloom_factor", &ps_r2_bloom_factor, 0.0f, 1.0f);

	CMD3(CCC_Mask, "r2_mblur_enabled", &ps_r2_postprocess_flags, R2FLAG_MBLUR);
	CMD4(CCC_Float, "r2_mblur_power", &ps_r2_mblur, 0.0f, 1.0f);
	CMD3(CCC_Token, "r2_mblur_quality", &ps_r2_mblur_quality, mblur_quality_token);

	tw_min.set(-10000, -10000, 0);
	tw_max.set(10000, 10000, 10000);
	CMD4(CCC_Dof, "r2_dof", &ps_r2_dof, tw_min, tw_max);
	CMD4(CCC_DofNear, "r2_dof_near", &ps_r2_dof.x, tw_min.x, tw_max.x);
	CMD4(CCC_DofFocus, "r2_dof_focus", &ps_r2_dof.y, tw_min.y, tw_max.y);
	CMD4(CCC_DofFar, "r2_dof_far", &ps_r2_dof.z, tw_min.z, tw_max.z);
	CMD4(CCC_Float, "r2_dof_kernel", &ps_r2_dof_kernel_size, .0f, 10.f);
	CMD4(CCC_Float, "r2_dof_sky", &ps_r2_dof_sky, -10000.f, 10000.f);
	CMD3(CCC_Mask, "r2_dof_enabled", &ps_r2_postprocess_flags, R2FLAG_DOF);
	CMD3(CCC_Token, "r2_dof_quality", &ps_r2_dof_quality, dof_quality_token);

	CMD3(CCC_Mask, "r2_barrel_blur", &ps_r2_postprocess_flags, R2FLAG_BARREL_BLUR);

	CMD3(CCC_Mask, "r2_cas_enabled", &ps_r2_postprocess_flags, R2FLAG_CONTRAST_ADAPTIVE_SHARPENING);
	CMD4(CCC_Float, "r2_cas_contrast", &ps_cas_contrast, 0.0f, 1.0f);
	CMD4(CCC_Float, "r2_cas_sharpening", &ps_cas_sharpening, 0.0f, 1.0f);

	CMD3(CCC_Mask, "r2_photo_grid", &ps_r2_overlay_flags, R2FLAG_PHOTO_GRID);
	CMD3(CCC_Mask, "r2_cinema_borders", &ps_r2_overlay_flags, R2FLAG_CINEMA_BORDERS);
	CMD3(CCC_Mask, "r2_watermark", &ps_r2_overlay_flags, R2FLAG_WATERMARK);

	CMD3(CCC_Token, "r2_sun_shafts", &ps_r2_sun_shafts, qsun_shafts_token);
	CMD3(CCC_Token, "r2_sun_quality", &ps_r2_sun_quality, sun_quality_token);
	CMD3(CCC_Token, "r2_shadow_filtering", &ps_r2_shadow_filtering, shadow_filter_token);
	CMD3(CCC_Mask, "r2_sun", &ps_r2_lighting_flags, R2FLAG_SUN);
	CMD3(CCC_Mask, "r2_sun_details", &ps_r2_lighting_flags, R2FLAG_SUN_DETAILS);
	CMD3(CCC_Mask, "r2_exp_donttest_uns", &ps_r2_lighting_flags, R2FLAG_EXP_DONT_TEST_UNSHADOWED);
	CMD4(CCC_Float, "r2_sun_tsm_bias", &ps_r2_sun_tsm_bias, -0.5, +0.5);
	CMD4(CCC_Float, "r2_sun_near", &ps_r2_sun_near, 1.f, 50.f);
	CMD4(CCC_Float, "r2_sun_far", &ps_r2_sun_far, 100.f, 360.f);
	CMD4(CCC_Float, "r2_sun_depth_far_scale", &ps_r2_sun_depth_far_scale, 0.5, 1.5);
	CMD4(CCC_Float, "r2_sun_depth_far_bias", &ps_r2_sun_depth_far_bias, -0.5, +0.5);
	CMD4(CCC_Float, "r2_sun_depth_near_scale", &ps_r2_sun_depth_near_scale, 0.5, 1.5);
	CMD4(CCC_Float, "r2_sun_depth_near_bias", &ps_r2_sun_depth_near_bias, -0.5, +0.5);
	CMD4(CCC_Float, "r2_sun_lumscale", &ps_r2_sun_lumscale, -1.0, +3.0);
	CMD4(CCC_Float, "r2_sun_lumscale_hemi", &ps_r2_sun_lumscale_hemi, 0.0, +3.0);
	CMD4(CCC_Float, "r2_sun_lumscale_amb", &ps_r2_sun_lumscale_amb, 0.0, +3.0);

	CMD3(CCC_Mask, "r2_shadow_cascede_zcul", &ps_r2_lighting_flags, R2FLAGEXT_SUN_ZCULLING);

	CMD3(CCC_Mask, "r2_allow_r1_lights", &ps_r2_lighting_flags, R2FLAG_R1LIGHTS);

	CMD4(CCC_Float, "r2_slight_fade", &ps_r2_slight_fade, .02f, 2.f);

	CMD4(CCC_Integer, "r2_dhemi_count", &ps_r2_dhemi_count, 4, 25);
	CMD4(CCC_Float, "r2_dhemi_scale", &ps_r2_dhemi_scale, .5f, 3.f);
	CMD4(CCC_Float, "r2_dhemi_smooth", &ps_r2_lt_smooth, 0.f, 10.f);

	CMD3(CCC_Token, "r2_bump_mode", &ps_r2_bump_mode, bump_mode_token);
	CMD3(CCC_Token, "r2_bump_quality", &ps_r2_bump_quality, bump_quality_token);
	CMD4(CCC_Float, "r2_parallax_h", &ps_r2_df_parallax_h, .0f, .5f);

	CMD3(CCC_Token, "r2_debug_render", &ps_r2_debug_render, debug_render_token);

	CMD3(CCC_Token, "r2_rt_format", &ps_r2_rt_format, ps_rt_format);

#if RENDER == R_R2
	CMD1(CCC_BuildSSA, "build_ssa");
#endif

	CMD4(CCC_Float, "r2_ssa_lod_a", &ps_r2_ssaLOD_A, 16, 96);
	CMD4(CCC_Float, "r2_ssa_lod_b", &ps_r2_ssaLOD_B, 32, 64);
	CMD4(CCC_Float, "r2_detalization_distance", &ps_r2_detalization_distance, 10.0f, 100.0f);

	CMD2(CCC_tf_MipBias, "r2_tf_mipbias", &ps_r2_tf_Mipbias);

#ifdef DEBUG
	CMD2(CCC_R2GM, "r2_global_material", &ps_r2_gmaterial);
#endif

	CMD4(CCC_Float, "r2_ls_squality", &ps_r2_ls_squality, .5f, 1.f);

	CMD3(CCC_Mask, "r2_zfill", &ps_r2_ls_flags, R2FLAG_ZFILL);
	CMD4(CCC_Float, "r2_zfill_depth", &ps_r2_zfill, .001f, .5f);

	CMD4(CCC_Float, "r2_gloss_factor", &ps_r2_gloss_factor, 1.f, 3.f);

	CMD3(CCC_Mask, "r2_use_nvdbt", &ps_r2_ls_flags, R2FLAG_USE_NVDBT);

	CMD4(CCC_Float, "r2_ls_depth_scale", &ps_r2_ls_depth_scale, 0.5, 1.5);
	CMD4(CCC_Float, "r2_ls_depth_bias", &ps_r2_ls_depth_bias, -0.5, +0.5);

	// !!! ÑÒÐÎÃÎ ÂÍÈÇÓ ÏÎÑËÅ ÂÑÅÕ ÎÏÖÈÉ !!!
	CMD3(CCC_ConditionsToken, "r1_aa_type", &ps_r1_aa, r1_aa_token);
	CMD3(CCC_ConditionsToken, "r1_aa_transluency", &ps_r1_aa_transluency, r1_aa_transluency_token);
}

void xrRender_console_apply_conditions()
{
	CCC_ConditionsToken* r1_aa_type = dynamic_cast<CCC_ConditionsToken*>(Console->GetCommand("r1_aa_type"));
	r1_aa_type->SetCondition(CSAA_4X, HW.Caps.max_coverage >= 2);
	r1_aa_type->SetCondition(CSAA_8X, HW.Caps.max_coverage >= 4);
	r1_aa_type->ApplyConditions();

	CCC_ConditionsToken* r1_aa_transluency = dynamic_cast<CCC_ConditionsToken*>(Console->GetCommand("r1_aa_transluency"));
	//r1_aa_transluency->SetCondition(trans_ssaa_fmt, HW.support(trans_ssaa_fmt, D3DRTYPE_SURFACE, NULL));
	//r1_aa_transluency->SetCondition(trans_atoc_fmt, HW.support(trans_atoc_fmt, D3DRTYPE_SURFACE, NULL));
	r1_aa_transluency->SetCondition(trans_ssaa_fmt, TRUE);
	r1_aa_transluency->SetCondition(trans_atoc_fmt, TRUE);
	r1_aa_transluency->ApplyConditions();
}
///////////////////////////////////////////////////////////////////////////////////
void xrRender_apply_tf()
{
	Console->Execute("r_tf_aniso");
#if RENDER == R_R1
	Console->Execute("r1_tf_mipbias");
#else
	Console->Execute("r2_tf_mipbias");
#endif
}
///////////////////////////////////////////////////////////////////////////////////
#endif // #ifndef _EDITOR
///////////////////////////////////////////////////////////////////////////////////
