#include "stdafx.h"

//-------------------------------------------
// RSM filtering

void CRenderTarget::phase_rsm_filter()
{
	PIX_EVENT(filter_rsm);
	u32 bias = 0;
	prepare_sq_vertex(rt_RSM_copy, bias, g_simple_quad);

	// We cant render into rt_RSM !!!!!
	u_setrt(rt_RSM_copy);
	u_setzb(NULL);
	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_rsm->E[SE_RSM_SPATIAL_FILTER]);
	RCache.set_c("c_rsm_generate_params_3", r__sun_il_params_3);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	//static Fmatrix m_vp_prev = {};

	u_setrt(rt_RSM, rt_RSM_depth);
	u_setzb(NULL);
	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_rsm->E[SE_RSM_TEMPORAL_FILTER]);
	RCache.set_c("m_tVP", TAA.get_xforms().m_VP);
	//RCache.set_c("m_VP_prev", m_vp_prev);
	RCache.set_c("c_rsm_generate_params_3", r__sun_il_params_3);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->CopyResource(rt_RSM_prev->pTexture->surface_get(), rt_RSM->pTexture->surface_get());
	//HW.pContext->CopyResource(rt_RSM_depth_prev->pTexture->surface_get(), rt_RSM_depth->pTexture->surface_get());

	//m_vp_prev = Device.mFullTransform;
}

//-------------------------------------------
// Screen Space Ambient Occlusion

void CRenderTarget::phase_ssao()
{
	PIX_EVENT(render_ssao);
	u32 bias = 0;
	prepare_sq_vertex(rt_Generic, bias, g_simple_quad);

	u_setrt(rt_Generic);
	u_setzb(FALSE);
	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_ssao->E[0]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	u_setrt(rt_PPTemp);
	u_setzb(FALSE);
	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_ssao->E[1]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
}

