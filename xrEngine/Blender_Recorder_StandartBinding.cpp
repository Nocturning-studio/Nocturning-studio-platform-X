#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <DXSDK/d3dx9.h>
#pragma warning(pop)

#include "ResourceManager.h"
#include "blenders\Blender_Recorder.h"
#include "blenders\Blender.h"

#include "igame_persistent.h"
#include "environment.h"

// matrices
#define BIND_DECLARE(xf)                                                                                               \
	class cl_xform_##xf : public R_constant_setup                                                                      \
	{                                                                                                                  \
		virtual void setup(R_constant* C)                                                                              \
		{                                                                                                              \
			RCache.xforms.set_c_##xf(C);                                                                               \
		}                                                                                                              \
	};                                                                                                                 \
	static cl_xform_##xf binder_##xf
BIND_DECLARE(w);
BIND_DECLARE(invw);
BIND_DECLARE(v);
BIND_DECLARE(p);
BIND_DECLARE(wv);
BIND_DECLARE(vp);
BIND_DECLARE(wvp);

#define DECLARE_TREE_BIND(c)                                                                                           \
	class cl_tree_##c : public R_constant_setup                                                                        \
	{                                                                                                                  \
		virtual void setup(R_constant* C)                                                                              \
		{                                                                                                              \
			RCache.tree.set_c_##c(C);                                                                                  \
		}                                                                                                              \
	};                                                                                                                 \
	static cl_tree_##c tree_binder_##c

DECLARE_TREE_BIND(m_xform_v);
DECLARE_TREE_BIND(m_xform);
DECLARE_TREE_BIND(consts);
DECLARE_TREE_BIND(wave);
DECLARE_TREE_BIND(wind);
DECLARE_TREE_BIND(c_scale);
DECLARE_TREE_BIND(c_bias);
DECLARE_TREE_BIND(c_sun);

class cl_hemi_cube_pos_faces : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.hemi.set_c_pos_faces(C);
	}
};

static cl_hemi_cube_pos_faces binder_hemi_cube_pos_faces;

class cl_hemi_cube_neg_faces : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.hemi.set_c_neg_faces(C);
	}
};

static cl_hemi_cube_neg_faces binder_hemi_cube_neg_faces;

class cl_texgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;

		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		float o_w = (.5f / _w);
		float o_h = (.5f / _h);
		Fmatrix mTexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f,		-0.5f,		0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};

		mTexgen.mul(mTexelAdjust, RCache.xforms.m_wvp);

		RCache.set_c(C, mTexgen);
	}
};
static cl_texgen binder_texgen;

class cl_VPtexgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;

		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		float o_w = (.5f / _w);
		float o_h = (.5f / _h);
		Fmatrix mTexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f,		-0.5f,		0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f, 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};

		mTexgen.mul(mTexelAdjust, RCache.xforms.m_vp);

		RCache.set_c(C, mTexgen);
	}
};
static cl_VPtexgen binder_VPtexgen;

// fog
class cl_fog_plane : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			// Plane
			Fvector4 plane;
			Fmatrix& M = Device.mFullTransform;
			plane.x = -(M._14 + M._13);
			plane.y = -(M._24 + M._23);
			plane.z = -(M._34 + M._33);
			plane.w = -(M._44 + M._43);
			float denom = -1.0f / _sqrt(_sqr(plane.x) + _sqr(plane.y) + _sqr(plane.z));
			plane.mul(denom);

			// Near/Far
			float FarPlane = g_pGamePersistent->Environment().CurrentEnv->far_plane;
			float FogFar = FarPlane;
			float FogIntensity = g_pGamePersistent->Environment().CurrentEnv->fog_density;
			float FogNear = FogFar * (1.0f - FogIntensity);

			float Fog = 1 / (FogFar - FogNear);
			Fvector4 FogPlane;
			FogPlane.set(-plane.x * Fog, -plane.y * Fog, -plane.z * Fog, 1 - (plane.w - FogNear) * Fog);

			result.set(FogPlane);
		}
		RCache.set_c(C, result);
	}
};
static cl_fog_plane binder_fog_plane;

// fog-params
class cl_fog_params : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->fog_color.x, desc->fog_color.y, desc->fog_color.z, desc->fog_density);
		}
		RCache.set_c(C, result);
	}
};
static cl_fog_params binder_fog_params;

// fog-color
class cl_fog_color : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->fog_color.x, desc->fog_color.y, desc->fog_color.z, 0);
		}
		RCache.set_c(C, result);
	}
};
static cl_fog_color binder_fog_color;

static class cl_fog_density final : public R_constant_setup
{
	u32 marker;
	Fvector4 FogDensity;
	void setup(R_constant* C) override
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			FogDensity.set(desc->fog_density, 0, 0, 0);
		}
		RCache.set_c(C, FogDensity);
	}
} binder_fog_density;

static class cl_fog_sky_influence final : public R_constant_setup
{
	u32 marker;
	Fvector4 FogDensity;
	void setup(R_constant* C) override
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			FogDensity.set(desc->fog_sky_influence, 0, 0, 0);
		}
		RCache.set_c(C, FogDensity);
	}
} binder_fog_sky_influence;

