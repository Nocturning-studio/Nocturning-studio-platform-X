#include "stdafx.h"
#include "../xrEngine/igame_persistent.h"
#include "../xrEngine/environment.h"

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z

static Fvector3		corners [8]			= 
{
	{ -1, -1,  0.7 },	{ -1, -1, +1},
	{ -1, +1, +1 },		{ -1, +1,  0.7},
	{ +1, +1, +1 },		{ +1, +1,  0.7},
	{ +1, -1, +1 },		{ +1, -1,  0.7}
};

static u16			facetable[16][3]		= 
{
	{ 3, 2, 1 },  
	{ 3, 1, 0 },		
	{ 7, 6, 5 }, 
	{ 5, 6, 4 },		
	{ 3, 5, 2 },
	{ 4, 2, 5 },		
	{ 1, 6, 7 },
	{ 7, 0, 1 },

	{ 5, 3, 0 },
	{ 7, 5, 0 },

	{ 1, 4, 6 },
	{ 2, 4, 1 },
};

void CRenderTarget::accum_direct_cascade	( u32 sub_phase, float radius_n, Fmatrix& xform, Fmatrix& xform_prev, float fBias )
{
	PIX_EVENT(accum_cascade);
	CRender* R = &RImplementation;

	// Choose normal code-path or filtered
	phase_accumulator					();

	//	choose correct element for the sun shader
	u32 uiElementIndex = sub_phase;

	//	TODO: DX10: Remove half pixe offset
	// *** assume accumulator setted up ***
	light* sun = (light*)R->Lights.sun_adapted._get()	;

	// Common calc for quad-rendering
	u32		Offset;
	u32		C					= color_rgba	(255,255,255,255);
	float	_w					= RImplementation.fWidth;
	float	_h					= RImplementation.fHeight;
	Fvector2					p0,p1;
	p0.set						(.5f/_w, .5f/_h);
	p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
	float	d_Z	= EPS_S, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir,L_clr;	float L_spec;
	L_clr.set					(sun->color.r, sun->color.g, sun->color.b);
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_dir	(L_dir, sun->direction);
	L_dir.normalize				();

	// Perform masking (only once - on the first/near phase)
	RCache.set_CullMode			(D3D_CULL_NONE);
	PIX_EVENT(SE_SUN_NEAR_sub_phase);
	if (SE_SUN_NEAR==sub_phase)	//.
		//if( 0 )
	{
		// Fill vertex buffer
		FVF::TL* pv					= (FVF::TL*)	RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
		pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y);	pv++;
		pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y);	pv++;
		pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y);	pv++;
		pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y);	pv++;
		RCache.Vertex.Unlock		(4,g_combine->vb_stride);
		RCache.set_Geometry			(g_combine);

		// setup
		float	intensity			= 0.3f* sun->color.r + 0.48f* sun->color.g + 0.22f* sun->color.b;
		Fvector	dir					= L_dir;
		dir.normalize().mul	(- _sqrt(intensity+EPS));
		RCache.set_Element			(s_accum_mask->E[SE_MASK_DIRECT]);		// masker
		RCache.set_c				("Ldynamic_dir",		dir.x,dir.y,dir.z,0		);

		// setup stencil

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);
				RCache.set_CullMode(D3D_CULL_NONE);
				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
				RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			// per pixel rendering // checked Holger
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			// per sample rendering
			RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
			RCache.set_CullMode(D3D_CULL_NONE);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		}
		else
		{
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}
		
	}

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad	(Device.vCameraPosition,Device.vCameraDirection, r__sun_near);
	Device.mFullTransform.transform(center_pt)	;
	d_Z							= center_pt.z	;

	// nv-stencil recompression
	if (R->o.nvstencil  && (SE_SUN_NEAR ==sub_phase))	u_stencil_optimize();	//. driver bug?

	PIX_EVENT(Perform_lighting);

	// Perform lighting
	{
		phase_accumulator					()	;
		RCache.set_CullMode					(D3D_CULL_BACK); //******************************************************************
		RCache.set_ColorWriteEnable			()	;

		float			fRange				= (SE_SUN_NEAR ==sub_phase)? r__sun_depth_near_scale : r__sun_depth_far_scale;

		//	TODO: DX10: Remove this when fix inverse culling for far region
		Fmatrix			m_TexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				fRange,			0.0f,
			0.5f,				0.5f,				fBias,			1.0f
		};

		// compute xforms
		FPU::m64r			();
		Fmatrix				xf_invview;		xf_invview.invert	(Device.mView)	;

		// shadow xform
		Fmatrix m_shadow;
		Fmatrix xf_project;
		Fmatrix bias_t = Fidentity;
		{
			xf_project.mul		(m_TexelAdjust, sun->X.D.combine);
			m_shadow.mul	(xf_project,	xf_invview);

			// tsm-bias
			if(SE_SUN_FAR == sub_phase)
			{
				float r__sun_tsm_bias = -0.01f;
				Fvector		bias;	bias.mul		(L_dir,r__sun_tsm_bias);
				bias_t.translate(bias);
				m_shadow.mulB_44	(bias_t);
			}
			FPU::m24r		();
		}

		// clouds xform
		Fmatrix				m_clouds_shadow;
		{
			static	float	w_shift		= 0;
			Fmatrix			m_xform;
			Fvector			direction	= sun->direction	;
			float	w_dir				= g_pGamePersistent->Environment().CurrentEnv->wind_direction	;
			//float	w_speed				= g_pGamePersistent->Environment().CurrentEnv->wind_velocity	;
			Fvector			normal	;	normal.setHP(w_dir,0);
			w_shift		+=	0.003f*Device.fTimeDelta;
			Fvector			position;	position.set(0,0,0);
			m_xform.build_camera_dir	(position,direction,normal)	;
			Fvector			localnormal;m_xform.transform_dir(localnormal,normal); localnormal.normalize();
			m_clouds_shadow.mul			(m_xform,xf_invview)		;
			m_xform.scale				(0.002f,0.002f,1.f)			;
			m_clouds_shadow.mulA_44		(m_xform)					;
			m_xform.translate			(localnormal.mul(w_shift))	;
			m_clouds_shadow.mulA_44		(m_xform)					;
		}

		// Compute textgen texture for pixel shader, for possitions texture.
		Fmatrix			m_Texgen;
		m_Texgen.identity();

		RCache.set_xform_world(m_Texgen);
		RCache.set_xform_view(Device.mView);
		RCache.set_xform_project(Device.mProject);

 		u_compute_texgen_screen	( m_Texgen );


		// Make jitter texture
		Fvector2					j0,j1;
		float	scale_X				= RImplementation.fWidth	/ float(TEX_jitter);
		float	offset				= (.5f / float(TEX_jitter));
		j0.set						(offset,offset);
		j1.set						(scale_X,scale_X).add(offset);

		// Fill vertex buffer
		u32		i_offset;
		{
			u16*	pib					= RCache.Index.Lock	(sizeof(facetable)/sizeof(u16),i_offset);
			CopyMemory					(pib,&facetable,sizeof(facetable));
			RCache.Index.Unlock			(sizeof(facetable)/sizeof(u16));

			//corners

			u32 ver_count = sizeof(corners)/ sizeof(Fvector3);
			Fvector4* pv				= (Fvector4*)	RCache.Vertex.Lock	( ver_count,g_combine_cuboid.stride(),Offset);
			

			Fmatrix inv_XDcombine;
			if(sub_phase == SE_SUN_FAR)
				inv_XDcombine.invert(xform_prev);
			else
				inv_XDcombine.invert(xform);
				

			for ( u32 i = 0; i < ver_count; ++i )
			{
				Fvector3 tmp_vec;
				inv_XDcombine.transform(tmp_vec, corners[i]);
				pv->set						( tmp_vec.x,tmp_vec.y, tmp_vec.z, 1 );	
				pv++;
			}
			RCache.Vertex.Unlock		(ver_count,g_combine_cuboid.stride());
		}

		RCache.set_Geometry			(g_combine_cuboid);

		// setup
		RCache.set_Element			(s_accum_direct->E[uiElementIndex]);
		RCache.set_c				("m_texgen",			m_Texgen);
		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0		);
		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,L_spec	);
		RCache.set_c				("m_shadow",			m_shadow						);
		RCache.set_c				("m_sunmask",			m_clouds_shadow					);

		if (RImplementation.o.vsm)
			RCache.set_c("SHADOW_CASCEDE_SCALE", (int)sub_phase);
		else
			RCache.set_c("cascede_scale", radius_n);


		if(sub_phase == SE_SUN_FAR)
		{
			Fvector3 view_viewspace;	view_viewspace.set( 0, 0, 1 );

			m_shadow.transform_dir( view_viewspace );
			Fvector4 view_projlightspace;
			view_projlightspace.set( view_viewspace.x, view_viewspace.y, 0, 0 );
			view_projlightspace.normalize();

			RCache.set_c				("view_shadow_proj",	view_projlightspace);
		}

		// nv-DBT
		float zMin,zMax;
		if (SE_SUN_NEAR ==sub_phase)	{
			zMin = 0;
			zMax = r__sun_near;
		} else {
			zMin = r__sun_near;
			zMax = r__sun_far;
		}
		center_pt.mad(Device.vCameraPosition,Device.vCameraDirection,zMin);	Device.mFullTransform.transform	(center_pt);
		zMin = center_pt.z	;

		center_pt.mad(Device.vCameraPosition,Device.vCameraDirection,zMax);	Device.mFullTransform.transform	(center_pt);
		zMax = center_pt.z	;

		//	TODO: DX10: Check if DX10 has analog for NV DBT
		//		if (u_DBT_enable(zMin,zMax))	{
		// z-test always
		//			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3D_COMPARISON_ALWAYS);
		//			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		//		}

		// Enable Z function only for near and middle cascades, the far one is restricted by only stencil.
		if ((SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase))
		{
			RCache.set_ZFunc(D3D_COMPARISON_GREATER_EQUAL);
		}
		else
		{
			if (opt(R__USE_SUN_ZCUL))
				RCache.set_ZFunc(D3D_COMPARISON_LESS);
			else
				RCache.set_ZFunc(D3D_COMPARISON_ALWAYS);
		}

		u32 st_mask = 0xFE;
		D3D_STENCIL_OP st_pass = D3D_STENCIL_OP_ZERO;

		if( sub_phase == SE_SUN_FAR)
		{
			st_mask = 0x00;
			st_pass = D3D_STENCIL_OP_KEEP;
		}

		// setup stencil

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_Element(s_accum_direct->E[uiElementIndex]);
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);

				switch (sub_phase)
				{
				case SE_SUN_NEAR: case SE_SUN_MIDDLE:
					RCache.set_ZFunc(D3D_COMPARISON_GREATER_EQUAL);
					break;
				default:
					if (opt(R__USE_SUN_ZCUL))
						RCache.set_ZFunc(D3D_COMPARISON_LESS);
					else
						RCache.set_ZFunc(D3D_COMPARISON_ALWAYS);
				}

				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
				RCache.set_CullMode(D3D_CULL_NONE);
				RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0xff, 0x00);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			// per pixel
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

			// per sample
			RCache.set_Element(s_accum_direct->E[uiElementIndex]);

			switch (sub_phase)
			{
			case SE_SUN_NEAR: case SE_SUN_MIDDLE:
				RCache.set_ZFunc(D3D_COMPARISON_GREATER_EQUAL);
				break;
			default:
				if(opt(R__USE_SUN_ZCUL))
					RCache.set_ZFunc(D3D_COMPARISON_LESS);
				else
					RCache.set_ZFunc(D3D_COMPARISON_ALWAYS);
			}

			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.set_CullMode(D3D_CULL_NONE);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0xff, 0x00);
		}
		else
		{
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
		}

		if (sub_phase == SE_SUN_FAR)
		{

			if (need_to_render_sunshafts(SUNSHAFTS_VOLUME))
				accum_direct_volumetric(Offset, m_shadow, L_clr);

			if (need_to_render_sun_il())
				accum_direct_reflective(Offset, m_shadow, xf_project, L_clr, radius_n);

			if (RImplementation.o.planar)
				RImplementation.planar_save_shadow(xf_project, bias_t);

		}
	}
}


