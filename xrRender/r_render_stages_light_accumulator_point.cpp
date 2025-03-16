#include "stdafx.h"

void CRenderTarget::accum_point(light* L)
{
	phase_accumulator();
	RenderImplementation.stats.l_visible++;

	ref_shader shader = L->s_point;
	if (!shader)
		shader = s_accum_point;

	// Common
	Fvector L_pos;
	float L_spec;
	float L_R = L->range;
	Fvector L_clr;
	L_clr.set(L->color.r, L->color.g, L->color.b);
	L_spec = u_diffuse2s(L_clr);
	Device.mView.transform_tiny(L_pos, L->position);

	// Xforms
	L->xform_calc();
	RenderBackend.set_xform_world(L->m_xform);
	RenderBackend.set_xform_view(Device.mView);
	RenderBackend.set_xform_project(Device.mProject);
	enable_scissor(L);
	enable_dbt_bounds(L);

	// *****************************	Mask by stencil		*************************************
	// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
	// *** thus can cope without stencil clear with 127 lights
	// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
	RenderBackend.set_Element(s_accum_mask->E[SE_MASK_POINT]); // masker
	RenderBackend.set_ColorWriteEnable(FALSE);

	// backfaces: if (stencil>=1 && zfail)	stencil = light_id
	RenderBackend.set_CullMode(CULL_CW);
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
					   D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
					   D3DSTENCILOP_REPLACE);
	draw_volume(L);

	// nv-stencil recompression
	if (RenderImplementation.o.nvstencil)
		u_stencil_optimize();

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RenderBackend.set_ColorWriteEnable();
	RenderBackend.set_CullMode(CULL_CW); // back
	/*
	if (bIntersect)	RenderBackend.set_CullMode		(CULL_CW);		// back
	else			RenderBackend.set_CullMode		(CULL_CCW);		// front
	*/

	// 2D texgens
	Fmatrix m_Texgen;
	u_compute_texgen_screen(m_Texgen);
	Fmatrix m_Texgen_J;
	u_compute_texgen_jitter(m_Texgen_J);

	// Draw volume with projective texgen
	{
		// Select shader
		u32 _id = 0;
		if (L->flags.bShadow)
		{
			bool bFullSize = (L->X.S.size == u32(RenderImplementation.o.smapsize));
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
			// m_Shadow				= m_Lmap;
		}
		RenderBackend.set_Element(shader->E[_id]);

		// Constants
		RenderBackend.set_Constant("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L_R * L_R));
		RenderBackend.set_Constant("Ldynamic_color", sRgbToLinear(L_clr.x), sRgbToLinear(L_clr.y), sRgbToLinear(L_clr.z), L_spec);
		RenderBackend.set_Constant("m_texgen", m_Texgen);

		// Render if (stencil >= light_id && z-pass)
		RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
						   D3DSTENCILOP_KEEP);
		draw_volume(L);
	}

	dwLightMarkerID += 2; // keep lowest bit always setted up
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE));

	u_DBT_disable();
}