static class cl_vertical_fog_intensity final : public R_constant_setup
{
	u32 marker;
	Fvector4 VerticalFogIntensity;
	void setup(R_constant* C) override
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			VerticalFogIntensity.set(desc->vertical_fog_intensity, 0, 0, 0);
		}
		RCache.set_c(C, VerticalFogIntensity);
	}
} binder_vertical_fog_intensity;

static class cl_vertical_fog_density final : public R_constant_setup
{
	u32 marker;
	Fvector4 VerticalFogDensity;
	void setup(R_constant* C) override
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			VerticalFogDensity.set(desc->vertical_fog_density, 0, 0, 0);
		}
		RCache.set_c(C, VerticalFogDensity);
	}
} binder_vertical_fog_density;

static class cl_vertical_fog_height final : public R_constant_setup
{
	u32 marker;
	Fvector4 VerticalFogHeight;
	void setup(R_constant* C) override
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			VerticalFogHeight.set(desc->vertical_fog_height, 0, 0, 0);
		}
		RCache.set_c(C, VerticalFogHeight);
	}
} binder_vertical_fog_height;

static class cl_rain_density : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->rain_density;
		RCache.set_c(C, fValue, fValue, fValue, 0);
	}
} binder_rain_density;

static class cl_far_plane : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->far_plane;
		RCache.set_c(C, fValue, fValue, fValue, 0);
	}
} binder_far_plane;

static class cl_sun_shafts_intensity : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->m_fSunShaftsIntensity;
		RCache.set_c(C, fValue, fValue, fValue, 0);
	}
} binder_sun_shafts_intensity;

static class cl_water_intensity : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->m_fWaterIntensity;
		RCache.set_c(C, fValue, fValue, fValue, 0);
	}
} binder_water_intensity;

static class cl_pos_decompress_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
		float HorzTan = -VertTan / Device.fASPECT;
		RCache.set_c(C, HorzTan, VertTan, VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv->far_plane);
	}
} binder_pos_decompress_params;

static class cl_sepia_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		Fvector3 SepiaColor = E->m_SepiaColor;
		float SepiaPower = E->m_SepiaPower;
		RCache.set_c(C, SepiaColor.x, SepiaColor.y, SepiaColor.z, SepiaPower);
	}
} binder_sepia_params;

static class cl_vignette_power : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->m_VignettePower;
		RCache.set_c(C, fValue, fValue, fValue, 0);
	}
} binder_vignette_power;

// times
class cl_times : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float t = Device.fTimeGlobal;
		RCache.set_c(C, t, t * 10, t / 10, _sin(t));
	}
};
static cl_times binder_times;

// eye-params
class cl_eye_P : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fvector& V = Device.vCameraPosition;
		RCache.set_c(C, V.x, V.y, V.z, 1);
	}
};
static cl_eye_P binder_eye_P;

// eye-params
class cl_eye_D : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fvector& V = Device.vCameraDirection;
		RCache.set_c(C, V.x, V.y, V.z, 0);
	}
};
static cl_eye_D binder_eye_D;

// eye-params
class cl_eye_N : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fvector& V = Device.vCameraTop;
		RCache.set_c(C, V.x, V.y, V.z, 0);
	}
};
static cl_eye_N binder_eye_N;

// D-Light0
class cl_sun0_color : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->sun_color.x, desc->sun_color.y, desc->sun_color.z, 0);
		}
		RCache.set_c(C, result);
	}
};
static cl_sun0_color binder_sun0_color;

static class cl_env_color : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
		Fvector4 envclr = {envdesc->hemi_color.x * 2 + EPS, envdesc->hemi_color.y * 2 + EPS,
						   envdesc->hemi_color.z * 2 + EPS, envdesc->weight};
		RCache.set_c(C, envclr);
	}
} binder_env_color;

class cl_sun0_dir_w : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->sun_dir.x, desc->sun_dir.y, desc->sun_dir.z, 0);
		}
		RCache.set_c(C, result);
	}
};
static cl_sun0_dir_w binder_sun0_dir_w;
class cl_sun0_dir_e : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			Fvector D;
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			Device.mView.transform_dir(D, desc->sun_dir);
			D.normalize();
			result.set(D.x, D.y, D.z, 0);
		}
		RCache.set_c(C, result);
	}
};
static cl_sun0_dir_e binder_sun0_dir_e;

//
class cl_amb_color : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptorMixer* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->ambient.x, desc->ambient.y, desc->ambient.z, desc->weight);
		}
		RCache.set_c(C, result);
	}
};
static cl_amb_color binder_amb_color;
class cl_hemi_color : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->hemi_color.x, desc->hemi_color.y, desc->hemi_color.z, desc->hemi_color.w);
		}
		RCache.set_c(C, result);
	}
};
static cl_hemi_color binder_hemi_color;

static class cl_screen_res : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, (float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth,
					 1.0f / (float)Device.dwHeight);
	}
} binder_screen_res;

static class cl_v2w final : public R_constant_setup
{
	void setup(R_constant* C) override
	{
		Fmatrix m_v2w;
		m_v2w.invert(Device.mView);
		RCache.set_c(C, m_v2w);
	}
} binder_v2w;

