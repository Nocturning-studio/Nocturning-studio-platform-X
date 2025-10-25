#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(pop)

#include "ResourceManager.h"
#include "blenders\Blender_Recorder.h"
#include "blenders\Blender.h"

#include "../xrRender/r_color_converting.h"

#include "igame_persistent.h"
#include "environment.h"

// matrices
#define BIND_DECLARE(xf)                                                                                               \
	class cl_xform_##xf : public R_constant_setup                                                                      \
	{                                                                                                                  \
		virtual void setup(R_constant* C)                                                                              \
		{                                                                                                              \
			RenderBackend.xforms.set_c_##xf(C);                                                                               \
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
			RenderBackend.tree.set_c_##c(C);                                                                                  \
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

class cl_invV : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mInvV = Fmatrix().invert(RenderBackend.xforms.m_v);

		RenderBackend.set_Constant(C, mInvV);
	}
};
static cl_invV binder_invv;

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

		mTexgen.mul(mTexelAdjust, RenderBackend.xforms.m_wvp);

		RenderBackend.set_Constant(C, mTexgen);
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

		mTexgen.mul(mTexelAdjust, RenderBackend.xforms.m_vp);

		RenderBackend.set_Constant(C, mTexgen);
	}
};
static cl_VPtexgen binder_VPtexgen;

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
			result.set(sRgbToLinear(desc->fog_color.x), sRgbToLinear(desc->fog_color.y), sRgbToLinear(desc->fog_color.z),
					   desc->fog_density);
		}
		RenderBackend.set_Constant(C, result);
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
			result.set(sRgbToLinear(desc->fog_color.x), sRgbToLinear(desc->fog_color.y), sRgbToLinear(desc->fog_color.z), 0);
		}
		RenderBackend.set_Constant(C, result);
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
		RenderBackend.set_Constant(C, FogDensity);
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
		RenderBackend.set_Constant(C, FogDensity);
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
		RenderBackend.set_Constant(C, VerticalFogIntensity);
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
		RenderBackend.set_Constant(C, VerticalFogDensity);
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
		RenderBackend.set_Constant(C, VerticalFogHeight);
	}
} binder_vertical_fog_height;

static class cl_rain_density : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->rain_density;
		RenderBackend.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_rain_density;

static class cl_far_plane : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->far_plane;
		RenderBackend.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_far_plane;

static class cl_water_intensity : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->m_fWaterIntensity;
		RenderBackend.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_water_intensity;

static class cl_pos_decompress_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float VertTan = -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
		float HorzTan = -VertTan / Device.fASPECT;
		RenderBackend.set_Constant(C, HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
	}
} binder_pos_decompress_params;

extern ENGINE_API float psHUD_FOV;
static class cl_pos_decompress_params_hud : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float VertTan = -1.0f * tanf(deg2rad(psHUD_FOV / 2.0f));
		float HorzTan = -VertTan / Device.fASPECT;

		RenderBackend.set_Constant(C, HorzTan, VertTan, (2.0f * HorzTan) / Device.dwWidth, (2.0f * VertTan) / Device.dwHeight);
	}
} binder_pos_decompress_params_hud;

static class cl_fov : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackend.set_Constant(C, Device.fFOV, 0, 0, 0);
	}
} binder_fov;

static class cl_sepia_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		Fvector3 SepiaColor = E->m_SepiaColor;
		float SepiaPower = E->m_SepiaPower;
		RenderBackend.set_Constant(C, sRgbToLinear(SepiaColor.x), sRgbToLinear(SepiaColor.y), sRgbToLinear(SepiaColor.z), SepiaPower);
	}
} binder_sepia_params;

static class cl_vignette_power : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* E = g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E->m_VignettePower;
		RenderBackend.set_Constant(C, fValue, fValue, fValue, 0);
	}
} binder_vignette_power;

// times
class cl_times : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float t = Device.fTimeGlobal;
		RenderBackend.set_Constant(C, t, t * 10, t / 10, _sin(t));
	}
};
static cl_times binder_times;

// eye-params
class cl_eye_P : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fvector& V = Device.vCameraPosition;
		RenderBackend.set_Constant(C, V.x, V.y, V.z, 1);
	}
};
static cl_eye_P binder_eye_P;

// eye-params
class cl_eye_D : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fvector& V = Device.vCameraDirection;
		RenderBackend.set_Constant(C, V.x, V.y, V.z, 0);
	}
};
static cl_eye_D binder_eye_D;

// eye-params
class cl_eye_N : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fvector& V = Device.vCameraTop;
		RenderBackend.set_Constant(C, V.x, V.y, V.z, 0);
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
			result.set(sRgbToLinear(desc->sun_color.x), sRgbToLinear(desc->sun_color.y), sRgbToLinear(desc->sun_color.z), 0);
		}
		RenderBackend.set_Constant(C, result);
	}
};
static cl_sun0_color binder_sun0_color;

static class cl_env_color : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptorMixer* envdesc = g_pGamePersistent->Environment().CurrentEnv;
		Fvector4 envclr = {sRgbToLinear(envdesc->hemi_color.x) * 2 + EPS,sRgbToLinear( envdesc->hemi_color.y) * 2 + EPS,
						   sRgbToLinear(envdesc->hemi_color.z) * 2 + EPS, envdesc->weight};
		RenderBackend.set_Constant(C, envclr);
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
		RenderBackend.set_Constant(C, result);
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
		RenderBackend.set_Constant(C, result);
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
			result.set(sRgbToLinear(desc->ambient.x), sRgbToLinear(desc->ambient.y), sRgbToLinear(desc->ambient.z), desc->weight);
		}
		RenderBackend.set_Constant(C, result);
	}
};
static cl_amb_color binder_amb_color;

