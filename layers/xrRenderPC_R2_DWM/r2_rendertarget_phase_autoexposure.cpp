#include "stdafx.h"

#pragma pack(push, 4)
struct v_build_autoexposure
{
	Fvector4 p;
	Fvector2 uv0;
	Fvector2 uv1;
	Fvector2 uv2;
	Fvector2 uv3;
};

struct v_filter_autoexposure
{
	Fvector4 p;
	Fvector4 uv[8];
};
#pragma pack(pop)

#pragma message(Reminder("Fix autoexposure (+tonemappng)"))
/* void CRenderTarget::phase_autoexposure()
{
	u32 Offset = 0;
	float eps = EPS_S;

	// Targets
	RCache.set_Stencil(FALSE);
	RCache.set_CullMode(CULL_NONE);
	RCache.set_ColorWriteEnable();
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, FALSE));

	// 000: Perform LUM-SAT, pass 0, 256x256 => 64x64
	u_setrt(rt_LUM_64, NULL, NULL, NULL);
	{
		float ts = 64;
		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		Fvector2 one = {2.f / _w, 2.f / _h};  // two, infact
		Fvector2 half = {1.f / _w, 1.f / _h}; // one, infact
		Fvector2 a_0 = {half.x + 0, half.y + 0};
		Fvector2 a_1 = {half.x + one.x, half.y + 0};
		Fvector2 a_2 = {half.x + 0, half.y + one.y};
		Fvector2 a_3 = {half.x + one.x, half.y + one.y};
		Fvector2 b_0 = {1 + a_0.x, 1 + a_0.y};
		Fvector2 b_1 = {1 + a_1.x, 1 + a_1.y};
		Fvector2 b_2 = {1 + a_2.x, 1 + a_2.y};
		Fvector2 b_3 = {1 + a_3.x, 1 + a_3.y};

		// Fill vertex buffer
		v_build_autoexposure* pv = (v_build_autoexposure*)RCache.Vertex.Lock(4, g_bloom_build->vb_stride, Offset);
		pv->p.set(eps, float(ts + eps), eps, 1.f);
		pv->uv0.set(a_0.x, b_0.y);
		pv->uv1.set(a_1.x, b_1.y);
		pv->uv2.set(a_2.x, b_2.y);
		pv->uv3.set(a_3.x, b_3.y);
		pv++;
		pv->p.set(eps, eps, eps, 1.f);
		pv->uv0.set(a_0.x, a_0.y);
		pv->uv1.set(a_1.x, a_1.y);
		pv->uv2.set(a_2.x, a_2.y);
		pv->uv3.set(a_3.x, a_3.y);
		pv++;
		pv->p.set(float(ts + eps), float(ts + eps), eps, 1.f);
		pv->uv0.set(b_0.x, b_0.y);
		pv->uv1.set(b_1.x, b_1.y);
		pv->uv2.set(b_2.x, b_2.y);
		pv->uv3.set(b_3.x, b_3.y);
		pv++;
		pv->p.set(float(ts + eps), eps, eps, 1.f);
		pv->uv0.set(b_0.x, a_0.y);
		pv->uv1.set(b_1.x, a_1.y);
		pv->uv2.set(b_2.x, a_2.y);
		pv->uv3.set(b_3.x, a_3.y);
		pv++;
		RCache.Vertex.Unlock(4, g_bloom_build->vb_stride);
		RCache.set_Element(s_autoexposure->E[0]);
		RCache.set_Geometry(g_bloom_build);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// 111: Perform LUM-SAT, pass 1, 64x64 => 8x8
	u_setrt(rt_LUM_8, NULL, NULL, NULL);
	{
		// Build filter-kernel
		float _ts = 8;
		float _src = float(64);
		Fvector2 a[16], b[16];
		for (int k = 0; k < 16; k++)
		{
			int _x = (k * 2 + 1) % 8;	// 1,3,5,7
			int _y = ((k / 4) * 2 + 1); // 1,1,1,1 ~ 3,3,3,3 ~...etc...
			a[k].set(_x, _y).div(_src);
			b[k].set(a[k]).add(1);
		}

		// Fill vertex buffer
		v_filter_autoexposure* pv = (v_filter_autoexposure*)RCache.Vertex.Lock(4, g_bloom_filter->vb_stride, Offset);
		pv->p.set(eps, float(_ts + eps), eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(a[t].x, b[t].y, b[t + 8].y, a[t + 8].x); // xy/yx	- left+down
		pv++;
		pv->p.set(eps, eps, eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(a[t].x, a[t].y, a[t + 8].y, a[t + 8].x); // xy/yx	- left+up
		pv++;
		pv->p.set(float(_ts + eps), float(_ts + eps), eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(b[t].x, b[t].y, b[t + 8].y, b[t + 8].x); // xy/yx	- right+down
		pv++;
		pv->p.set(float(_ts + eps), eps, eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(b[t].x, a[t].y, a[t + 8].y, b[t + 8].x); // xy/yx	- right+up
		pv++;
		RCache.Vertex.Unlock(4, g_bloom_filter->vb_stride);
		RCache.set_Element(s_autoexposure->E[1]);
		RCache.set_Geometry(g_bloom_filter);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// 222: Perform LUM-SAT, pass 2, 8x8 => 1x1
	u32 gpu_id = Device.dwFrame % HW.Caps.iGPUNum;
	u_setrt(rt_LUM_pool[gpu_id * 2 + 1], NULL, NULL, NULL);
	{
		// Build filter-kernel
		float _ts = 1;
		float _src = float(8);
		Fvector2 a[16], b[16];
		for (int k = 0; k < 16; k++)
		{
			int _x = (k * 2 + 1) % 8;	// 1,3,5,7
			int _y = ((k / 4) * 2 + 1); // 1,1,1,1 ~ 3,3,3,3 ~...etc...
			a[k].set(_x, _y).div(_src);
			b[k].set(a[k]).add(1);
		}

		// Fill vertex buffer
		v_filter_autoexposure* pv = (v_filter_autoexposure*)RCache.Vertex.Lock(4, g_bloom_filter->vb_stride, Offset);
		pv->p.set(eps, float(_ts + eps), eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(a[t].x, b[t].y, b[t + 8].y, a[t + 8].x); // xy/yx	- left+down
		pv++;
		pv->p.set(eps, eps, eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(a[t].x, a[t].y, a[t + 8].y, a[t + 8].x); // xy/yx	- left+up
		pv++;
		pv->p.set(float(_ts + eps), float(_ts + eps), eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(b[t].x, b[t].y, b[t + 8].y, b[t + 8].x); // xy/yx	- right+down
		pv++;
		pv->p.set(float(_ts + eps), eps, eps, 1.f);
		for (int t = 0; t < 8; t++)
			pv->uv[t].set(b[t].x, a[t].y, a[t + 8].y, b[t + 8].x); // xy/yx	- right+up
		pv++;
		RCache.Vertex.Unlock(4, g_bloom_filter->vb_stride);

		f_autoexposure_adapt = .9f * f_autoexposure_adapt + .1f * Device.fTimeDelta * ps_r2_autoexposure_adaptation;
		float amount = ps_r2_postprocess_flags.test(R2FLAG_AUTOEXPOSURE) ? ps_r2_autoexposure_amount : 0;
		Fvector3 _none, _full, _result;
		_none.set(1, 0, 1);
		_full.set(ps_r2_autoexposure_middlegray, 1.f, ps_r2_autoexposure_low_lum);
		_result.lerp(_none, _full, amount);

		RCache.set_Element(s_autoexposure->E[2]);
		RCache.set_Geometry(g_bloom_filter);
		RCache.set_c("MiddleGray", _result.x, _result.y, _result.z, f_autoexposure_adapt);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// Cleanup states
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));

	{
		u_setrt(rt_Generic_0, NULL, NULL, NULL);

		RCache.set_CullMode(CULL_NONE);
		RCache.set_Stencil(FALSE);

		// Constants
		u32 Offset = 0;
		u32 C = color_rgba(0, 0, 0, 255);

		float w = float(Device.dwWidth);
		float h = float(Device.dwHeight);

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

		// Set pass
		RCache.set_Element(s_autoexposure->E[3]);

		// Set geometry
		RCache.set_Geometry(g_combine);

		// Draw
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}
}
*/