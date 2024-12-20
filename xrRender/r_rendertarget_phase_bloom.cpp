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
	u32 C = color_rgba(0, 0, 0, 255);

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

	float w = 256; // float(Device.dwWidth) * BloomResolutionMultiplier;
	float h = 256; // float(Device.dwHeight) * BloomResolutionMultiplier;

	float d_Z = EPS_S;
	float d_W = 1.f;

	Fvector2 p0, p1;
	p0.set(0.5f / w, 0.5f / h);
	p1.set((w + 0.5f) / w, (h + 0.5f) / h);

	// Fill vertex buffer
	FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
	pv->set(0, h, d_Z, d_W, C, p0.x, p1.y);
	pv++;
	pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y);
	pv++;
	pv->set(w, h, d_Z, d_W, C, p1.x, p1.y);
	pv++;
	pv->set(w, 0, d_Z, d_W, C, p1.x, p0.y);
	pv++;
	RCache.Vertex.Unlock(4, g_combine->vb_stride);

	// Set geometry
	RCache.set_Geometry(g_combine);

	// Downsample, prepare image and store in rt_Bloom_1 and rt_Bloom_Blades
	{
		u_setrt(rt_Bloom_1, rt_Bloom_Blades_1, NULL, NULL);
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
		u_setrt(rt_Bloom_2, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[1]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Bloom_1, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[2]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// Blades effect
	for (int i = 0; i < 2; i++)
	{
		u_setrt(rt_Bloom_Blades_2, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[3]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

		u_setrt(rt_Bloom_Blades_1, NULL, NULL, NULL);
		RCache.set_Element(s_bloom->E[4]);
		RCache.set_c("bloom_resolution", w, h, 1.0f / w, 1.0f / h);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}