void CRenderTarget::accum_direct_volumetric(const u32 Offset, const Fmatrix& mShadow, const Fvector L_clr)
{
	PIX_EVENT(accum_direct_volumetric);

	CRender* R = &RImplementation;

	phase_vol_accumulator();

	RCache.set_ColorWriteEnable();

	ref_selement Element;

	if (use_minmax_sm_this_frame())
		Element = s_accum_direct->E[SE_SUN_FAR_VOL_MINMAX];
	else
		Element = s_accum_direct->E[SE_SUN_FAR_VOLUMETRIC];

	//	Assume everything was recalculated before this call by accum_direct

	{
		//	Use g_combine_2UV that was set up by accum_direct
		//	RCache.set_Geometry			(g_combine_2UV);

		// setup
		RCache.set_Element(Element);
		RCache.set_CullMode(D3D_CULL_BACK);

		RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, 0);
		RCache.set_c("m_shadow", mShadow);

		Fmatrix m_Texgen;
		m_Texgen.identity();

		RCache.set_xform_world(m_Texgen);
		RCache.set_xform_view(Device.mView);
		RCache.set_xform_project(Device.mProject);

		u_compute_texgen_screen(m_Texgen);
		RCache.set_c("m_texgen", m_Texgen);

		Fvector	center_pt;
		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, 0);
		Device.mFullTransform.transform(center_pt);

		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, r__sun_far);
		Device.mFullTransform.transform(center_pt);

		RCache.set_ZFunc(D3D_COMPARISON_ALWAYS);

		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
	}
}

