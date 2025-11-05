#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#include "Blender_Recorder.h"
#include "Blender.h"

#include "../xrEngine/igame_persistent.h"
#include "../xrEngine/environment.h"

#include "dxRenderDeviceRender.h"

#include "../xrEngine/Rain.h"

// matrices
class cl_xform_p : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.xforms.set_c_p(C);
	}
};
static cl_xform_p binder_p;

class cl_xform_w : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
		RCache.xforms.set_c_w(C); 
	} 
};
static cl_xform_w	binder_w;

class cl_xform_invw : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
		RCache.xforms.set_c_invw(C);
	} 
};
static cl_xform_invw	binder_invw;

class cl_xform_v : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
		RCache.xforms.set_c_v(C);
	} 
};
static cl_xform_v	binder_v;

class cl_xform_wv : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
		RCache.xforms.set_c_wv(C);
	} 
};
static cl_xform_wv	binder_wv;

class cl_xform_vp : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
		RCache.xforms.set_c_vp(C);
	} 
};
static cl_xform_vp	binder_vp;

class cl_xform_wvp : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
		RCache.xforms.set_c_wvp(C);
	} 
};
static cl_xform_wvp	binder_wvp;

class cl_xform_tp : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
//		RCache.xforms_taa.set_c_p(C);
	}
};
static cl_xform_tp binder_tp;

class cl_xform_tw : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
//		RCache.xforms_taa.set_c_w(C);
	} 
};
static cl_xform_tw	binder_tw;

class cl_xform_tinvw : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
//		RCache.xforms_taa.set_c_invw(C);
	} 
};
static cl_xform_tinvw	binder_tinvw;

class cl_xform_tv : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
//		RCache.xforms_taa.set_c_v(C);
	} 
};
static cl_xform_tv	binder_tv;

class cl_xform_twv : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
//		RCache.xforms_taa.set_c_wv(C);
	} 
};
static cl_xform_twv	binder_twv;

class cl_xform_tvp : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
//		RCache.xforms_taa.set_c_vp(C);
	} 
};
static cl_xform_tvp	binder_tvp;

class cl_xform_twvp : public R_constant_setup
{ 
	virtual void setup(R_constant * C) 
	{ 
//		RCache.xforms_taa.set_c_wvp(C);
	} 
};
static cl_xform_twvp	binder_twvp;

class cl_tree_m_xform_v : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_m_xform_v(C);
	}
};
static cl_tree_m_xform_v	tree_binder_m_xform_v;

class cl_tree_m_xform : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_m_xform(C);
	}
};
static cl_tree_m_xform	tree_binder_m_xform;

class cl_tree_consts : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_consts(C);
	}
};
static cl_tree_consts	tree_binder_consts;

class cl_tree_c_sun : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_c_sun(C);
	}
};
static cl_tree_c_sun	tree_binder_c_sun;

class cl_tree_c_bias : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_c_bias(C);
	}
};
static cl_tree_c_bias	tree_binder_c_bias;

class cl_tree_c_scale : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_c_scale(C);
	}
};
static cl_tree_c_scale	tree_binder_c_scale;

class cl_tree_wave : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_wave(C);
	}
};
static cl_tree_wave	tree_binder_wave;

class cl_tree_wind : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.tree.set_c_wind(C);
	}
};
static cl_tree_wind	tree_binder_wind;

class cl_hemi_cube_pos_faces: public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.hemi.set_c_pos_faces(C);
	}
};

static cl_hemi_cube_pos_faces binder_hemi_cube_pos_faces;

class cl_hemi_cube_neg_faces: public R_constant_setup
{
	virtual void setup(R_constant* C) 
	{
		RCache.hemi.set_c_neg_faces(C);
	}
};

static cl_hemi_cube_neg_faces binder_hemi_cube_neg_faces;

class cl_material: public R_constant_setup
{
	virtual void setup(R_constant* C) 
	{
		RCache.hemi.set_c_material(C);
	}
};

static cl_material binder_material;

class cl_texgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;


		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f,				0.5f,				0.0f,			1.0f
		};


		mTexgen.mul	(mTexelAdjust,RCache.get_xform_wvp());

		RCache.set_c( C, mTexgen);
	}
};
static cl_texgen		binder_texgen;

