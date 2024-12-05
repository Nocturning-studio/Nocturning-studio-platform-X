//-----------------------------------------------------------------------------
// Environment descriptor
//-----------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "xr_efflensflare.h"
#include "thunderbolt.h"

CEnvDescriptor::CEnvDescriptor(shared_str const& identifier) : m_identifier(identifier)
{
	exec_time = 0.0f;
	exec_time_loaded = 0.0f;

	clouds_color.set(1, 1, 1, 1);
	sky_color.set(1, 1, 1);
	sky_rotation = 0.0f;

	far_plane = 400.0f;

	fog_color.set(1, 1, 1);
	fog_density = 0.0f;
	fog_sky_influence = 0.0f;
	vertical_fog_intensity = 0.0001f;
	vertical_fog_density = 0.0001f;
	vertical_fog_height = 0.8f;

	rain_density = 0.0f;
	rain_color.set(0, 0, 0);

	bolt_period = 0.0f;
	bolt_duration = 0.0f;

	wind_velocity = 0.0f;
	wind_direction = 0.0f;

	ambient.set(0, 0, 0);
	hemi_color.set(1, 1, 1, 1);
	sun_color.set(1, 1, 1);
	sun_dir.set(0, -1, 0);

	m_fSunShaftsIntensity = 0;
	m_fWaterIntensity = 1;

	lens_flare_id = "";
	tb_id = "";

	m_fTreeAmplitude = 0.005f;
	m_fTreeSpeed = 1.00f;
	m_fTreeRotation = 10.0f;
	m_fTreeWave.set(.1f, .01f, .11f);

	m_SepiaColor.set(1.0f, 1.0f, 1.0f);
	m_SepiaPower = 0.0f;
	m_VignettePower = 0.2f;

	env_ambient = NULL;
}

