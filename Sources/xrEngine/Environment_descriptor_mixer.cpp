//-----------------------------------------------------------------------------
// Environment Mixer
//-----------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "../xrGame/LevelGameDef.h"

CEnvDescriptorMixer::CEnvDescriptorMixer(shared_str const& identifier) : CEnvDescriptor(identifier)
{
}

void CEnvDescriptorMixer::destroy()
{
	sky_r_textures.clear();
	sky_r_textures_env.clear();
	lut_r_textures.clear();
	clouds_r_textures.clear();

	on_device_destroy();

	sky_texture.destroy();
	sky_texture_env.destroy();
	clouds_texture.destroy();
	lut_texture.destroy();
}

void CEnvDescriptorMixer::clear()
{
	std::pair<u32, ref_texture> zero = mk_pair(u32(0), ref_texture(0));
	sky_r_textures.clear();
	sky_r_textures.push_back(zero);
	sky_r_textures.push_back(zero);
	sky_r_textures.push_back(zero);

	sky_r_textures_env.clear();
	sky_r_textures_env.push_back(zero);
	sky_r_textures_env.push_back(zero);
	sky_r_textures_env.push_back(zero);

	clouds_r_textures.clear();
	clouds_r_textures.push_back(zero);
	clouds_r_textures.push_back(zero);
	clouds_r_textures.push_back(zero);

	lut_r_textures.clear();
	lut_r_textures.push_back(zero);
	lut_r_textures.push_back(zero);
	lut_r_textures.push_back(zero);
}

int get_ref_count(IUnknown* ii);

void CEnvDescriptorMixer::lerp(	CEnvironment*, 
								CEnvDescriptor& A, 
								CEnvDescriptor& B, 
								float f, 
								CEnvModifier& Mdf,
								float modifier_power )
{
	float modif_power = 1.f / (modifier_power + 1); // the environment itself
	float fi = 1 - f;

	sky_r_textures.clear();
	sky_r_textures.push_back(mk_pair(0, A.sky_texture));
	sky_r_textures.push_back(mk_pair(1, B.sky_texture));

	sky_r_textures_env.clear();

	sky_r_textures_env.push_back(mk_pair(0, A.sky_texture_env));
	sky_r_textures_env.push_back(mk_pair(1, B.sky_texture_env));

	clouds_r_textures.clear();
	clouds_r_textures.push_back(mk_pair(0, A.clouds_texture));
	clouds_r_textures.push_back(mk_pair(1, B.clouds_texture));

	lut_r_textures.clear();
	lut_r_textures.push_back(mk_pair(0, A.lut_texture));
	lut_r_textures.push_back(mk_pair(1, B.lut_texture));

	weight = f;

	clouds_color.lerp(A.clouds_color, B.clouds_color, f);

	sky_rotation = (fi * A.sky_rotation + f * B.sky_rotation);

	if (Mdf.use_flags.test(eViewDist))
		far_plane = (fi * A.far_plane + f * B.far_plane + Mdf.far_plane) * psVisDistance * modif_power;
	else
		far_plane = (fi * A.far_plane + f * B.far_plane) * psVisDistance;

	fog_color.lerp(A.fog_color, B.fog_color, f);
	if (Mdf.use_flags.test(eFogColor))
		fog_color.add(Mdf.fog_color).mul(modif_power);

	fog_density = (fi * A.fog_density + f * B.fog_density);
	if (Mdf.use_flags.test(eFogDensity))
	{
		fog_density += Mdf.fog_density;
		fog_density *= modif_power;
	}
	fog_sky_influence = (fi * A.fog_sky_influence + f * B.fog_sky_influence);

	vertical_fog_intensity = (fi * A.vertical_fog_intensity + f * B.vertical_fog_intensity);
	vertical_fog_density = (fi * A.vertical_fog_density + f * B.vertical_fog_density);
	vertical_fog_height = (fi * A.vertical_fog_height + f * B.vertical_fog_height);

	rain_density = fi * A.rain_density + f * B.rain_density;
	rain_color.lerp(A.rain_color, B.rain_color, f);
	bolt_period = fi * A.bolt_period + f * B.bolt_period;
	bolt_duration = fi * A.bolt_duration + f * B.bolt_duration;

	// wind
	wind_strength = fi * A.wind_strength + f * B.wind_strength;
	wind_direction = fi * A.wind_direction + f * B.wind_direction;
	wind_gusting = fi * A.wind_gusting + f * B.wind_gusting;

	// trees
	m_fTreeAmplitude = fi * A.m_fTreeAmplitude + f * B.m_fTreeAmplitude;
	m_fTreeSpeed = fi * A.m_fTreeSpeed + f * B.m_fTreeSpeed;
	m_fTreeRotation = fi * A.m_fTreeRotation + f * B.m_fTreeRotation;
	m_fTreeWave.lerp(A.m_fTreeWave, B.m_fTreeWave, f);

	m_fSunShaftsIntensity = fi * A.m_fSunShaftsIntensity + f * B.m_fSunShaftsIntensity;
	m_fWaterIntensity = fi * A.m_fWaterIntensity + f * B.m_fWaterIntensity;

	m_SepiaColor.lerp(A.m_SepiaColor, B.m_SepiaColor, f);
	m_SepiaPower = fi * A.m_SepiaPower + f * B.m_SepiaPower;
	m_VignettePower = fi * A.m_VignettePower + f * B.m_VignettePower;

	// colors
	sky_color.lerp(A.sky_color, B.sky_color, f);
	if (Mdf.use_flags.test(eSkyColor))
		sky_color.add(Mdf.sky_color).mul(modif_power);

	ambient.lerp(A.ambient, B.ambient, f);
	if (Mdf.use_flags.test(eAmbientColor))
		ambient.add(Mdf.ambient).mul(modif_power);

	hemi_color.lerp(A.hemi_color, B.hemi_color, f);

	ambient_brightness = fi * A.ambient_brightness + f * B.ambient_brightness;

	if (Mdf.use_flags.test(eHemiColor))
	{
		hemi_color.x += Mdf.hemi_color.x;
		hemi_color.y += Mdf.hemi_color.y;
		hemi_color.z += Mdf.hemi_color.z;
		hemi_color.x *= modif_power;
		hemi_color.y *= modif_power;
		hemi_color.z *= modif_power;
	}

	sun_color.lerp(A.sun_color, B.sun_color, f);

	R_ASSERT(_valid(A.sun_dir));
	R_ASSERT(_valid(B.sun_dir));
	sun_dir.lerp(A.sun_dir, B.sun_dir, f).normalize();
	R_ASSERT(_valid(sun_dir));

	VERIFY2(sun_dir.y < 0, "Invalid sun direction settings while lerp");
}