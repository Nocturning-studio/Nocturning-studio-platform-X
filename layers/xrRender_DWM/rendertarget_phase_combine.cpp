#include "stdafx.h"
#include "../xrEngine/igame_persistent.h"
#include "../xrEngine/environment.h"
#include "dxEnvironmentRender.h"

#include "halton.h"

void CRenderTarget::phase_combine()
{
	PIX_EVENT(phase_combine);

	//enable_SSAA();
	//RImplementation.rmNormal();

	//*** exposure-pipeline
	u32 gpu_id = Device.dwFrame % HW.Caps.iGPUNum;
	t_LUM_src->surface_set(rt_LUM_pool[gpu_id * 2 + 0]->pSurface);
	t_LUM_dest->surface_set(rt_LUM_pool[gpu_id * 2 + 1]->pSurface);

#ifdef __GFSDK_DX11__
	if(RImplementation.o.txaa)
		motion_txaa();
#endif

	if (need_to_render_sun_il() || RImplementation.o.spot_il)
		phase_rsm_filter();

	switch (RImplementation.o.ssao)
	{
	case SSAO_SSAO:
		phase_ssao();
		break;
	case SSAO_PATH_TRACING:
		phase_ssao_path_tracing();
		break;
#ifdef __GFSDK_DX11__
	case SSAO_HBAO_PLUS:
		phase_hbao_plus();
		break;
#endif
	};

	if (RImplementation.o.advanced_mode)
		RCache.clear_RenderTargetView(rt_Generic->pRT, rgba_black);

	if (RImplementation.o.aa_mode == AA_MSAA)
	{
		RCache.clear_RenderTargetView(rt_Generic_0_ms->pRT, rgba_black);
		RCache.clear_RenderTargetView(rt_Generic_1_ms->pRT, rgba_black);
		u_setrt(rt_Generic_0_ms, rt_Generic_1_ms);
		u_setzb(rt_MSAA_depth); // for sky render
	}
	else
	{
		RCache.clear_RenderTargetView(rt_Generic_0->pRT, rgba_black);
		RCache.clear_RenderTargetView(rt_Generic_1->pRT, rgba_black);
		u_setrt(rt_Generic_0, rt_Generic_1);
		if(RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
		else						u_setzb(HW.pBaseDepthReadWriteDSV); // for sky render
	}

	RCache.set_CullMode(D3D_CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Draw skybox
	{
		g_pGamePersistent->Environment().RenderSky();
		g_pGamePersistent->Environment().RenderClouds();
	}
	
	RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, 0x01, 0xff, 0x00);	// stencil should be >= 1

	if (RImplementation.o.nvstencil)
	{
		u_stencil_optimize(SO_Combine);
		RCache.set_ColorWriteEnable();
	}

	Fvector4 sunclr, sundir;

	// sun-params
	{
		light* sun = (light*)RImplementation.Lights.sun_adapted._get();
		Fvector		L_dir, L_clr;	float L_spec;
		L_clr.set(sun->color.r, sun->color.g, sun->color.b);
		L_spec = u_diffuse2s(L_clr);
		Device.mView.transform_dir(L_dir, sun->direction);
		L_dir.normalize();

		sunclr.set(L_clr.x, L_clr.y, L_clr.z, L_spec);
		sundir.set(L_dir.x, L_dir.y, L_dir.z, 0);
	}

	bool menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	Fvector4	envclr = { 0, 0, 0, 0 }, ambclr = { 0, 0, 0, 0 }, fogclr = { 0, 0, 0, 0 };

	if(!menu_pp || RImplementation.o.planar)

	{
		// Calc params
		CEnvDescriptorMixer& envdesc = *g_pGamePersistent->Environment().CurrentEnv;
		const float minamb = 0.001f;
		ambclr.set(_max(envdesc.ambient.x * 2, minamb), _max(envdesc.ambient.y * 2, minamb), _max(envdesc.ambient.z * 2, minamb), 0);
		ambclr.mul(r__sun_lumscale_amb);

		envclr.set(envdesc.hemi_color.x * 2 + EPS,	envdesc.hemi_color.y * 2 + EPS,	envdesc.hemi_color.z * 2 + EPS,	envdesc.weight);

		fogclr.set(envdesc.fog_color.x, envdesc.fog_color.y, envdesc.fog_color.z, 0);
		envclr.x *= 2 * r__sun_lumscale_hemi;
		envclr.y *= 2 * r__sun_lumscale_hemi;
		envclr.z *= 2 * r__sun_lumscale_hemi;

		// Setup textures
		dxEnvDescriptorMixerRender& envdescren = *(dxEnvDescriptorMixerRender*)(&*envdesc.m_pDescriptorMixer);
		ID3DBaseTexture* e0 = menu_pp ? 0 : envdescren.sky_r_textures_env[0].second->surface_get();
		ID3DBaseTexture* e1 = menu_pp ? 0 : envdescren.sky_r_textures_env[1].second->surface_get();
		t_envmap_0->surface_set(e0);	_RELEASE(e0);
		t_envmap_1->surface_set(e1);	_RELEASE(e1);
	}

	// Draw full-screen quad textured with our scene image
	if (!menu_pp)
	{
		PIX_EVENT(combine_1);

		// Draw
		u32 bias = 0;
		prepare_sq_vertex(rt_Generic_0, bias, g_simple_quad);

		RCache.set_Element(s_combine->E[0]);
		RCache.set_Geometry(g_simple_quad);

		RCache.set_c("L_ambient", ambclr);

		float c_overbright = opt(R__USE_TONEMAP) ? r__tonemap_overbright : 0;
		RCache.set_c("c_overbright", c_overbright);

		RCache.set_c("Ldynamic_color", sunclr);
		RCache.set_c("Ldynamic_dir", sundir);

		RCache.set_c("env_color", envclr);
		RCache.set_c("fog_color", fogclr);

		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);
				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
				RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(FALSE, D3D_COMPARISON_EQUAL, 0x01, 0xff, 0);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			// per pixel
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x01, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

			// per sample
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, 0x81, 0x81, 0);
			RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

			RCache.set_Stencil(FALSE, D3D_COMPARISON_EQUAL, 0x01, 0xff, 0);
		}
		else
		{
			RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
		}
	}

	// Stencil enabled
	if (RImplementation.o.ssr && RImplementation.o.ssr_replace)
		phase_sspr();

	if (RImplementation.o.planar)
		RImplementation.planar_render(t_envmap_0, t_envmap_1, envclr, ambclr);

	// Forward rendering
	{
		PIX_EVENT(Forward_rendering);

		if (RImplementation.o.aa_mode == AA_MSAA)
		{
			if (RImplementation.o.advanced_mode) HW.pContext->ResolveSubresource(rt_Generic->pTexture->surface_get(), 0, 
				rt_Generic_0_ms->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
			if (RImplementation.o.ssr_replace) u_setrt(rt_Generic_0_ms, rt_Depth_1);
			else u_setrt(rt_Generic_0_ms);
			u_setzb(rt_MSAA_depth);
		}
		else
		{
			if (RImplementation.o.advanced_mode) HW.pContext->CopyResource(rt_Generic->pTexture->surface_get(), 
				rt_Generic_0->pTexture->surface_get());
			if (RImplementation.o.ssr_replace) u_setrt(rt_Generic_0, rt_Depth_1);
			else u_setrt(rt_Generic_0);
			if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
			else						u_setzb(HW.pBaseDepthReadWriteDSV);
		}

		RCache.set_CullMode(D3D_CULL_BACK);
		RCache.set_Stencil(FALSE);
		RCache.set_ColorWriteEnable();

		RImplementation.render_forward();

		if (g_pGamePersistent)	
			g_pGamePersistent->OnRenderPPUI_main();	// PP-UI
	}

	// Stencil disabled

	// Volumetric lights apply
	if (RImplementation.o.volumetriclight || RImplementation.o.sunshafts)
	{
		if (need_to_render_sunshafts(SUNSHAFTS_SCREEN))	phase_sunshafts_screen(sundir, sunclr);
		phase_combine_volumetric(sundir, sunclr);
	}

	RCache.set_Stencil(FALSE);

#ifdef __GFSDK_DX11__
	if (RImplementation.o.txaa)
		resolve_txaa();
	else 
#endif
		if (RImplementation.o.aa_mode == AA_MSAA && r__aa == AA_MSAA_FXAA)
		resolve_fxaa();
	else if (RImplementation.o.aa_mode == AA_MSAA)
		resolve_msaa();

	// Save G-Buffer
	if (RImplementation.o.gbd_save)
		HW.pContext->CopyResource(rt_Position_prev->pTexture->surface_get(), rt_Position->pTexture->surface_get());

	// Tonemap and Bloom 
	{
		// Write to rt_Bloom_1
		phase_bloom();

		// Combine distort (if need), bloom, perform depth of field
		// Write to rt_Color
		phase_combine_color();

		// Antialiasing
		switch (RImplementation.o.aa_mode)
		{
		case AA_TAA:
			phase_TAA();
			break;
		case AA_TAA_V2:
			phase_TAA_V2();
			break;
		case AA_MLAA:
			phase_MLAA();
			break;
		case AA_FXAA:
			phase_FXAA();
			break;
		}

		// resolve SSAA
		if (RImplementation.o.ssaa > USE_FSR)
			phase_amd_fsr_port();
		else if (RImplementation.o.ssaa > USE_SSAA)
			resolve_ssaa();
	}

	// AMD CAS
	if(opt(R__USE_CAS))
	{
		phase_amd_cas();
	}
	
	// Final PP
	{
		phase_pp();
	}

	// Re-adapt luminance
	// exposure-pipeline-clear
	RCache.set_Stencil(FALSE);
	std::swap(rt_LUM_pool[gpu_id * 2 + 0], rt_LUM_pool[gpu_id * 2 + 1]);
	t_LUM_src->surface_set(NULL);
	t_LUM_dest->surface_set(NULL);
}