class cl_VPtexgen : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		Fmatrix mTexgen;


		Fmatrix			mTexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				1.0f,			0.0f,
			0.5f,				0.5f,				0.0f,			1.0f
		};

		mTexgen.mul	(mTexelAdjust,RCache.get_xform_vp());

		RCache.set_c( C, mTexgen);
	}
};
static cl_VPtexgen		binder_VPtexgen;

class cl_m_v2w : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
	Fmatrix		m_v2w;			m_v2w.invert				(Device.mView		);
	RCache.set_c				(C,				m_v2w	);
	}
};
static cl_m_v2w binder_m_v2w;

class cl_m_w2v : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
	Fmatrix m_w2v;
	m_w2v.set(Device.mView);
	RCache.set_c(C, m_w2v);
	}
};
static cl_m_w2v binder_m_w2v;

class cl_m_p2v : public R_constant_setup
{
	virtual void setup(R_constant* C) 
	{
		Fmatrix m_p2v; m_p2v.invert(Device.mProject);
		RCache.set_c(C, m_p2v);
	}
};
static cl_m_p2v binder_m_p2v;

class cl_rain_params : public R_constant_setup 
{
	virtual void setup(R_constant* C)
	{
		float wetness_accum	= g_pGamePersistent->Environment().wetness_accum;
		float rain_density	= g_pGamePersistent->Environment().CurrentEnv->rain_density;
		//float wetness_accum = rain_density;
#ifdef DEBUG
		Log("Wetness accumulator:", wetness_accum);
#endif
		RCache.set_c (C, wetness_accum, rain_density, 0, 0);
	}
};
static cl_rain_params binder_rain_params;

class cl_depth_params : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float nearPlane = float(VIEWPORT_NEAR);
		float farPlane = g_pGamePersistent->Environment().CurrentEnv->far_plane;
		RCache.set_c(C, nearPlane, farPlane, 0, r__dbg_planar_h);
	}
};
static cl_depth_params binder_depth_params;

class cl_dwframe : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, (int)Device.dwFrame);
	}
};
static cl_dwframe binder_dwframe;

class cl_sky_color	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	
	{
		CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
		result.set				(desc.sky_color.x,	desc.sky_color.y, desc.sky_color.z,	desc.m_fWaterIntensity);
		RCache.set_c	(C,result);
	}
};	static cl_sky_color		binder_sky_color;
// fog
#ifndef _EDITOR
class cl_fog_plane	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C)
	{
		if (marker!=Device.dwFrame)
		{
			// Plane
			Fvector4		plane;
			Fmatrix&	M	= Device.mFullTransform;
			plane.x			= -(M._14 + M._13);
			plane.y			= -(M._24 + M._23);
			plane.z			= -(M._34 + M._33);
			plane.w			= -(M._44 + M._43);
			float denom		= -1.0f / _sqrt(_sqr(plane.x)+_sqr(plane.y)+_sqr(plane.z));
			plane.mul		(denom);

			// Near/Far
			float A			= g_pGamePersistent->Environment().CurrentEnv->fog_near;
			float B			= 1/(g_pGamePersistent->Environment().CurrentEnv->fog_far - A);
			result.set		(-plane.x*B, -plane.y*B, -plane.z*B, 1 - (plane.w-A)*B	);								// view-plane
		}
		RCache.set_c	(C,result);
	}
};
static cl_fog_plane		binder_fog_plane;

// fog-params
class cl_fog_params	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup(R_constant* C)
	{
		if (marker!=Device.dwFrame)
		{
			// Near/Far
			float	n		= g_pGamePersistent->Environment().CurrentEnv->fog_near	;
			float	f		= g_pGamePersistent->Environment().CurrentEnv->fog_far	;
			float	r		= 1/(f-n);

			float   dist    = g_pGamePersistent->Environment().CurrentEnv->fog_distance;
			float   dens    = g_pGamePersistent->Environment().CurrentEnv->fog_density;

			// Set out
			result.set		(-n*r, dist, dens, r);
		}
		RCache.set_c	(C,result);
	}
};	static cl_fog_params	binder_fog_params;