void CRenderTarget::accum_direct_reflective(const u32 Offset, const Fmatrix& mShadow, const Fmatrix& mShadowP, const Fvector L_clr, const float cascede_scale)
{
	PIX_EVENT(accum_direct_reflective);

	CRender* R = &RImplementation;

	phase_rsm_accumulator();

	{
		// setup
		RCache.set_Element(s_rsm->E[SE_RSM_DIRECT]);

		RCache.set_c("cascede_scale", cascede_scale);

		RCache.set_c("dwframe", (int)Device.dwFrame);

		RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, 0);
		RCache.set_c("m_shadow", mShadow);
		//RCache.set_c("m_shadowP", mShadowP);

		Fmatrix m_Texgen;
		m_Texgen.identity();

		RCache.set_xform_world(m_Texgen);
		RCache.set_xform_view(Device.mView);
		RCache.set_xform_project(Device.mProject);

		u_compute_texgen_screen(m_Texgen);
		RCache.set_c("m_texgen", m_Texgen);

		RCache.set_c("c_rsm_generate_params_0", r__sun_il_params_0);
		RCache.set_c("c_rsm_generate_params_1", r__sun_il_params_1);
		RCache.set_c("c_rsm_generate_params_2", r__sun_il_params_2);

		Fvector	center_pt;
		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, 0);
		Device.mFullTransform.transform(center_pt);

		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, r__sun_far);
		Device.mFullTransform.transform(center_pt);

		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
	}
}

