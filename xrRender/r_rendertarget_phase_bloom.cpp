#include "stdafx.h"
#include "r_rendertarget.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"

void CRenderTarget::phase_bloom()
{
	OPTICK_EVENT("CRenderTarget::phase_bloom");

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// Constants
	u32 Offset = 0;

	float BloomResolutionMultiplier = 0.0f;

	switch (ps_r_bloom_quality)
	{
	case 1:
		BloomResolutionMultiplier = 0.2f;
		break;
	case 2:
		BloomResolutionMultiplier = 0.3f;
		break;
	case 3:
		BloomResolutionMultiplier = 0.5f;
		break;
	case 4:
		BloomResolutionMultiplier = 1.0f;
		break;
	}

	float w = float(Device.dwWidth) * BloomResolutionMultiplier;
	float h = float(Device.dwHeight) * BloomResolutionMultiplier;

	set_viewport_vertex_buffer(w, h, Offset);

	// Downsample, prepare image and store in rt_Bloom_1 and rt_Bloom_Blades
	{
		u_setrt(rt_Bloom_1, rt_Bloom_Blades_1, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[0]);
		RCache.set_c("bloom_parameters", ps_r_bloom_threshold, 
										 ps_r_bloom_brightness, 
										 ps_r_bloom_blades_threshold, 
										 ps_r_bloom_blades_brightness);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	//Main bloom effect
	for (int i = 0; i < 2; i++)
	{
		u_setrt(rt_Bloom_2, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[1]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Bloom_1, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[2]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// Blades effect
	for (int i = 0; i < 2; i++)
	{
		u_setrt(rt_Bloom_Blades_2, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[3]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Bloom_Blades_1, NULL, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[4]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}