// fog-color
class cl_fog_color	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.fog_color.x,	desc.fog_color.y, desc.fog_color.z,	0);
		}
		RCache.set_c	(C,result);
	}
};	static cl_fog_color		binder_fog_color;

#endif
//wea_sky_color
// times
class cl_times		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		static float _t = 0;
		float t	= RDEVICE.fTimeGlobal;
		RCache.set_c(C, t, _sin(t), _t, _sin(_t));
		_t = t;
	}
};
static cl_times		binder_times;

// eye-params
class cl_eye_P		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= RDEVICE.vCameraPosition;
		RCache.set_c	(C,V.x,V.y,V.z,1);
	}
};
static cl_eye_P		binder_eye_P;

// matrix inv
// eye-params
class cl_eye_D		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= RDEVICE.vCameraDirection;
		RCache.set_c	(C,V.x,V.y,V.z,0);
	}
};
static cl_eye_D		binder_eye_D;

// eye-params
class cl_eye_N		: public R_constant_setup {
	virtual void setup(R_constant* C)
	{
		Fvector&		V	= RDEVICE.vCameraTop;
		RCache.set_c	(C,V.x,V.y,V.z,0);
	}
};
static cl_eye_N		binder_eye_N;

#ifndef _EDITOR
// D-Light0
class cl_sun0_color	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.sun_color.x,	desc.sun_color.y, desc.sun_color.z,	0);
		}
		RCache.set_c	(C,result);
	}
};	static cl_sun0_color		binder_sun0_color;
class cl_sun0_dir_w	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.sun_dir.x,	desc.sun_dir.y, desc.sun_dir.z,	0);
		}
		RCache.set_c	(C,result);
	}
};	static cl_sun0_dir_w		binder_sun0_dir_w;

//---------------------------------------------
class cl_fov_test	: public R_constant_setup {
	virtual void setup	(R_constant* C)	
	{
		RCache.set_c	(C,float(Device.fFOV),float(Device.fASPECT),0,0);
	}
};	static cl_fov_test		binder_fov_test;
//---------------------------------------------
class cl_ss_dec_param1	: public R_constant_setup {
	virtual void setup	(R_constant* C)	
	{
		CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
		RCache.set_c	(C, float(E.m_fSunShaftsIntensity), 0, 0, 0);
	}
};	static cl_ss_dec_param1		binder_ss_dec_param1;

class cl_sun0_dir_e	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			Fvector D;
			CEnvDescriptor&	desc		= *g_pGamePersistent->Environment().CurrentEnv;
			Device.mView.transform_dir	(D,desc.sun_dir);
			D.normalize					();
			result.set					(D.x,D.y,D.z,0);
		}
		RCache.set_c	(C,result);
	}
};	static cl_sun0_dir_e		binder_sun0_dir_e;


//
class cl_amb_color	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptorMixer&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.ambient.x, desc.ambient.y, desc.ambient.z, desc.weight);
		}
		RCache.set_c	(C,result);
	}
};	static cl_amb_color		binder_amb_color;
class cl_hemi_color	: public R_constant_setup {
	u32			marker;
	Fvector4	result;
	virtual void setup	(R_constant* C)	{
		if (marker!=Device.dwFrame)	{
			CEnvDescriptor&	desc	= *g_pGamePersistent->Environment().CurrentEnv;
			result.set				(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z, desc.hemi_color.w);
		}
		RCache.set_c	(C,result);
	}
};	static cl_hemi_color		binder_hemi_color;
#endif

static class cl_developer_float4 : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		RCache.set_c(C, r__free_vector);
	}
}	binder_developer_float4;

static class cl_msaa_sample_pattern : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		if (RImplementation.o.txaa || RImplementation.o.aa_mode == AA_TAA || RImplementation.o.aa_mode == AA_TAA_V2)
			RCache.set_c(C, CTAA::calc_jitter_x(), CTAA::calc_jitter_y(), 0, 0);
		else
			RCache.set_c(C, 0, 0, 0, 0);
	}
}	binder_msaa_sample_pattern;