class cl_ambient_brightness : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptorMixer* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(desc->ambient_brightness, 0, 0, 0);
		}
		RenderBackend.set_Constant(C, result);
	}
};
static cl_ambient_brightness binder_ambient_brightness;

class cl_hemi_color : public R_constant_setup
{
	u32 marker;
	Fvector4 result;
	virtual void setup(R_constant* C)
	{
		if (marker != Device.dwFrame)
		{
			CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
			result.set(sRgbToLinear(desc->hemi_color.x), sRgbToLinear(desc->hemi_color.y), sRgbToLinear(desc->hemi_color.z), desc->hemi_color.w);
		}
		RenderBackend.set_Constant(C, result);
	}
};
static cl_hemi_color binder_hemi_color;

static class cl_screen_res : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RenderBackend.set_Constant(C, (float)Device.dwWidth, (float)Device.dwHeight, 1.0f / (float)Device.dwWidth, 1.0f / (float)Device.dwHeight);
	}
} binder_screen_res;

static class cl_invP final : public R_constant_setup
{
	void setup(R_constant* C) override
	{
		Fmatrix m_invProject;
		m_invProject.invert(Device.mProject);
		RenderBackend.set_Constant(C, m_invProject);
	}
} binder_invP;

class cl_wind_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
		float WindGusting = desc->wind_gusting;
		float WindDirectionX = cos(desc->wind_direction), WindDirectionY = sin(desc->wind_direction);
		float WindStrength = desc->wind_strength;
		RenderBackend.set_Constant(C, WindDirectionX, WindDirectionY, WindGusting, WindStrength);
	}
};
static cl_wind_params binder_wind_params;

class cl_wind_turbulence : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
		float Turbulence = desc->wind_turbulence;
		float TurbulencePacked = std::max(std::min(Turbulence, 0.0f), 1.0f);
		float WindSpeed = Device.fTimeGlobal * 2.5f * (1.0f + TurbulencePacked);
		RenderBackend.set_Constant(C, Turbulence, TurbulencePacked, WindSpeed, 0);
	}
};
static cl_wind_turbulence binder_wind_turbulence;

// Standart constant-binding
void CBlender_Compile::SetMapping()
{
	// matrices
	r_Constant("m_World", &binder_w);
	r_Constant("m_invWorld", &binder_invw);
	r_Constant("m_View", &binder_v);
	r_Constant("m_invView", &binder_invv);
	r_Constant("m_Project", &binder_p);
	r_Constant("m_invProject", &binder_invP);
	r_Constant("m_WorldView", &binder_wv);
	r_Constant("m_ViewProject", &binder_vp);
	r_Constant("m_WorldViewProject", &binder_wvp);

	r_Constant("m_xform_v", &tree_binder_m_xform_v);
	r_Constant("m_xform", &tree_binder_m_xform);
	r_Constant("consts", &tree_binder_consts);
	r_Constant("wave", &tree_binder_wave);
	r_Constant("wind", &tree_binder_wind);
	r_Constant("c_scale", &tree_binder_c_scale);
	r_Constant("c_bias", &tree_binder_c_bias);
	r_Constant("c_sun", &tree_binder_c_sun);
	r_Constant("wind_params", &binder_wind_params);
	r_Constant("wind_turbulence", &binder_wind_turbulence);

	//	Igor	temp solution for the texgen functionality in the shader
	r_Constant("m_texgen", &binder_texgen);
	r_Constant("mVPTexgen", &binder_VPtexgen);

#ifndef _EDITOR
	// fog-params
	r_Constant("fog_params", &binder_fog_params);
	r_Constant("fog_color", &binder_fog_color);
	r_Constant("fog_density", &binder_fog_density);
	r_Constant("fog_sky_influence", &binder_fog_sky_influence);
	r_Constant("vertical_fog_intensity", &binder_vertical_fog_intensity);
	r_Constant("vertical_fog_density", &binder_vertical_fog_density);
	r_Constant("vertical_fog_height", &binder_vertical_fog_height);
#endif

	r_Constant("water_intensity", &binder_water_intensity);
	r_Constant("rain_density", &binder_rain_density);

	r_Constant("sepia_params", &binder_sepia_params);
	r_Constant("vignette_power", &binder_vignette_power);

	r_Constant("far_plane", &binder_far_plane);

	r_Constant("pos_decompression_params", &binder_pos_decompress_params);
	r_Constant("pos_decompression_params_hud", &binder_pos_decompress_params_hud);

	r_Constant("fov", &binder_fov);
	
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
	r_Constant("L_hemi_color", &binder_hemi_color);
	r_Constant("L_ambient", &binder_amb_color);
	r_Constant("ambient_brightness", &binder_ambient_brightness);
#endif

	r_Constant("screen_res", &binder_screen_res);

	if (bDetail && bDetail_Diffuse && detail_scaler)
		r_Constant("dt_params", detail_scaler);

	// other common
	for (u32 it = 0; it < Device.Resources->v_constant_setup.size(); it++)
	{
		std::pair<shared_str, R_constant_setup*> cs = Device.Resources->v_constant_setup[it];
		r_Constant(*cs.first, cs.second);
	}
}
