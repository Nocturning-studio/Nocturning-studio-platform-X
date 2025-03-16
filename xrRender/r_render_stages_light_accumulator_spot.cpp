#include "stdafx.h"

extern Fvector du_cone_vertices[DU_CONE_NUMVERTEX];

void CRenderTarget::accum_spot(light* L)
{
	phase_accumulator();
	RenderImplementation.stats.l_visible++;

	// *** assume accumulator setted up ***
	// *****************************	Mask by stencil		*************************************
	ref_shader shader;
	if (IRender_Light::OMNIPART == L->flags.type)
	{
		shader = L->s_point;
		if (!shader)
			shader = s_accum_point;
	}
	else
	{
		shader = L->s_spot;
		if (!shader)
			shader = s_accum_spot;
	}

	BOOL bIntersect = FALSE; // enable_scissor(L);
	{
		// setup xform
		L->xform_calc();
		RenderBackend.set_xform_world(L->m_xform);
		RenderBackend.set_xform_view(Device.mView);
		RenderBackend.set_xform_project(Device.mProject);
		bIntersect = enable_scissor(L);
		enable_dbt_bounds(L);

		// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
		// *** thus can cope without stencil clear with 127 lights
		// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
		RenderBackend.set_ColorWriteEnable(FALSE);
		RenderBackend.set_Element(s_accum_mask->E[SE_MASK_SPOT]); // masker

		// backfaces: if (stencil>=1 && zfail)			stencil = light_id
		RenderBackend.set_CullMode(CULL_CW);
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
						   D3DSTENCILOP_REPLACE);
		draw_volume(L);

		// frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
		RenderBackend.set_CullMode(CULL_CCW);
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
						   D3DSTENCILOP_REPLACE);
		draw_volume(L);
	}

	// nv-stencil recompression
	if (RenderImplementation.o.nvstencil)
		u_stencil_optimize();

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RenderBackend.set_ColorWriteEnable();
	RenderBackend.set_CullMode(CULL_CW); // back

	// 2D texgens
	Fmatrix m_Texgen;
	u_compute_texgen_screen(m_Texgen);
	Fmatrix m_Texgen_J;
	u_compute_texgen_jitter(m_Texgen_J);

	// Shadow xform (+texture adjustment matrix)
	Fmatrix m_Shadow, m_Lmap;
	{
		float smapsize = float(RenderImplementation.o.smapsize);
		float fTexelOffs = (.5f / smapsize);
		float view_dim = float(L->X.S.size - 2) / smapsize;
		float view_sx = float(L->X.S.posX + 1) / smapsize;
		float view_sy = float(L->X.S.posY + 1) / smapsize;
		float fRange = float(1.f) * ps_r_ls_depth_scale;
		float fBias = ps_r_ls_depth_bias;
		Fmatrix m_TexelAdjust = {view_dim / 2.f,
								 0.0f,
								 0.0f,
								 0.0f,
								 0.0f,
								 -view_dim / 2.f,
								 0.0f,
								 0.0f,
								 0.0f,
								 0.0f,
								 fRange,
								 0.0f,
								 view_dim / 2.f + view_sx + fTexelOffs,
								 view_dim / 2.f + view_sy + fTexelOffs,
								 fBias,
								 1.0f};

		// compute xforms
		Fmatrix xf_world;
		xf_world.invert(Device.mView);
		Fmatrix xf_view = L->X.S.view;
		Fmatrix xf_project;
		xf_project.mul(m_TexelAdjust, L->X.S.project);
		m_Shadow.mul(xf_view, xf_world);
		m_Shadow.mulA_44(xf_project);

		// lmap
		view_dim = 1.f;
		view_sx = 0.f;
		view_sy = 0.f;
		Fmatrix m_TexelAdjust2 = {view_dim / 2.f,
								  0.0f,
								  0.0f,
								  0.0f,
								  0.0f,
								  -view_dim / 2.f,
								  0.0f,
								  0.0f,
								  0.0f,
								  0.0f,
								  fRange,
								  0.0f,
								  view_dim / 2.f + view_sx + fTexelOffs,
								  view_dim / 2.f + view_sy + fTexelOffs,
								  fBias,
								  1.0f};

		// compute xforms
		xf_project.mul(m_TexelAdjust2, L->X.S.project);
		m_Lmap.mul(xf_view, xf_world);
		m_Lmap.mulA_44(xf_project);
	}

	// Common constants
	Fvector L_dir, L_clr, L_pos;
	float L_spec;
	L_clr.set(L->color.r, L->color.g, L->color.b);
	L_clr.mul(L->get_LOD());
	L_spec = u_diffuse2s(L_clr);
	Device.mView.transform_tiny(L_pos, L->position);
	Device.mView.transform_dir(L_dir, L->direction);
	L_dir.normalize();

	// Draw volume with projective texgen
	{
		// Select shader
		u32 _id = 0;
		if (L->flags.bShadow)
		{
			bool bFullSize = (L->X.S.size == RenderImplementation.o.smapsize);
			if (L->X.S.transluent)
				_id = SE_L_TRANSLUENT;
			else if (bFullSize)
				_id = SE_L_FULLSIZE;
			else
				_id = SE_L_NORMAL;
		}
		else
		{
			_id = SE_L_UNSHADOWED;
			m_Shadow = m_Lmap;
		}
		RenderBackend.set_Element(shader->E[_id]);

		// Constants
		float att_R = L->range * .95f;
		float att_factor = 1.f / (att_R * att_R);
		RenderBackend.set_Constant("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, att_factor);
		RenderBackend.set_Constant("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z), L_spec);
		RenderBackend.set_Constant("m_texgen", m_Texgen);
		RenderBackend.set_Constant("m_texgen_J", m_Texgen_J);
		RenderBackend.set_Constant("m_shadow", m_Shadow);
		RenderBackend.set_Array_Constant("m_lmap", 0, m_Lmap._11, m_Lmap._21, m_Lmap._31, m_Lmap._41);
		RenderBackend.set_Array_Constant("m_lmap", 1, m_Lmap._12, m_Lmap._22, m_Lmap._32, m_Lmap._42);

		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
		draw_volume(L);
	}

	dwLightMarkerID += 2; // keep lowest bit always setted up
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE));

	u_DBT_disable();
}
