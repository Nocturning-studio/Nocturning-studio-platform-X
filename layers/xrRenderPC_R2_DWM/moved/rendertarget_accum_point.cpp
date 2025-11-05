#include "stdafx.h"

void CRenderTarget::accum_point		(light* L)
{
	phase_accumulator				();
	RImplementation.stats.l_visible	++;

	ref_shader		shader	    = L->s_point;
	if (!shader)
	{
		shader			= s_accum_point;
	}	

	// Common
	Fvector		L_pos;
	float		L_spec;
	//float		L_R					= L->range;
	float		L_R					= L->range*.95f;
	Fvector		L_clr;				L_clr.set		(L->color.r,L->color.g,L->color.b);
	L_spec							= u_diffuse2s	(L_clr);
	Device.mView.transform_tiny		(L_pos,L->position);

	// Xforms
	L->xform_calc					();
	RCache.set_xform_world			(L->m_xform);
	RCache.set_xform_view			(Device.mView);
	RCache.set_xform_project		(Device.mProject);
	enable_scissor					(L);
	enable_dbt_bounds				(L);

	// *****************************	Mask by stencil		*************************************
	// *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
	// *** thus can cope without stencil clear with 127 lights
	// *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
	RCache.set_Element				(s_accum_mask->E[SE_MASK_POINT]);			// masker
	//	Done in blender!
	//RCache.set_ColorWriteEnable		(FALSE);

	// backfaces: if (1<=stencil && zfail)	stencil = light_id
	RCache.set_CullMode(D3D_CULL_FRONT);

	if (RImplementation.o.aa_mode == AA_MSAA)
		RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE);
	else
		RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, dwLightMarkerID, 0x01, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE);

	draw_volume(L);

	// frontfaces: if (1<=stencil && zfail)	stencil = 0x1
	RCache.set_CullMode(D3D_CULL_BACK);

	if (RImplementation.o.aa_mode == AA_MSAA)
		RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, 0x01, 0x7f, 0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE);
	else
		RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, 0x01, 0xff, 0xff, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE);

	draw_volume(L);

	// nv-stencil recompression
	if (RImplementation.o.nvstencil)		u_stencil_optimize();

	// *****************************	Minimize overdraw	*************************************
	// Select shader (front or back-faces), *** back, if intersect near plane
	RCache.set_ColorWriteEnable				();
	RCache.set_CullMode						(D3D_CULL_FRONT);		// back

	// 2D texgens 
	Fmatrix			m_Texgen;			u_compute_texgen_screen	(m_Texgen	);
	Fmatrix			m_Texgen_J;			u_compute_texgen_jitter	(m_Texgen_J	);

	// Draw volume with projective texgen
	{
		// Select shader
		u32 _id = 0;
		if (L->flags.bShadow)
		{
			bool bFullSize = (L->X.S.size == RImplementation.o.smapsize);
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
			//m_Shadow = m_Lmap;
		}
		RCache.set_Element				(shader->E[ _id ]	);

		// Constants
		RCache.set_c					("Ldynamic_pos",	L_pos.x,L_pos.y,L_pos.z,1/(L_R*L_R));
		RCache.set_c					("Ldynamic_color",	L_clr.x,L_clr.y,L_clr.z,L_spec);
		RCache.set_c					("m_texgen",		m_Texgen);

		RCache.set_CullMode				(D3D_CULL_FRONT);

		// Render if (light_id <= stencil && z-pass)
		// nvidia 8000, 9000 and 200 series
		if (RImplementation.o.aa_mode == AA_MSAA &&
			HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
		{
			// per pixel
			RCache.set_c(dx10_msaa_id, 0);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, 0x00);
			draw_volume(L);

			// per sample
			for (u32 i = 0; i < RImplementation.o.msaa_samples; i++)
			{
				RCache.set_c(dx10_msaa_id, (int)i);
				StateManager.SetSampleMask(u32(1) << i);
				RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
				RCache.set_CullMode(D3D_CULL_FRONT);
				draw_volume(L);
			}

			RCache.set_c(dx10_msaa_id, 0);
			StateManager.SetSampleMask(0xffffffff);
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, 0x00);
		}
		else if (RImplementation.o.aa_mode == AA_MSAA)
		{
			// per pixel
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, 0x00);
			draw_volume(L);

			// per sample
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
			RCache.set_CullMode(D3D_CULL_FRONT);
			draw_volume(L);

			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, 0x00);
		}
		else 
		{
			RCache.set_Stencil(TRUE, D3D_COMPARISON_EQUAL, dwLightMarkerID, 0xff, 0x00);
			draw_volume(L);
		}
	}

	//CHK_DX		(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE));
	RCache.set_Scissor(0);

	//dwLightMarkerID					+=	2;	// keep lowest bit always setted up
	increment_light_marker();

	u_DBT_disable				();
}