void CRenderTarget::phase_ssao_path_tracing()
{
	PIX_EVENT(path_trace_ssao);
	u32 bias = 0;

	// path trace
	if (RImplementation.o.pt_downsample)
	{
		prepare_sq_vertex(rt_SSAO_small, bias, g_simple_quad);
		u_setrt(rt_SSAO_small);
		u_setzb(FALSE);
	}
	else
	{
		prepare_sq_vertex(rt_SSAO_temp, bias, g_simple_quad);
		u_setrt(rt_SSAO_temp);
		u_setzb(FALSE);
	}

	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_ssao->E[3]);
	RCache.set_c("dwframe", (int)Device.dwFrame);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	// reset geometry
	prepare_sq_vertex(RImplementation.fWidth, RImplementation.fHeight, bias, g_simple_quad);

	// temporal resolve 
	//static Fmatrix m_vp_prev = {};
	u_setrt(rt_SSAO);
	u_setzb(FALSE);
	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_ssao->E[4]);
	RCache.set_c("m_tVP", TAA.get_xforms().m_VP);
	//RCache.set_c("m_VP_prev", m_vp_prev);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
	//m_vp_prev = Device.mFullTransform;

	// copy buffer
	HW.pContext->CopyResource(rt_SSAO_prev->pTexture->surface_get(), rt_SSAO->pTexture->surface_get());

	// blur
	if (opt(R__USE_SSAO_PT_BLUR))
	{
		u_setrt(rt_SSAO_temp);
		u_setzb(FALSE);
		RCache.set_Stencil(FALSE);
		RCache.set_Element(s_ssao->E[5]);
		RCache.set_c("ssao_pt_blur_params", 1, 2, 1, 0);
		RCache.set_Geometry(g_simple_quad);
		RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

		u_setrt(rt_SSAO);
		u_setzb(FALSE);
		RCache.set_Stencil(FALSE);
		RCache.set_Element(s_ssao->E[6]);
		RCache.set_c("ssao_pt_blur_params", 0, 2, 0, 1);
		RCache.set_Geometry(g_simple_quad);
		RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

		u_setrt(rt_SSAO_temp);
		u_setzb(FALSE);
		RCache.set_Stencil(FALSE);
		RCache.set_Element(s_ssao->E[5]);
		RCache.set_c("ssao_pt_blur_params", 1, 2, 3, 0);
		RCache.set_Geometry(g_simple_quad);
		RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

		u_setrt(rt_SSAO);
		u_setzb(FALSE);
		RCache.set_Stencil(FALSE);
		RCache.set_Element(s_ssao->E[6]);
		RCache.set_c("ssao_pt_blur_params", 0, 2, 0, 3);
		RCache.set_Geometry(g_simple_quad);
		RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
	}
}
#ifdef __GFSDK_DX11__
// NVIDIA HBAO+
void CRenderTarget::phase_hbao_plus()
{
	PIX_EVENT(render_hbao_plus);
	u32 bias = 0;
	prepare_sq_vertex(rt_HBAO_plus_normal, bias, g_simple_quad);

	u_setrt(rt_HBAO_plus_normal);
	u_setzb(NULL);
	RCache.set_Stencil(FALSE);
	RCache.set_Element(s_ssao->E[2]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	GFSDK_SSAO_InputData_D3D Input;
	Input.NormalData.Enable = true;
	Input.NormalData.pFullResNormalTextureSRV = rt_HBAO_plus_normal->pTexture->get_SRView();
	Input.NormalData.WorldToViewMatrix.Data = GFSDK_SSAO_Float4x4((const GFSDK_SSAO_FLOAT*)&RCache.get_xform_view());

	Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	Input.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4((const GFSDK_SSAO_FLOAT*)&RCache.get_xform_project());
	Input.DepthData.MetersToViewSpaceUnits = 1.0;

	if(RImplementation.o.aa_mode == AA_MSAA)
		Input.DepthData.pFullResDepthTextureSRV = rt_MSAA_depth->pTexture->get_SRView();
	else if(RImplementation.o.ssaa)
		Input.DepthData.pFullResDepthTextureSRV = rt_SSAA_depth->pTexture->get_SRView();
	else
		Input.DepthData.pFullResDepthTextureSRV = HW.pBaseDepthReadSRV;

	GFSDK_SSAO_Parameters Params;
	Params.StepCount = GFSDK_SSAO_STEP_COUNT_4;
	Params.Radius = 1.7;
	Params.Bias = 0.1;
	Params.PowerExponent = 1;
	Params.Blur.Enable = true;
	Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
	Params.Blur.Sharpness = 32;
	Params.DepthStorage = GFSDK_SSAO_FP16_VIEW_DEPTHS;

	GFSDK_SSAO_Output_D3D Output;
	Output.pRenderTargetView = rt_PPTemp->pRT;

	HW.pSSAO->RenderAO(HW.pContext, Input, Params, Output);
}
#endif
//-------------------------------------------
// Screen Space Sunshafts

void CRenderTarget::phase_sunshafts_screen(Fvector4& sun_direction, Fvector4& sun_color)
{
	PIX_EVENT(render_sunshafts);
	u32 bias = 0;
	prepare_sq_vertex(rt_PPTemp, bias, g_simple_quad);
	
	// Calc sunshafts (with noise)
	u_setrt(rt_PPTemp);
	u_setzb(NULL);
	RCache.set_Element(s_sunshafts->E[0]);
	RCache.set_c("Ldynamic_dir", sun_direction);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	// Dont need multisampling because it blurred

	// Denoise & apply
	phase_vol_accumulator(); // Render it to volumetric light texture
	RCache.set_Element(s_sunshafts->E[1]);
	RCache.set_c("Ldynamic_color", sun_color);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
}

//-------------------------------------------
// Screen Space Planar Reflections

void CRenderTarget::phase_sspr()
{
	PIX_EVENT(compute_ssr);
	if (HW.FeatureLevel < D3D_FEATURE_LEVEL_11_0) return;

	static u32 g_uGroupTexelDimension = 56;
	static u32 g_uGroupTexelOverlap = 12;
	static u32 g_uGroupTexelDimensionAfterOverlap = g_uGroupTexelDimension - 2 * g_uGroupTexelOverlap;

	RCache.clear_RenderTargetView(rt_SSPR->pRT, rgba_black);

	RCache.set_ComputeShader(s_sspr);

	int iGroupsX = (int)ceil(RImplementation.fWidth / (float)g_uGroupTexelDimensionAfterOverlap);
	int iGroupsY = (int)ceil(RImplementation.fHeight / (float)g_uGroupTexelDimensionAfterOverlap);

	RCache.ComputeUAVWithRestore(rt_SSPR->pUAView, iGroupsX, iGroupsY, 1);
}

//-------------------------------------------
// Antialiasing

// Multisampled

// Perform TAA and put all frames together 
void CRenderTarget::phase_TAA()
{
	PIX_EVENT(taa_recursive);
	u32 bias = 0;
	prepare_sq_vertex(rt_PPTemp, bias, g_simple_quad);

	u_setrt(rt_PPTemp);
	u_setzb(NULL);

	RCache.set_Element(s_taa->E[SE_TAA]);
	RCache.set_c("m_tVP", TAA.get_xforms().m_VP);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->CopyResource(rt_Color->pTexture->surface_get(), rt_PPTemp->pTexture->surface_get());
	HW.pContext->CopyResource(rt_Generic_0_feedback->pTexture->surface_get(), rt_PPTemp->pTexture->surface_get());
}

void CRenderTarget::phase_TAA_V2()
{
	PIX_EVENT(taa);
	u32 bias = 0;
	prepare_sq_vertex(rt_Generic, bias, g_simple_quad);
	RCache.set_Geometry(g_simple_quad);

	u_setrt(rt_Generic);
	u_setzb(FALSE);

	RCache.set_Element(s_taa->E[SE_TAA_V2_ANTIALIASING]);

	for (u32 i = 0; i < TAA_FEEDBACK_SIZE; i++)
		RCache.set_ca("m_tVP_array", i, m_TAA[i]);

	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	u_setrt(rt_TAA[Device.dwFrame % TAA_FEEDBACK_SIZE]);
	RCache.set_Element(s_taa->E[SE_TAA_V2_COPY_FRAME]);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->CopyResource(rt_Color->pTexture->surface_get(), rt_Generic->pTexture->surface_get());

	m_TAA[Device.dwFrame % TAA_FEEDBACK_SIZE] = Device.mFullTransform;
}

// Singlesampled

void CRenderTarget::phase_FXAA()
{
	PIX_EVENT(fxaa);
	u32 bias = 0;
	prepare_sq_vertex(rt_PPTemp, bias, g_simple_quad);

	u_setrt(rt_PPTemp);
	u_setzb(NULL);
	RCache.set_Element(s_antialiasing->E[SE_FXAA]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->CopyResource(rt_Color->pTexture->surface_get(), rt_PPTemp->pTexture->surface_get());
}

void CRenderTarget::phase_MLAA()
{
	PIX_EVENT(mlaa);
	u32 bias = 0;
	prepare_sq_vertex(rt_MLAA_0, bias, g_simple_quad);

	// Detect seperating lines
	u_setrt(rt_MLAA_0);
	u_setzb(NULL);
	RCache.set_Element(s_antialiasing->E[SE_MLAA_0]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	// Lines length compute
	u_setrt(rt_MLAA_1);
	u_setzb(NULL);
	RCache.set_Element(s_antialiasing->E[SE_MLAA_1]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	// Antialiasing and blend
	u_setrt(rt_PPTemp);
	u_setzb(NULL);
	RCache.set_Element(s_antialiasing->E[SE_MLAA_2]);
	RCache.set_Geometry(g_simple_quad);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->CopyResource(rt_Color->pTexture->surface_get(), rt_PPTemp->pTexture->surface_get());
}
