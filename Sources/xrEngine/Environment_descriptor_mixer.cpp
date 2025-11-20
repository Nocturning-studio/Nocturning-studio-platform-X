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
	sky_r_textures_irradiance.clear();
	lut_r_textures.clear();
	clouds_r_textures.clear();

	on_device_destroy();

	sky_texture.destroy();
	sky_texture_irradiance.destroy();
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

	sky_r_textures_irradiance.clear();
	sky_r_textures_irradiance.push_back(zero);
	sky_r_textures_irradiance.push_back(zero);
	sky_r_textures_irradiance.push_back(zero);

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

void CEnvDescriptorMixer::lerp( CEnvironment* env, 
								CEnvDescriptor& A, 
								CEnvDescriptor& B, 
								float f, 
								CEnvModifier& Mdf,
							    float modifier_power )
{
	// Макрос для интерполяции обычных чисел (float)
	// Разворачивается в: variable = (1-f)*A.variable + f*B.variable
#define LERP(v) v = fi * A.v + f * B.v

	// Макрос для интерполяции векторов (Fvector3, Fvector4, Fcolor)
	// Разворачивается в: variable.lerp(A.variable, B.variable, f)
#define LERP_VEC(v) v.lerp(A.v, B.v, f)

	float modif_power = 1.f / (modifier_power + 1); // the environment itself
	float fi = 1 - f;

	// -------------------------------------------------------------------------
	// Текстуры
	// -------------------------------------------------------------------------
	sky_r_textures.clear();
	sky_r_textures.push_back(mk_pair(0, A.sky_texture));
	sky_r_textures.push_back(mk_pair(1, B.sky_texture));

	sky_r_textures_irradiance.clear();
	sky_r_textures_irradiance.push_back(mk_pair(0, A.sky_texture_irradiance));
	sky_r_textures_irradiance.push_back(mk_pair(1, B.sky_texture_irradiance));

	clouds_r_textures.clear();
	clouds_r_textures.push_back(mk_pair(0, A.clouds_texture));
	clouds_r_textures.push_back(mk_pair(1, B.clouds_texture));

	lut_r_textures.clear();
	lut_r_textures.push_back(mk_pair(0, A.lut_texture));
	lut_r_textures.push_back(mk_pair(1, B.lut_texture));

	// -------------------------------------------------------------------------
	// Интерполяция параметров
	// -------------------------------------------------------------------------
	weight = f;

	LERP_VEC(clouds_color);
	LERP(sky_rotation);

	LERP(far_plane);
	if (Mdf.use_flags.test(eViewDist))
		far_plane *= psVisDistance* modif_power;
	else
		far_plane *= psVisDistance;

	// Fog Color
	LERP_VEC(fog_color);
	if (Mdf.use_flags.test(eFogColor))
		fog_color.add(Mdf.fog_color).mul(modif_power);

	// Fog Density
	LERP(fog_density);
	if (Mdf.use_flags.test(eFogDensity))
	{
		fog_density += Mdf.fog_density;
		fog_density *= modif_power;
	}

	LERP(fog_sky_influence);

	// Vertical Fog
	LERP(vertical_fog_intensity);
	LERP(vertical_fog_density);
	LERP(vertical_fog_height);

	// Rain & Bolt
	LERP(rain_density);
	LERP_VEC(rain_color);
	LERP(bolt_period);
	LERP(bolt_duration);

	// Wind (Здесь же добавьте ваши новые параметры из предыдущего шага, если нужно)
	LERP(wind_strength);
	LERP(wind_direction);
	LERP(wind_gusting);
	LERP(wind_tilt);
	LERP(wind_velocity);

	// Trees
	LERP(m_fTreeAmplitude);
	LERP(m_fTreeSpeed);
	LERP(m_fTreeRotation);
	LERP_VEC(m_fTreeWave);

	// Sun Shafts & Water
	LERP(m_fSunShaftsIntensity);
	LERP(m_fWaterIntensity);

	// Screen Effects
	LERP_VEC(m_SepiaColor);
	LERP(m_SepiaPower);
	LERP(m_VignettePower);

	// -------------------------------------------------------------------------
	// Цвета с учетом модификаторов
	// -------------------------------------------------------------------------

	// Sky Color
	LERP_VEC(sky_color);
	if (Mdf.use_flags.test(eSkyColor))
		sky_color.add(Mdf.sky_color).mul(modif_power);

	// Ambient
	LERP_VEC(ambient);
	if (Mdf.use_flags.test(eAmbientColor))
		ambient.add(Mdf.ambient).mul(modif_power);

	// Hemi
	LERP_VEC(hemi_color);
	if (Mdf.use_flags.test(eHemiColor))
	{
		hemi_color.x += Mdf.hemi_color.x;
		hemi_color.y += Mdf.hemi_color.y;
		hemi_color.z += Mdf.hemi_color.z;
		hemi_color.mul(modif_power); // Оптимизировал: умножение всего вектора сразу
	}

	LERP(ambient_brightness);
	LERP_VEC(sun_color);

	// Sun Direction (Требует нормализации, макрос не подходит)
	R_ASSERT(_valid(A.sun_dir) && _valid(B.sun_dir));
	sun_dir.lerp(A.sun_dir, B.sun_dir, f).normalize();
	VERIFY2(sun_dir.y < 0, "Invalid sun direction settings while lerp");

	// Удаляем макросы, чтобы не мусорить в глобальной области видимости
#undef LERP
#undef LERP_VEC
}
