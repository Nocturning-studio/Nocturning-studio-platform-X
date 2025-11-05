#include "stdafx.h"
#include "blender_add_effects.h"

void CRenderTarget::phase_amd_cas()
{
	PIX_EVENT(AMD_CAS);
	u32 bias = 0;
	prepare_sq_vertex(rt_Generic_0, bias, g_simple_quad);

	u_setrt(rt_Generic_0);
	u_setzb(FALSE);

	RCache.set_Element(s_combine->E[4]);
	RCache.set_Geometry(g_simple_quad);
	RCache.set_c("cas_params", r__cas_contrast, r__cas_sharpening, 0, 0);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->CopyResource(rt_Color->pTexture->surface_get(), rt_Generic_0->pTexture->surface_get());
}

void CRenderTarget::phase_amd_cas_port()
{
	u32 bias = 0;
	prepare_sq_vertex(RImplementation.fWidth, RImplementation.fHeight, bias, g_simple_quad);
	RCache.set_Geometry(g_simple_quad);

	u_setrt(rt_Generic_0);
	u_setzb(FALSE);

	RCache.set_Element(s_ssaa->E[SE_CAS_PORT]);

	RCache.set_c("const0", 
		(float)SSAA.CAS_const.const0[0], 
		(float)SSAA.CAS_const.const0[1], 
		(float)SSAA.CAS_const.const0[2], 
		(float)SSAA.CAS_const.const0[3]);

	RCache.set_c("const1", 
		(float)SSAA.CAS_const.const1[0], 
		(float)SSAA.CAS_const.const1[1], 
		(float)SSAA.CAS_const.const1[2], 
		(float)SSAA.CAS_const.const1[3]);

	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	HW.pContext->CopyResource(rt_Color->pTexture->surface_get(), rt_Generic_0->pTexture->surface_get());
}

void CRenderTarget::phase_amd_fsr_port()
{
	PIX_EVENT(AMD_FSR);
	if (!RImplementation.o.ssaa) return;

	// disable SSAA & update viewport
	disable_SSAA();
	RImplementation.rmNormal();

	u32 bias = 0;

	// update geometry
	prepare_sq_vertex(RImplementation.fWidth, RImplementation.fHeight, bias, g_simple_quad);
	RCache.set_Geometry(g_simple_quad);

	// Edge Adaptive Spatial Upsampling
	u_setrt(rt_SSAA_distort);
	u_setzb(FALSE);

	SE_SSAA easu = SE_FSR_PORT_EASU;

	if (opt(R__FSR_16) && HW.FeatureLevel >= D3D_FEATURE_LEVEL_12_0)
		easu = SE_FSR_PORT_EASU_16;

	RCache.set_Element(s_ssaa->E[easu]);

	RCache.set_c("const0",
		(float)SSAA.FSR_const.EASU_const.const0[0],
		(float)SSAA.FSR_const.EASU_const.const0[1],
		(float)SSAA.FSR_const.EASU_const.const0[2],
		(float)SSAA.FSR_const.EASU_const.const0[3]);

	RCache.set_c("const1",
		(float)SSAA.FSR_const.EASU_const.const1[0],
		(float)SSAA.FSR_const.EASU_const.const1[1],
		(float)SSAA.FSR_const.EASU_const.const1[2],
		(float)SSAA.FSR_const.EASU_const.const1[3]);

	RCache.set_c("const2",
		(float)SSAA.FSR_const.EASU_const.const2[0],
		(float)SSAA.FSR_const.EASU_const.const2[1],
		(float)SSAA.FSR_const.EASU_const.const2[2],
		(float)SSAA.FSR_const.EASU_const.const2[3]);

	RCache.set_c("const3",
		(float)SSAA.FSR_const.EASU_const.const3[0],
		(float)SSAA.FSR_const.EASU_const.const3[1],
		(float)SSAA.FSR_const.EASU_const.const3[2],
		(float)SSAA.FSR_const.EASU_const.const3[3]);

	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	// Robust Contrast Adaptive Sharpening
	u_setrt(rt_SSAA_color);
	u_setzb(FALSE);

	SE_SSAA rcas = SE_FSR_PORT_RCAS;

	if (opt(R__FSR_16) && HW.FeatureLevel >= D3D_FEATURE_LEVEL_12_0)
		easu = SE_FSR_PORT_RCAS_16;

	RCache.set_Element(s_ssaa->E[rcas]);

	RCache.set_c("const0",
		(float)SSAA.FSR_const.RCAS_const.const0[0],
		(float)SSAA.FSR_const.RCAS_const.const0[1],
		(float)SSAA.FSR_const.RCAS_const.const0[2],
		(float)SSAA.FSR_const.RCAS_const.const0[3]);

	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);
}