class cl_dwframe : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, (int)Device.dwFrame);
	}
};
static cl_dwframe binder_dwframe;

class cl_depth_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float nearPlane = float(VIEWPORT_NEAR);
		float farPlane = g_pGamePersistent->Environment().CurrentEnv->far_plane;
		//RCache.set_c(C, nearPlane, farPlane, 0, r__dbg_planar_h);
#pragma todo(fix planar height)
		RCache.set_c(C, nearPlane, farPlane, 0, -7.2f);
	}
};
static cl_depth_params binder_depth_params;

class cl_sky_color : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
		result.set(desc.sky_color.x, desc.sky_color.y, desc.sky_color.z, desc.m_fWaterIntensity);
		RCache.set_c(C, result);
	}
};
static cl_sky_color binder_sky_color;

static class cl_msaa_sample_pattern : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
#pragma todo(fix TAA constants)
		//RCache.set_c(C, CTAA::calc_jitter_x(), CTAA::calc_jitter_y(), 0, 0);
		RCache.set_c(C, 0, 0, 0, 0);
	}
} binder_msaa_sample_pattern;

// Standart constant-binding
void CBlender_Compile::SetMapping()
{
	// matrices
	r_Constant("m_W", &binder_w);
	r_Constant("m_invW", &binder_invw);
	r_Constant("m_V", &binder_v);
	r_Constant("m_P", &binder_p);
	r_Constant("m_WV", &binder_wv);
	r_Constant("m_VP", &binder_vp);
	r_Constant("m_WVP", &binder_wvp);
	r_Constant("m_v2w", &binder_v2w);

	r_Constant("m_xform_v", &tree_binder_m_xform_v);
	r_Constant("m_xform", &tree_binder_m_xform);
	r_Constant("consts", &tree_binder_consts);
	r_Constant("wave", &tree_binder_wave);
	r_Constant("wind", &tree_binder_wind);
	r_Constant("c_scale", &tree_binder_c_scale);
	r_Constant("c_bias", &tree_binder_c_bias);
	r_Constant("c_sun", &tree_binder_c_sun);

	// hemi cube
	// r_Constant("L_material", &binder_material);
	r_Constant("hemi_cube_pos_faces", &binder_hemi_cube_pos_faces);
	r_Constant("hemi_cube_neg_faces", &binder_hemi_cube_neg_faces);

	//	Igor	temp solution for the texgen functionality in the shader
	r_Constant("m_texgen", &binder_texgen);
	r_Constant("mVPTexgen", &binder_VPtexgen);

#ifndef _EDITOR
	// fog-params
	r_Constant("fog_plane", &binder_fog_plane);
	r_Constant("fog_params", &binder_fog_params);
	r_Constant("fog_color", &binder_fog_color);
	r_Constant("fog_density", &binder_fog_density);
	r_Constant("fog_sky_influence", &binder_fog_sky_influence);
	r_Constant("vertical_fog_intensity", &binder_vertical_fog_intensity);
	r_Constant("vertical_fog_density", &binder_vertical_fog_density);
	r_Constant("vertical_fog_height", &binder_vertical_fog_height);
#endif

	r_Constant("water_intensity", &binder_water_intensity);
	r_Constant("sun_shafts_intensity", &binder_sun_shafts_intensity);
	r_Constant("rain_density", &binder_rain_density);

	r_Constant("sepia_params", &binder_sepia_params);
	r_Constant("vignette_power", &binder_vignette_power);

	r_Constant("far_plane", &binder_far_plane);

	r_Constant("pos_decompression_params", &binder_pos_decompress_params);

	// env-params
	r_Constant("env_color", &binder_env_color);

	// time
	r_Constant("timers", &binder_times);

	// eye-params
	r_Constant("eye_position", &binder_eye_P);
	r_Constant("eye_direction", &binder_eye_D);
	r_Constant("eye_normal", &binder_eye_N);

#ifndef _EDITOR
	// global-lighting (env params)
	r_Constant("L_sun_color", &binder_sun0_color);
	r_Constant("L_sun_dir_w", &binder_sun0_dir_w);
	r_Constant("L_sun_dir_e", &binder_sun0_dir_e);
	//	r_Constant				("L_lmap_color",	&binder_lm_color);
	r_Constant("L_hemi_color", &binder_hemi_color);
	r_Constant("L_ambient", &binder_amb_color);
#endif

	r_Constant("screen_res", &binder_screen_res);

	if (bDetail && bDetail_Diffuse && detail_scaler)
		r_Constant("dt_params", detail_scaler);

	r_Constant("dwframe", &binder_dwframe);
	r_Constant("depth_params", &binder_depth_params);
	r_Constant("wea_sky_color", &binder_sky_color);
	r_Constant("taa_jitter", &binder_msaa_sample_pattern);

	// other common
	for (u32 it = 0; it < Device.Resources->v_constant_setup.size(); it++)
	{
		std::pair<shared_str, R_constant_setup*> cs = Device.Resources->v_constant_setup[it];
		r_Constant(*cs.first, cs.second);
	}
}