// Standart constant-binding
void	CBlender_Compile::SetMapping	()
{
	// matrices
	r_Constant				("m_W",				&binder_w);
	r_Constant				("m_invW",			&binder_invw);
	r_Constant				("m_V",				&binder_v);
	r_Constant				("m_P",				&binder_p);
	r_Constant				("m_WV",			&binder_wv);
	r_Constant				("m_VP",			&binder_vp);
	r_Constant				("m_WVP",			&binder_wvp);
	
	//r_Constant				("m_tW",			&binder_tw);
	//r_Constant				("m_tinvW",			&binder_tinvw);
	//r_Constant				("m_tV",			&binder_tv);
	//r_Constant				("m_tP",			&binder_tp);
	//r_Constant				("m_tWV",			&binder_twv);
	//r_Constant				("m_tVP",			&binder_tvp);
	//r_Constant				("m_tWVP",			&binder_twvp);

	r_Constant				("m_v2w",			&binder_m_v2w);

	//r_Constant				("m_VP_prev",		&RImplementation.m_C_VP_prev);
	//r_Constant				("m_P_prev",		&RImplementation.m_C_P_prev);
	//r_Constant				("m_v2w_prev",		&RImplementation.m_C_v2w_prev);

	r_Constant				("m_xform_v",		&tree_binder_m_xform_v);
	r_Constant				("m_xform",			&tree_binder_m_xform);
	r_Constant				("consts",			&tree_binder_consts);
	r_Constant				("wave",			&tree_binder_wave);
	r_Constant				("wind",			&tree_binder_wind);
	r_Constant				("c_scale",			&tree_binder_c_scale);
	r_Constant				("c_bias",			&tree_binder_c_bias);
	r_Constant				("c_sun",			&tree_binder_c_sun);

	r_Constant				("taa_jitter",	&binder_msaa_sample_pattern);

	//r_Constant("rain_updater", &binder_rain_params);
    //r_Constant("rain_updater", &binder_rain_params);

	//hemi cube
	r_Constant				("L_material",			&binder_material);
	r_Constant				("hemi_cube_pos_faces",			&binder_hemi_cube_pos_faces);
	r_Constant				("hemi_cube_neg_faces",			&binder_hemi_cube_neg_faces);

	//	Igor	temp solution for the texgen functionality in the shader
	r_Constant				("m_texgen",			&binder_texgen);
	//r_Constant				("mVPTexgen",			&binder_VPtexgen);

	r_Constant				("wea_sky_color",	&binder_sky_color);

	r_Constant				("dwframe",	&binder_dwframe);
	r_Constant				("depth_params",	&binder_depth_params);
#ifndef _EDITOR
	// fog-params
	r_Constant				("fog_plane",		&binder_fog_plane);
	r_Constant				("fog_params",		&binder_fog_params);
	r_Constant				("fog_color",		&binder_fog_color);
#endif
	// time
	r_Constant				("timers",			&binder_times);

	// eye-params
	r_Constant				("eye_position",	&binder_eye_P);
	r_Constant				("eye_direction",	&binder_eye_D);
	//r_Constant				("eye_normal",		&binder_eye_N);

#ifndef _EDITOR
	// global-lighting (env params)
	r_Constant				("L_sun_color",		&binder_sun0_color);
	r_Constant				("L_sun_dir_w",		&binder_sun0_dir_w);
	r_Constant				("L_sun_dir_e",		&binder_sun0_dir_e);
	r_Constant				("L_hemi_color",	&binder_hemi_color);
	r_Constant				("L_ambient",		&binder_amb_color);
#endif

	r_Constant				("developer_float4",		&binder_developer_float4);

	//r_Constant				("fov", &binder_fov_test);
	//r_Constant				("ss_dec_param1", &binder_ss_dec_param1);
	// detail 
	//if (bDetail	&& detail_scaler)
	//	Igor: bDetail can be overridden by no_detail_texture option.
	//	But shader can be deatiled implicitly, so try to set this parameter
	//	anyway.
	if (detail_scaler)
		r_Constant			("dt_params",		detail_scaler);

	// other common
	for (u32 it=0; it<DEV->v_constant_setup.size(); it++)
	{
		std::pair<shared_str,R_constant_setup*>	cs	= DEV->v_constant_setup[it];
		r_Constant			(*cs.first,cs.second);
	}
}