void CRenderTarget::phase_combine_color()
{
	PIX_EVENT(combine_2);
	// Distortion filter

	bool menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	bool need_distort = (RImplementation.mapDistort.size() || menu_pp);

	if (need_distort)
	{
		FLOAT color[4] = {127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 127.0f / 255.0f};

		if (RImplementation.o.aa_mode == AA_MSAA)
		{
			RCache.clear_RenderTargetView(rt_Generic_1_ms->pRT, color);
			u_setrt(rt_Generic_1_ms);
			u_setzb(rt_MSAA_depth);
		}
		else
		{
			RCache.clear_RenderTargetView(rt_Generic_1->pRT, color);
			u_setrt(rt_Generic_1);
			if(RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
			else						u_setzb(HW.pBaseDepthReadWriteDSV);
		}

		RCache.set_CullMode(D3D_CULL_BACK);
		RCache.set_Stencil(FALSE);
		RCache.set_ColorWriteEnable();
		RImplementation.r_dsgraph_render_distort();

		if (g_pGamePersistent)	
			g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI
	}

	RCache.set_CullMode(D3D_CULL_NONE);

	// DoF params

	Fvector3 dof;
	g_pGamePersistent->GetCurrentDof(dof);

	// Combine 2

	u32 bias = 0;
	prepare_sq_vertex(rt_Color, bias, g_simple_quad);

	u_setrt(rt_Color);
	u_setzb(FALSE);
	RCache.set_Element(s_combine->E[need_distort ? 1 : 2]);
	RCache.set_c("dof_params", r__dof_kernel, dof.y, dof.z, r__dof_sky);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	// Combine flares
	g_pGamePersistent->Environment().RenderFlares();
}

void CRenderTarget::phase_combine_volumetric(Fvector4& sun_direction, Fvector4& sun_color)
{
	PIX_EVENT(combine_volumetric);
	u32 bias = 0;
	prepare_sq_vertex(rt_Generic_0, bias, g_simple_quad);

	if (RImplementation.o.aa_mode == AA_MSAA)
		u_setrt(rt_Generic_0_ms, rt_Generic_1_ms);
	else
		u_setrt(rt_Generic_0, rt_Generic_1);

	u_setzb(FALSE);

	RCache.set_Element(s_combine->E[3]);
	RCache.set_Geometry(g_simple_quad);

	RCache.set_c("Ldynamic_dir", sun_direction);
	RCache.set_c("Ldynamic_color", sun_color);

	float c_overbright = opt(R__USE_TONEMAP) ? r__tonemap_overbright : 0;
	RCache.set_c("c_overbright", c_overbright);

	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
}

void CRenderTarget::resolve_msaa(void)
{
	PIX_EVENT(resolve_msaa);
	HW.pContext->ResolveSubresource(rt_Generic_0->pTexture->surface_get(), 0, 
		rt_Generic_0_ms->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	HW.pContext->ResolveSubresource(rt_Generic_1->pTexture->surface_get(), 0, 
		rt_Generic_1_ms->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
}
#ifdef __GFSDK_DX11__
void CRenderTarget::feedback_txaa(void)
{
	HW.pContext->CopyResource(rt_Generic_0_feedback->pTexture->surface_get(), rt_Generic_0->pTexture->surface_get());
	//HW.pContext->CopyResource(rt_Generic_1_feedback->pTexture->surface_get(), rt_Generic_1->pTexture->surface_get());
}

void CRenderTarget::motion_txaa(void)
{
	PIX_EVENT(render_txaa_vectors);
	u32 bias = 0;
	prepare_sq_vertex(rt_Motion, bias, g_simple_quad);

	if (RImplementation.o.aa_mode == AA_MSAA)
		u_setrt(rt_Motion_ms);
	else
		u_setrt(rt_Motion);

	u_setzb(NULL);
	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_taa->E[SE_TXAA_MOTION]);
	RCache.set_c("m_tVP", TAA.get_xforms().m_VP);
	//RCache.set_c("m_VP_prev", m_txaa_vp_prev);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	//m_txaa_vp_prev.set(Device.mFullTransform);
}

void CRenderTarget::resolve_txaa(void)
{
	PIX_EVENT(resolve_txaa);
	NvTxaaResolveParametersDX11 resolveParameters;
	memset(&resolveParameters, 0, sizeof(resolveParameters));

	resolveParameters.txaaContext = &HW.m_TXAA;
	resolveParameters.deviceContext = HW.pContext;
	resolveParameters.alphaResolveMode = NV_TXAA_ALPHARESOLVEMODE_KEEPDESTALPHA;

	if (RImplementation.o.aa_mode == AA_MSAA)
		resolveParameters.msaaDepth = rt_MSAA_depth->pTexture->get_SRView();
	else if (RImplementation.o.ssaa)
		resolveParameters.msaaDepth = rt_SSAA_depth->pTexture->get_SRView();
	else
		resolveParameters.msaaDepth = HW.pBaseDepthReadSRV;

	NvTxaaFeedbackParameters feedback;
	feedback.clippedWeight = 0.75;
	feedback.weight = 0.75;
	resolveParameters.feedback = &feedback;

	NvTxaaPerFrameConstants perFrameConstants;
	perFrameConstants.useBHFilters = true;
	perFrameConstants.useRGB = true;
	perFrameConstants.isZFlipped = false;
	perFrameConstants.useAntiFlickerFilter = !!DEVX;
	perFrameConstants.mvScale = 1024;
	perFrameConstants.motionVecSelection = NV_TXAA_USEMV_NEAREST;
	perFrameConstants.frameBlendFactor = 0.04;
	perFrameConstants.dbg1 = 0;
	perFrameConstants.bbScale = 1;
	perFrameConstants.enableClipping = 1;

	float jitter[2] = { 0.0f, 0.0f };
	u8 x = Halton::Gen(Device.dwFrame, 0, NV_TXAA_MAX_NUM_FRAMES);
	u8 y = Halton::Gen(Device.dwFrame, 1, NV_TXAA_MAX_NUM_FRAMES);
	jitter[0] = 2.0f * (((float)x / 16.0f) - 0.5f) / RImplementation.fWidth;
	jitter[1] = 2.0f * (((float)y / 16.0f) - 0.5f) / RImplementation.fHeight;

	perFrameConstants.xJitter = jitter[0];
	perFrameConstants.yJitter = jitter[1];

	resolveParameters.perFrameConstants = &perFrameConstants;

	NvTxaaMotionDX11 motion;
	memset(&motion, 0, sizeof(motion));
	motion.motionVectorsMS = rt_Motion_ms->pTexture->get_SRView();
	motion.motionVectors = rt_Motion->pTexture->get_SRView();

	if (RImplementation.o.aa_mode == AA_MSAA)
	{
		resolveParameters.resolveTarget = rt_Generic_0->pRT;
		resolveParameters.msaaSource = rt_Generic_0_ms->pTexture->get_SRView();
	}
	else
	{
		HW.pContext->CopyResource(rt_PPTemp->pTexture->surface_get(), rt_Generic_0->pTexture->surface_get());
		resolveParameters.resolveTarget = rt_Generic_0->pRT;
		resolveParameters.msaaSource = rt_PPTemp->pTexture->get_SRView();
	}

	resolveParameters.feedbackSource = rt_Generic_0_feedback->pTexture->get_SRView();

	NvTxaaStatus R;
	
	R = GFSDK_TXAA_DX11_ResolveFromMotionVectors(&resolveParameters, &motion);

	//Msg("TXAA depth: %d", rt_MSAA_depth->pTexture->get_SRView());
	//Msg("TXAA Status: %d", R);

	//if (RImplementation.o.aa_mode == AA_MSAA)
	//{
	//	resolveParameters.resolveTarget = rt_Generic_1->pRT;
	//	resolveParameters.msaaSource = rt_Generic_1_ms->pTexture->get_SRView();
	//}
	//else
	//{
	//	HW.pContext->CopyResource(rt_PPTemp->pTexture->surface_get(), rt_Generic_1->pTexture->surface_get());
	//	resolveParameters.resolveTarget = rt_Generic_1->pRT;
	//	resolveParameters.msaaSource = rt_PPTemp->pTexture->get_SRView();
	//}
	//
	//resolveParameters.feedbackSource = rt_Generic_1_feedback->pTexture->get_SRView();
	//
	//R = GFSDK_TXAA_DX11_ResolveFromMotionVectors(&resolveParameters, &motion);

	// msaa resolve
	if (RImplementation.o.aa_mode == AA_MSAA)
	{
		HW.pContext->ResolveSubresource(rt_Generic_1->pTexture->surface_get(), 0,
			rt_Generic_1_ms->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	feedback_txaa();
}
#endif
void CRenderTarget::resolve_fxaa(void)
{
	PIX_EVENT(resolve_msaa_fxaa);
	u32 bias = 0;
	prepare_sq_vertex(rt_Color, bias, g_simple_quad);

	u_setrt(rt_Color, rt_PPTemp);
	u_setzb(NULL);
	RCache.set_Element(s_antialiasing->E[SE_MSAA_RESOLVE]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
	
	u_setrt(rt_Generic_0);
	u_setzb(NULL);
	RCache.set_Element(s_antialiasing->E[SE_FXAA2]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->ResolveSubresource(rt_Generic_1->pTexture->surface_get(), 0,
		rt_Generic_1_ms->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void CRenderTarget::resolve_ssaa(void)
{
	PIX_EVENT(resolve_ssaa);
	u32 bias = 0;
	prepare_sq_vertex(rt_SSAA_color, bias, g_simple_quad);
	u_setrt(rt_SSAA_color);
	u_setzb(NULL);
	RCache.set_Element(s_antialiasing->E[SE_MSAA_RESOLVE]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
}

