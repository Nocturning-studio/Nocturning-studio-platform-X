#include "stdafx.h"

static const float msaa_sample_pattern[34][2] =
{
	{ 0.500f,  0.500f }, {-0.500f, -0.500f },
	{-0.250f, -0.750f }, { 0.750f, -0.250f },
	{-0.750f,  0.250f }, { 0.250f,  0.750f },
	{ 0.125f, -0.375f }, {-0.125f,  0.375f },
	{ 0.625f,  0.125f }, {-0.375f, -0.625f },
	{-0.625f,  0.625f }, {-0.875f, -0.125f },
	{ 0.375f,  0.875f }, { 0.875f, -0.875f },
	{ 0.125f,  0.125f }, {-0.125f, -0.375f },
	{-0.375f,  0.250f }, { 0.500f, -0.125f },
	{-0.625f, -0.250f }, { 0.250f,  0.625f },
	{ 0.625f,  0.375f }, { 0.375f, -0.625f },
	{-0.250f,  0.750f }, { 0.000f, -0.875f },
	{-0.500f, -0.750f }, {-0.750f,  0.500f },
	{-1.000f,  0.000f }, { 0.875f, -0.500f },
	{ 0.750f,  0.875f }, {-0.875f, -1.000f }
};

float msaa_curr_offset[2] = { 0, 0 };

u32 TAA_samples = 0, TAA_use_opt = 0, TAA_need_opt = 0;

extern float hclip(float v, float dim);

void CRenderTarget::phase_TAA_define(u32* pTAA_samples)
{
	switch (ps_r2_aa)
	{
	case AA_TAA16S: *pTAA_samples = 16; break;
	case AA_TAA8S: *pTAA_samples = 8; break;
	case AA_TAA4S: *pTAA_samples = 4; break;
	case AA_TAA2S: *pTAA_samples = 2; break;
	};
}

void CRenderTarget::phase_TAA_prepare()
{
	u32 add_tab;

	switch (TAA_samples)
	{
	case 16: add_tab = 6; break;
	case  8: add_tab = 4; break;
	case  4: add_tab = 2; break;
	case  2: add_tab = 0; break;
	default: msaa_curr_offset[0] = msaa_curr_offset[1] = 0; return;
	};

	u_setrt(rt_TAA_mask_curr);
	CHK_DX(HW.pDevice->Clear(0l, NULL, D3DCLEAR_TARGET, 0x0, 1.0f, 0l));

	u_setrt(rt_Position);
	CHK_DX(HW.pDevice->Clear(0l, NULL, D3DCLEAR_TARGET, 0x0, 1.0f, 0l));

	for (u32 i = 0; i < 2; i++)
		msaa_curr_offset[i] = msaa_sample_pattern[Device.dwFrame % TAA_samples + add_tab][i];
}

void CRenderTarget::phase_TAA_apply()
{
	if (!TAA_samples)
		return;

	static Fmatrix m_vp_prev[15] = {};

	u32 curr_samples = TAA_samples - 1;

	u32 bias = 0;

	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);

	Fvector2 p0, p1;
	p0.set(0.5f / _w, 0.5f / _h);
	p1.set(1 + p0.x, 1 + p0.y);
	
	Fvector4* pv = (Fvector4*)RCache.Vertex.Lock(4, g_combine_VP->vb_stride, bias);

	pv->set(hclip(EPS,		_w),	hclip(_h + EPS,	_h),	p0.x, p1.y);	pv++;
	pv->set(hclip(EPS,		_w),	hclip(EPS,		_h),	p0.x, p0.y);	pv++;
	pv->set(hclip(_w + EPS,	_w),	hclip(_h + EPS,	_h),	p1.x, p1.y);	pv++;
	pv->set(hclip(_w + EPS,	_w),	hclip(EPS,		_h),	p1.x, p0.y);	pv++;

	RCache.Vertex.Unlock(4, g_combine_VP->vb_stride);

	u_setrt(rt_TAA_temp);

	RCache.set_Element(s_taa->E[SE_TAA_OPT]);
	RCache.set_Geometry(g_combine_VP);

	for (u32 i = 0; i < curr_samples; i++)
	{
		string16 name;
		sprintf_s(name, "%s%u", "m_VP_", i + 1);
		RCache.set_c(name, m_vp_prev[i]);
	}

	Fmatrix m_v2w;
	m_v2w.invert(Device.mView);
	RCache.set_c("m_v2w", m_v2w);

	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	u32 sample_id = Device.dwFrame % curr_samples;

	u_setrt(rt_TAA[sample_id]);
	RCache.set_Element(s_taa->E[SE_TAA_FRAME_COPY_OPT]);
	RCache.set_Geometry(g_combine_VP);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	u_setrt(rt_Color);
	RCache.set_Element(s_taa->E[SE_TAA_IMG_COPY_OPT]);
	RCache.set_Geometry(g_combine_VP);
	RCache.Render(D3DPT_TRIANGLELIST, bias, 0, 4, 0, 2);

	m_vp_prev[sample_id].set(Device.mFullTransform);
}