void CRenderTarget::accum_direct_vsm(sun::cascade shadow)
{
	PIX_EVENT(accum_variance_smap);
	CRender* R = &RImplementation;

	// Choose normal code-path or filtered
	phase_accumulator();

	//	TODO: DX10: Remove half pixe offset
	// *** assume accumulator setted up ***
	light* sun = (light*)R->Lights.sun_adapted._get();

	// Common calc for quad-rendering
	u32		Offset;
	u32		C = color_rgba(255, 255, 255, 255);
	float	_w = RImplementation.fWidth;
	float	_h = RImplementation.fHeight;
	Fvector2					p0, p1;
	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h);
	float	d_Z = EPS_S, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir, L_clr;	float L_spec;
	L_clr.set(sun->color.r, sun->color.g, sun->color.b);
	L_spec = u_diffuse2s(L_clr);
	Device.mView.transform_dir(L_dir, sun->direction);
	L_dir.normalize();

	// Perform masking (only once - on the first/near phase)
	RCache.set_CullMode(D3D_CULL_NONE);
	PIX_EVENT(SE_SUN_NEAR_sub_phase);
	//if (SE_SUN_NEAR == sub_phase)	//.
		//if( 0 )
	//{
		// Fill vertex buffer
		FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
		pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);	pv++;
		pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);	pv++;
		pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);	pv++;
		pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);	pv++;
		RCache.Vertex.Unlock(4, g_combine->vb_stride);
		RCache.set_Geometry(g_combine);

		// setup
		float	intensity = 0.3f * sun->color.r + 0.48f * sun->color.g + 0.22f * sun->color.b;
		Fvector	dir = L_dir;
		dir.normalize().mul(-_sqrt(intensity + EPS));
		RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]);		// masker
		RCache.set_c("Ldynamic_dir", dir.x, dir.y, dir.z, 0);

		// setup stencil

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);
				RCache.set_CullMode(D3D_CULL_NONE);
				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
				RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			// per pixel rendering // checked Holger
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			// per sample rendering
			RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
			RCache.set_CullMode(D3D_CULL_NONE);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0x81, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
		}
		else
		{
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}

	//}

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, r__sun_near);
	Device.mFullTransform.transform(center_pt);
	d_Z = center_pt.z;

	// nv-stencil recompression
	//if (R->o.nvstencil && (SE_SUN_NEAR == sub_phase))	u_stencil_optimize();	//. driver bug?
	u_stencil_optimize();	//. driver bug?

	PIX_EVENT(Perform_lighting);

	// Perform lighting
	{
		phase_accumulator();
		RCache.set_CullMode(D3D_CULL_BACK); //******************************************************************
		RCache.set_ColorWriteEnable();

		float			fRange = 1;

		//	TODO: DX10: Remove this when fix inverse culling for far region
		Fmatrix			m_TexelAdjust =
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				fRange,			0.0f,
			0.5f,				0.5f,				shadow.bias,	1.0f
		};

		// compute xforms
		FPU::m64r();
		Fmatrix				xf_invview;		xf_invview.invert(Device.mView);

		// shadow xform
		Fmatrix m_shadow;
		Fmatrix xf_project;
		Fmatrix bias_t = Fidentity;
		{
			xf_project.mul(m_TexelAdjust, sun->X.D.combine);
			m_shadow.mul(xf_project, xf_invview);

			// tsm-bias
			/*if (SE_SUN_FAR == sub_phase)
			{
				float r__sun_tsm_bias = -0.01f;
				Fvector		bias;	bias.mul(L_dir, r__sun_tsm_bias);
				bias_t.translate(bias);
				m_shadow.mulB_44(bias_t);
			}*/
			FPU::m24r();
		}

		// clouds xform
		Fmatrix				m_clouds_shadow;
		{
			static	float	w_shift = 0;
			Fmatrix			m_xform;
			Fvector			direction = sun->direction;
			float	w_dir = g_pGamePersistent->Environment().CurrentEnv->wind_direction;
			//float	w_speed				= g_pGamePersistent->Environment().CurrentEnv->wind_velocity	;
			Fvector			normal;	normal.setHP(w_dir, 0);
			w_shift += 0.003f * Device.fTimeDelta;
			Fvector			position;	position.set(0, 0, 0);
			m_xform.build_camera_dir(position, direction, normal);
			Fvector			localnormal; m_xform.transform_dir(localnormal, normal); localnormal.normalize();
			m_clouds_shadow.mul(m_xform, xf_invview);
			m_xform.scale(0.002f, 0.002f, 1.f);
			m_clouds_shadow.mulA_44(m_xform);
			m_xform.translate(localnormal.mul(w_shift));
			m_clouds_shadow.mulA_44(m_xform);
		}

		// Compute textgen texture for pixel shader, for possitions texture.
		Fmatrix			m_Texgen;
		m_Texgen.identity();

		RCache.set_xform_world(m_Texgen);
		RCache.set_xform_view(Device.mView);
		RCache.set_xform_project(Device.mProject);

		u_compute_texgen_screen(m_Texgen);


		// Make jitter texture
		Fvector2					j0, j1;
		float	scale_X = RImplementation.fWidth / float(TEX_jitter);
		float	offset = (.5f / float(TEX_jitter));
		j0.set(offset, offset);
		j1.set(scale_X, scale_X).add(offset);

		// Fill vertex buffer
		u32		i_offset;
		{
			u16* pib = RCache.Index.Lock(sizeof(facetable) / sizeof(u16), i_offset);
			CopyMemory(pib, &facetable, sizeof(facetable));
			RCache.Index.Unlock(sizeof(facetable) / sizeof(u16));

			//corners

			u32 ver_count = sizeof(corners) / sizeof(Fvector3);
			Fvector4* pv = (Fvector4*)RCache.Vertex.Lock(ver_count, g_combine_cuboid.stride(), Offset);


			Fmatrix inv_XDcombine;
			//if (sub_phase == SE_SUN_FAR)
			//	inv_XDcombine.invert(xform_prev);
			//else
				inv_XDcombine.invert(shadow.xform);


			for (u32 i = 0; i < ver_count; ++i)
			{
				Fvector3 tmp_vec;
				inv_XDcombine.transform(tmp_vec, corners[i]);
				pv->set(tmp_vec.x, tmp_vec.y, tmp_vec.z, 1);
				pv++;
			}
			RCache.Vertex.Unlock(ver_count, g_combine_cuboid.stride());
		}

		RCache.set_Geometry(g_combine_cuboid);

		// setup
		RCache.set_Element(s_accum_direct->E[SE_SUN_VSM]);
		RCache.set_c("m_texgen", m_Texgen);
		RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
		RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
		RCache.set_c("m_shadow", m_shadow);
		RCache.set_c("m_sunmask", m_clouds_shadow);

		RCache.set_c("cascede_scale", 1);

		/*if (sub_phase == SE_SUN_FAR)
		{
			Fvector3 view_viewspace;	view_viewspace.set(0, 0, 1);

			m_shadow.transform_dir(view_viewspace);
			Fvector4 view_projlightspace;
			view_projlightspace.set(view_viewspace.x, view_viewspace.y, 0, 0);
			view_projlightspace.normalize();

			RCache.set_c("view_shadow_proj", view_projlightspace);
		}*/

		// nv-DBT
		float zMin, zMax;
		//if (SE_SUN_NEAR == sub_phase) {
			zMin = 0;
			zMax = r__sun_near;
		//}
		//else {
		//	zMin = r__sun_near;
		//	zMax = r__sun_far;
		//}
		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMin);	Device.mFullTransform.transform(center_pt);
		zMin = center_pt.z;

		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMax);	Device.mFullTransform.transform(center_pt);
		zMax = center_pt.z;

		//	TODO: DX10: Check if DX10 has analog for NV DBT
		//		if (u_DBT_enable(zMin,zMax))	{
		// z-test always
		//			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3D_COMPARISON_ALWAYS);
		//			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		//		}

		// Enable Z function only for near and middle cascades, the far one is restricted by only stencil.
		//if ((SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase))
		{
			RCache.set_ZFunc(D3D_COMPARISON_GREATER_EQUAL);
		}
		//else
		//{
		//	if (opt(R__USE_SUN_ZCUL))
		//		RCache.set_ZFunc(D3D_COMPARISON_LESS);
		//	else
		//		RCache.set_ZFunc(D3D_COMPARISON_ALWAYS);
		//}

		u32 st_mask = 0xFE;
		D3D_STENCIL_OP st_pass = D3D_STENCIL_OP_ZERO;

		//if (sub_phase == SE_SUN_FAR)
		//{
		//	st_mask = 0x00;
		//	st_pass = D3D_STENCIL_OP_KEEP;
		//}

		// setup stencil

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_Element(s_accum_direct->E[SE_SUN_VSM]);
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);

				//switch (sub_phase)
				//{
				//case SE_SUN_NEAR: case SE_SUN_MIDDLE:
					RCache.set_ZFunc(D3D_COMPARISON_GREATER_EQUAL);
				//	break;
				//default:
				//	if (opt(R__USE_SUN_ZCUL))
				//		RCache.set_ZFunc(D3D_COMPARISON_LESS);
				//	else
				//		RCache.set_ZFunc(D3D_COMPARISON_ALWAYS);
				//}

				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
				RCache.set_CullMode(D3D_CULL_NONE);
				RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0xff, 0x00);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			// per pixel
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

			// per sample
			RCache.set_Element(s_accum_direct->E[SE_SUN_VSM]);

			//switch (sub_phase)
			//{
			//case SE_SUN_NEAR: case SE_SUN_MIDDLE:
				RCache.set_ZFunc(D3D_COMPARISON_GREATER_EQUAL);
			//	break;
			//default:
			//	if (opt(R__USE_SUN_ZCUL))
			//		RCache.set_ZFunc(D3D_COMPARISON_LESS);
			//	else
			//		RCache.set_ZFunc(D3D_COMPARISON_ALWAYS);
			//}

			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.set_CullMode(D3D_CULL_NONE);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0xff, 0x00);
		}
		else
		{
			RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0xff, st_mask, D3D_STENCIL_OP_KEEP, st_pass, D3D_STENCIL_OP_KEEP);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);
		}

		/*if (sub_phase == SE_SUN_FAR)
		{

			if (need_to_render_sunshafts(SUNSHAFTS_VOLUME))
				accum_direct_volumetric(Offset, m_shadow, L_clr);

			if (need_to_render_sun_il())
				accum_direct_reflective(Offset, m_shadow, xf_project, L_clr, radius_n);

			if (RImplementation.o.planar)
				RImplementation.planar_save_shadow(xf_project, bias_t);

		}*/
	}
}