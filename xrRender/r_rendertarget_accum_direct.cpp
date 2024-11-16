#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#include "r_rendertarget.h"

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3 corners[8] = {{-1, -1, 0.7}, {-1, -1, +1},	{-1, +1, +1}, {-1, +1, 0.7},
							  {+1, +1, +1},	 {+1, +1, 0.7}, {+1, -1, +1}, {+1, -1, 0.7}};
static u16 facetable[16][3] = {
	{3, 2, 1}, {3, 1, 0}, {7, 6, 5}, {5, 6, 4}, {3, 5, 2}, {4, 2, 5}, {1, 6, 7}, {7, 0, 1},

	{5, 3, 0}, {7, 5, 0},

	{1, 4, 6}, {2, 4, 1},
};

void CRenderTarget::accum_direct_cascade(u32 sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias)
{
	// Choose normal code-path or filtered
	phase_accumulator();

	// *** assume accumulator setted up ***
	light* sun = (light*)RImplementation.Lights.sun_adapted._get();

	// Common calc for quad-rendering
	u32 Offset;
	u32 C = color_rgba(255, 255, 255, 255);
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	Fvector2 p0, p1;
	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h);
	float d_Z = EPS_S, d_W = 1.f;

	// Common constants (light-related)
	Fvector L_dir, L_clr;
	float L_spec;
	L_clr.set(sun->color.r, sun->color.g, sun->color.b);
	L_spec = u_diffuse2s(L_clr);
	Device.mView.transform_dir(L_dir, sun->direction);
	L_dir.normalize();

	// Perform masking (only once - on the first/near phase)
	RCache.set_CullMode(CULL_NONE);
	if (SE_SUN_NEAR == sub_phase) //.
	{
		// Fill vertex buffer
		FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
		pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
		pv++;
		pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
		pv++;
		pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
		pv++;
		pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
		pv++;
		RCache.Vertex.Unlock(4, g_combine->vb_stride);
		RCache.set_Geometry(g_combine);

		// setup
		RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]); // masker
		RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);

		// if (stencil>=1 && aref_pass)	stencil = light_id
		RCache.set_ColorWriteEnable(FALSE);
		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
						   D3DSTENCILOP_KEEP);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// recalculate d_Z, to perform depth-clipping
	Fvector center_pt;
	center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, ps_r_sun_near);
	Device.mFullTransform.transform(center_pt);
	d_Z = center_pt.z;

	// nv-stencil recompression
	if (RImplementation.o.nvstencil && (SE_SUN_NEAR == sub_phase))
		u_stencil_optimize();

	// Perform lighting
	//******************************************************************
	{
		phase_accumulator();
		RCache.set_CullMode(CULL_CCW); //******************************************************************
		RCache.set_ColorWriteEnable();

		// texture adjustment matrix
		float fTexelOffs = (0.5f / float(RImplementation.o.smapsize));

		float fRange = 0.0f;
		float fBias = 0.0f;

		switch (sub_phase)
		{
		case SE_SUN_NEAR:
			fBias = ps_r_sun_depth_near_bias;
			fRange = ps_r_sun_depth_near_scale;
			break;
		case SE_SUN_MIDDLE:
			fBias = ps_r_sun_depth_middle_bias;
			fRange = ps_r_sun_depth_middle_scale;
			break;
		case SE_SUN_FAR:
			fBias = ps_r_sun_depth_far_bias;
			fRange = ps_r_sun_depth_far_scale;
			break;
		}

		Fmatrix m_TexelAdjust = {0.5f,
								 0.0f,
								 0.0f,
								 0.0f,
								 0.0f,
								 -0.5f,
								 0.0f,
								 0.0f,
								 0.0f,
								 0.0f,
								 fRange,
								 0.0f,
								 0.5f + fTexelOffs,
								 0.5f + fTexelOffs,
								 fBias,
								 1.0f};

		// compute xforms
		FPU::m64r();
		Fmatrix xf_invview;
		xf_invview.invert(Device.mView);

		// shadow xform
		Fmatrix m_shadow;
		{
			Fmatrix xf_project;
			xf_project.mul(m_TexelAdjust, sun->X.D.combine);
			m_shadow.mul(xf_project, xf_invview);

			// tsm-bias
			{
				Fvector bias;
				bias.mul(L_dir, ps_r_sun_tsm_bias);
				Fmatrix bias_t;
				bias_t.translate(bias);
				m_shadow.mulB_44(bias_t);
			}
			FPU::m24r();
		}

		Fmatrix m_Texgen;
		m_Texgen.identity();
		RCache.xforms.set_W(m_Texgen);
		RCache.xforms.set_V(Device.mView);
		RCache.xforms.set_P(Device.mProject);
		u_compute_texgen_screen(m_Texgen);

		// Fill vertex buffer
		u32 i_offset;
		{
			u16* pib = RCache.Index.Lock(sizeof(facetable) / sizeof(u16), i_offset);
			CopyMemory(pib, &facetable, sizeof(facetable));
			RCache.Index.Unlock(sizeof(facetable) / sizeof(u16));

			// corners
			u32 ver_count = sizeof(corners) / sizeof(Fvector3);
			FVF::L* pv = (FVF::L*)RCache.Vertex.Lock(ver_count, g_combine_cuboid.stride(), Offset);

			Fmatrix inv_XDcombine;
			if ( sub_phase == SE_SUN_FAR)
				inv_XDcombine.invert(xform_prev);
			else
				inv_XDcombine.invert(xform);

			for (u32 i = 0; i < ver_count; ++i)
			{
				Fvector3 tmp_vec;
				inv_XDcombine.transform(tmp_vec, corners[i]);
				pv->set(tmp_vec, C);
				pv++;
			}
			RCache.Vertex.Unlock(ver_count, g_combine_cuboid.stride());
		}

		RCache.set_Geometry(g_combine_cuboid);

		// setup
		RCache.set_Element(s_accum_direct_cascade->E[sub_phase]);

		RCache.set_c("m_texgen", m_Texgen);
		RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
		RCache.set_c("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z), L_spec);
		RCache.set_c("m_shadow", m_shadow);

		// nv-DBT
		float zMin, zMax;
		if (SE_SUN_NEAR == sub_phase)
		{
			zMin = 0;
			zMax = ps_r_sun_near;
		}
		else
		{
			extern float ps_r_sun_far;
			zMin = ps_r_sun_near;
			zMax = ps_r_sun_far;
		}

		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMin);
		Device.mFullTransform.transform(center_pt);
		zMin = center_pt.z;

		center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, zMax);
		Device.mFullTransform.transform(center_pt);
		zMax = center_pt.z;

		if (u_DBT_enable(zMin, zMax))
		{
			// z-test always
			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
			HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		}

		// Enable Z function only for near and middle cascades, the far one is restricted by only stencil.
		if ((SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase))
			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
		else if (!ps_r_lighting_flags.is(RFLAGEXT_SUN_ZCULLING))
			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
		else
			HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);

		// setup stencil
		if (SE_SUN_NEAR == sub_phase || sub_phase == SE_SUN_MIDDLE)
			RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0xFE, D3DSTENCILOP_KEEP,
							   D3DSTENCILOP_ZERO, D3DSTENCILOP_KEEP);
		else
			RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);

		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 8, 0, 16);

		// disable depth bounds
		u_DBT_disable();
	}
}
