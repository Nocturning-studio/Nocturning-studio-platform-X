#include "stdafx.h"

void CRenderTarget::draw_rain( light &RainSetup )
{
	PIX_EVENT(render_rain_postprocess);
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

	// recalculate d_Z, to perform depth-clipping
	const float fRainFar = 50;

	Fvector	center_pt;			center_pt.mad	(Device.vCameraPosition,Device.vCameraDirection,fRainFar);
	Device.mFullTransform.transform(center_pt)	;
	d_Z							= center_pt.z	;

	// Perform lighting
	{
		// texture adjustment matrix
		float			fRange				=  1;
		float			fBias				= -0.0001;
		float			smapsize			= float(RImplementation.o.smapsize);
		float			fTexelOffs			= (.5f / smapsize);
		float			view_dimX			= float(RainSetup.X.D.maxX-RainSetup.X.D.minX)/smapsize;
		float			view_dimY			= float(RainSetup.X.D.maxX-RainSetup.X.D.minX)/smapsize;
		float			view_sx				= float(RainSetup.X.D.minX)/smapsize;
		float			view_sy				= float(RainSetup.X.D.minY)/smapsize;
		Fmatrix			m_TexelAdjust		= 
		{
			view_dimX/2.f,							0.0f,									0.0f,		0.0f,
			0.0f,									-view_dimY/2.f,							0.0f,		0.0f,
			0.0f,									0.0f,									fRange,		0.0f,
			view_dimX/2.f + view_sx + fTexelOffs,	view_dimY/2.f + view_sy + fTexelOffs,	fBias,		1.0f
		};

		// compute xforms
		FPU::m64r			();
		Fmatrix				xf_invview;		xf_invview.invert	(Device.mView)	;

		// shadow xform
		Fmatrix				m_shadow;
		{
			Fmatrix			xf_project;		xf_project.mul		(m_TexelAdjust,RainSetup.X.D.combine);
			m_shadow.mul	(xf_project,	xf_invview);

			FPU::m24r		();
		}

		// Make jitter texture
		Fvector2					j0,j1;
		float	scale_X				= RImplementation.fWidth	/ float(TEX_jitter);
		float	offset				= (.5f / float(TEX_jitter));
		j0.set						(offset,offset);
		j1.set						(scale_X,scale_X).add(offset);

		// Fill vertex buffer
		FVF::TL2uv* pv				= (FVF::TL2uv*) RCache.Vertex.Lock	(4,g_combine_2UV->vb_stride,Offset);
		pv->set						(-1,	-1,	d_Z,	d_W, C, 0, 1, 0,		scale_X);	pv++;
		pv->set						(-1,	1,	d_Z,	d_W, C, 0, 0, 0,		0);	pv++;
		pv->set						(1,		-1,	d_Z,	d_W, C, 1, 1, scale_X,	scale_X);	pv++;
		pv->set						(1,		1,	d_Z,	d_W, C, 1, 0, scale_X,	0);	pv++;
		RCache.Vertex.Unlock		(4,g_combine_2UV->vb_stride);
		RCache.set_Geometry			(g_combine_2UV);

		//	Use for intermediate results
		//	Patch normal
		RCache.set_Element(s_rain->E[0]);
		RCache.set_c("m_shadow", m_shadow);
		RCache.set_c("wetness_params", g_pGamePersistent->Environment().CurrentEnv->rain_density, g_pGamePersistent->Environment().wetness_accum, fRainFar, 0);
		//RCache.set_c("wetness_params", g_pGamePersistent->Environment().CurrentEnv->rain_density, g_pGamePersistent->Environment().CurrentEnv->rain_density, fRainFar, 0);

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			u_setrt(rt_Accumulator);
			u_setzb(rt_MSAA_depth);

			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);
				RCache.set_CullMode(D3D_CULL_NONE);
				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
				RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(FALSE, D3D_COMPARISON_EQUAL, 0x01, 0xff, 0);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			u_setrt(rt_Accumulator);
			u_setzb(rt_MSAA_depth);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2); // per pixel execution
			RCache.set_CullMode(D3D_CULL_NONE);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2); // per sample
		}
		else
		{
			u_setrt(rt_Accumulator);
			if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
			else						u_setzb(HW.pBaseDepthReadWriteDSV);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x01, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}

		//	Apply normal and gloss
		RCache.set_Element(s_rain->E[1]);

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			u_setrt(rt_Position);
			u_setzb(rt_MSAA_depth);

			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);
				RCache.set_CullMode(D3D_CULL_NONE);
				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
				RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(FALSE, D3D_COMPARISON_EQUAL, 0x01, 0xff, 0);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			u_setrt(rt_Position);
			u_setzb(rt_MSAA_depth);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2); // per pixel execution
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
			RCache.set_CullMode(D3D_CULL_NONE);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2); // per sample
		}
		else
		{
			u_setrt(rt_Position);
			if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
			else						u_setzb(HW.pBaseDepthReadWriteDSV);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x01, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}

		//	Apply color
		RCache.set_Element(s_rain->E[2]);

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			u_setrt(rt_Color_ms);
			u_setzb(rt_MSAA_depth);

			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);
				RCache.set_CullMode(D3D_CULL_NONE);
				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
				RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(FALSE, D3D_COMPARISON_EQUAL, 0x01, 0xff, 0);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			u_setrt(rt_Color_ms);
			u_setzb(rt_MSAA_depth);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2); // per pixel execution
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2); // per sample
		}
		else
		{
			u_setrt(rt_Color);
			if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
			else						u_setzb(HW.pBaseDepthReadWriteDSV);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x01, 0);
			RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		}
	}
}
