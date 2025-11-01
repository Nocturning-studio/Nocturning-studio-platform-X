///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "blender_bloom.h"
///////////////////////////////////////////////////////////////////////////////////
bool TexturesIsClean = false;
///////////////////////////////////////////////////////////////////////////////////
void CRender::clear_bloom()
{
	TexturesIsClean = true;
	RenderBackend.ClearTexture(	RenderTarget->rt_Bloom_1, 
								RenderTarget->rt_Bloom_2, RenderTarget->rt_Bloom_Blades_1,
								RenderTarget->rt_Bloom_Blades_2 );
}

void CRender::calculate_bloom()
{
	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_Stencil(FALSE);

	float BloomResolutionMultiplier = 0.5f;

	float w = float(Device.dwWidth) * BloomResolutionMultiplier;
	float h = float(Device.dwHeight) * BloomResolutionMultiplier;

	// Downsample, prepare image and store in rt_Bloom_1 and rt_Bloom_Blades
	{
		RenderBackend.set_Element(RenderTarget->s_bloom->E[SE_PASS_PREPARE]);
		RenderBackend.set_Constant("bloom_parameters",  ps_r_bloom_threshold, 
														ps_r_bloom_brightness, 
														ps_r_bloom_blades_threshold, 
														ps_r_bloom_blades_brightness);
		RenderBackend.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Bloom_1, RenderTarget->rt_Bloom_Blades_1);
	}

	//Main bloom effect
	for (int i = 0; i < 4; i++)
	{
		RenderBackend.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLOOM], 0);
		RenderBackend.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackend.set_Constant("bloom_blur_params", float(i) / 4, 0, 0, 0);
		RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Bloom_2);

		RenderBackend.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLOOM], 1);
		RenderBackend.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackend.set_Constant("bloom_blur_params", float(i) / 4, 0, 0, 0);
		RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Bloom_1);
	}

	// Blades effect
	if (ps_r_bloom_quality > 1)
	{
		RenderBackend.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLADES], 0);
		RenderBackend.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackend.set_Constant("bloom_blur_params", 0.5f, 0, 0, 0);
		RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Bloom_Blades_2);

		RenderBackend.set_Element(RenderTarget->s_bloom->E[SE_PASS_PROCESS_BLADES], 1);
		RenderBackend.set_Constant("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RenderBackend.set_Constant("bloom_blur_params", 0.5f, 0, 0, 0);
		RenderBackend.RenderViewportSurface(w, h, RenderTarget->rt_Bloom_Blades_1);
	}
}

void CRender::render_bloom()
{
	OPTICK_EVENT("CRender::render_bloom");

	if (ps_r_postprocess_flags.test(RFLAG_BLOOM))
	{
		TexturesIsClean = false;
		calculate_bloom();
	}
	else
	{
		if (!TexturesIsClean)
			clear_bloom();
	}
}
///////////////////////////////////////////////////////////////////////////////////
