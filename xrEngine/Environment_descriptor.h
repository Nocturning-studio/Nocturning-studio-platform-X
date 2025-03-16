#pragma once

class ENGINE_API CEnvDescriptor
{
  public:
	float exec_time;
	float exec_time_loaded;

	shared_str sky_texture_name;
	shared_str sky_texture_env_name;
	shared_str clouds_texture_name;
	shared_str lut_texture_name;

	ref_texture sky_texture;
	ref_texture sky_texture_env;
	ref_texture clouds_texture;
	ref_texture lut_texture;

	Fvector4 clouds_color;
	Fvector3 sky_color;
	float sky_rotation;

	float far_plane;

	Fvector3 fog_color;
	float fog_density;
	float fog_sky_influence;
	float vertical_fog_intensity;
	float vertical_fog_density;
	float vertical_fog_height;

	float rain_density;
	Fvector3 rain_color;

	float bolt_period;
	float bolt_duration;

	float wind_velocity;
	float wind_direction;

	Fvector3 ambient;
	Fvector4 hemi_color; // w = R2 correction
	Fvector3 sun_color;
	Fvector3 sun_dir;
	float m_fSunShaftsIntensity;
	float m_fWaterIntensity;

	shared_str lens_flare_id;
	shared_str tb_id;

	// SkyLoader: trees wave
	float m_fTreeAmplitude;
	float m_fTreeSpeed;
	float m_fTreeRotation;
	Fvector3 m_fTreeWave;

	// NSDeathman(Очередное ЧСВ в коде указало свой ник): Кинематографические инструменты
	Fvector3 m_SepiaColor;
	float m_SepiaPower;
	float m_VignettePower;

	CEnvAmbient* env_ambient;

	CEnvDescriptor(shared_str const& identifier);

	void load(CEnvironment& environment, CInifile& config);
	void copy(const CEnvDescriptor& src)
	{
		float tm0 = exec_time;
		float tm1 = exec_time_loaded;
		*this = src;
		exec_time = tm0;
		exec_time_loaded = tm1;
	}

	void on_device_create();
	void on_device_destroy();

	shared_str m_identifier;
};