#define C_CHECK(C)                                                                                                     \
	if (C.x < 0 || C.x > 2 || C.y < 0 || C.y > 2 || C.z < 0 || C.z > 2)                                                \
	{                                                                                                                  \
		Msg("! Invalid '%s' in env-section '%s'", #C, m_identifier.c_str());                                           \
	}

void CEnvDescriptor::load(CEnvironment& environment, CInifile& config)
{
	Ivector3 tm = {0, 0, 0};
	sscanf(m_identifier.c_str(), "%d:%d:%d", &tm.x, &tm.y, &tm.z);
	R_ASSERT3((tm.x >= 0) && (tm.x < 24) && (tm.y >= 0) && (tm.y < 60) && (tm.z >= 0) && (tm.z < 60),
			  "Incorrect weather time", m_identifier.c_str());

	exec_time = tm.x * 3600.f + tm.y * 60.f + tm.z;
	exec_time_loaded = exec_time;

	string_path st, st_env;
	strcpy(st, config.r_string(m_identifier.c_str(), "sky_texture"));
	strconcat(sizeof(st_env), st_env, st, "#small");
	sky_texture_name = st;
	sky_texture_env_name = st_env;
	clouds_texture_name = config.r_string(m_identifier.c_str(), "clouds_texture");
	LPCSTR cldclr = config.r_string(m_identifier.c_str(), "clouds_color");

	if (config.line_exist(m_identifier.c_str(), "lut_texture"))
		lut_texture_name = config.r_string(m_identifier.c_str(), "lut_texture");
	else
		lut_texture_name = "lut\\lut_neutral";

	float multiplier = 0, save = 0;
	sscanf(cldclr, "%f,%f,%f,%f,%f", &clouds_color.x, &clouds_color.y, &clouds_color.z, &clouds_color.w, &multiplier);
	save = clouds_color.w;
	clouds_color.w = save;

	sky_color = config.r_fvector3(m_identifier.c_str(), "sky_color");

	if (config.line_exist(m_identifier.c_str(), "sky_rotation"))
		sky_rotation = deg2rad(config.r_float(m_identifier.c_str(), "sky_rotation"));
	else
		sky_rotation = 0;
	far_plane = config.r_float(m_identifier.c_str(), "far_plane");
	fog_color = config.r_fvector3(m_identifier.c_str(), "fog_color");
	fog_density = config.r_float(m_identifier.c_str(), "fog_density");

	if (config.line_exist(m_identifier.c_str(), "fog_sky_influence"))
		fog_sky_influence = config.r_float(m_identifier.c_str(), "fog_sky_influence");
	else
		fog_sky_influence = 0.0000f;

	if (config.line_exist(m_identifier.c_str(), "vertical_fog_intensity"))
		vertical_fog_intensity = config.r_float(m_identifier.c_str(), "vertical_fog_intensity");
	else
		vertical_fog_intensity = 0.0001f;

	if (config.line_exist(m_identifier.c_str(), "vertical_fog_density"))
		vertical_fog_density = config.r_float(m_identifier.c_str(), "vertical_fog_density");
	else
		vertical_fog_density = 0.0001f;

	if (config.line_exist(m_identifier.c_str(), "vertical_fog_height"))
		vertical_fog_height = config.r_float(m_identifier.c_str(), "vertical_fog_height");
	else
		vertical_fog_height = 0.8f;

	rain_density = config.r_float(m_identifier.c_str(), "rain_density");
	clamp(rain_density, 0.f, 1.f);
	rain_color = config.r_fvector3(m_identifier.c_str(), "rain_color");
	wind_velocity = config.r_float(m_identifier.c_str(), "wind_velocity");
	wind_direction = deg2rad(config.r_float(m_identifier.c_str(), "wind_direction"));
	ambient = config.r_fvector3(m_identifier.c_str(), "ambient_color");
	hemi_color = config.r_fvector4(m_identifier.c_str(), "hemisphere_color");
	sun_color = config.r_fvector3(m_identifier.c_str(), "sun_color");

	lens_flare_id = environment.eff_LensFlare->AppendDef(environment, environment.m_suns_config,
														 config.r_string(m_identifier.c_str(), "sun"));

	tb_id = environment.eff_Thunderbolt->AppendDef(environment, environment.m_thunderbolt_collections_config,
												   environment.m_thunderbolts_config,
												   config.r_string(m_identifier.c_str(), "thunderbolt_collection"));

	bolt_period = (tb_id.size()) ? config.r_float(m_identifier.c_str(), "thunderbolt_period") : 0.f;
	bolt_duration = (tb_id.size()) ? config.r_float(m_identifier.c_str(), "thunderbolt_duration") : 0.f;
	env_ambient = config.line_exist(m_identifier.c_str(), "ambient")
					  ? environment.AppendEnvAmb(config.r_string(m_identifier.c_str(), "ambient"))
					  : 0;

	if (config.line_exist(m_identifier.c_str(), "sun_shafts_intensity"))
		m_fSunShaftsIntensity = config.r_float(m_identifier.c_str(), "sun_shafts_intensity");

	if (config.line_exist(m_identifier.c_str(), "water_intensity"))
		m_fWaterIntensity = config.r_float(m_identifier.c_str(), "water_intensity");

	m_fTreeAmplitude = config.line_exist(m_identifier.c_str(), "trees_amplitude")
						   ? config.r_float(m_identifier.c_str(), "trees_amplitude")
						   : 0.005f;
	m_fTreeSpeed = config.line_exist(m_identifier.c_str(), "trees_speed")
					   ? config.r_float(m_identifier.c_str(), "trees_speed")
					   : 1.00f;
	m_fTreeRotation = config.line_exist(m_identifier.c_str(), "trees_rotation")
						  ? config.r_float(m_identifier.c_str(), "trees_rotation")
						  : 10.0f;

	if (config.line_exist(m_identifier.c_str(), "trees_wave"))
		m_fTreeWave = config.r_fvector3(m_identifier.c_str(), "trees_wave");
	else
		m_fTreeWave.set(.1f, .01f, .11f);

	if (config.line_exist(m_identifier.c_str(), "sepia_color"))
		m_SepiaColor = config.r_fvector3(m_identifier.c_str(), "sepia_color");

	if (config.line_exist(m_identifier.c_str(), "sepia_power"))
		m_SepiaPower = config.r_float(m_identifier.c_str(), "sepia_power");

	if (config.line_exist(m_identifier.c_str(), "vignette_power"))
		m_VignettePower = config.r_float(m_identifier.c_str(), "vignette_power");

	C_CHECK(clouds_color);
	C_CHECK(sky_color);
	C_CHECK(fog_color);
	C_CHECK(rain_color);
	C_CHECK(ambient);
	C_CHECK(hemi_color);
	C_CHECK(sun_color);
	on_device_create();
}

void CEnvDescriptor::on_device_create()
{
	if (sky_texture_name.size())
		sky_texture.create(sky_texture_name.c_str());
	if (sky_texture_env_name.size())
		sky_texture_env.create(sky_texture_env_name.c_str());
	if (clouds_texture_name.size())
		clouds_texture.create(clouds_texture_name.c_str());
	if (lut_texture_name.size())
		lut_texture.create(lut_texture_name.c_str());
}

void CEnvDescriptor::on_device_destroy()
{
	sky_texture.destroy();
	sky_texture_env.destroy();
	clouds_texture.destroy();
	lut_texture.destroy();
}

CEnvDescriptor* CEnvironment::create_descriptor(shared_str const& identifier, CInifile* config)
{
	CEnvDescriptor* result = xr_new<CEnvDescriptor>(identifier);
	if (config)
		result->load(*this, *config);
	return (result);
}