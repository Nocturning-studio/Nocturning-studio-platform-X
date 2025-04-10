#include "stdafx.h"

IC bool pred_area(light* _1, light* _2)
{
	u32 a0 = _1->X.S.size;
	u32 a1 = _2->X.S.size;
	return a0 > a1; // reverse -> descending
}

void CRender::render_lights(light_Package& LP)
{
	OPTICK_EVENT("CRender::render_lights");

	//////////////////////////////////////////////////////////////////////////
	// Refactor order based on ability to pack shadow-maps
	// 1. calculate area + sort in descending order
	// const	u16		smap_unassigned		= u16(-1);
	{
		OPTICK_EVENT("CRender::render_lights - Refactor order based");

		xr_vector<light*>& source = LP.v_shadowed;
		for (u32 it = 0; it < source.size(); it++)
		{
			light* L = source[it];
			L->vis_update();
			if (!L->vis.visible)
			{
				source.erase(source.begin() + it);
				it--;
			}
			else
			{
				LR.compute_xf_spot(L);
			}
		}
	}

	// 2. refactor - infact we could go from the backside and sort in ascending order
	{
		OPTICK_EVENT("CRender::render_lights - refactor");

		xr_vector<light*>& source = LP.v_shadowed;
		xr_vector<light*> refactored;
		refactored.reserve(source.size());
		u32 total = source.size();

		for (u16 smap_ID = 0; refactored.size() != total; smap_ID++)
		{
			LP_smap_pool.initialize(RenderImplementation.o.smapsize);
			concurrency::parallel_sort(source.begin(), source.end(), pred_area);
			for (u32 test = 0; test < source.size(); test++)
			{
				light* L = source[test];
				SMAP_Rect R;
				if (LP_smap_pool.push(R, L->X.S.size))
				{
					// OK
					L->X.S.posX = R.min.x;
					L->X.S.posY = R.min.y;
					L->vis.smap_ID = smap_ID;
					refactored.push_back(L);
					source.erase(source.begin() + test);
					test--;
				}
			}
		}

		// save (lights are popped from back)
		std::reverse(refactored.begin(), refactored.end());
		LP.v_shadowed = refactored;
	}

	//////////////////////////////////////////////////////////////////////////
	// sort lights by importance???
	// while (has_any_lights_that_cast_shadows) {
	//		if (has_point_shadowed)		->	generate point shadowmap
	//		if (has_spot_shadowed)		->	generate spot shadowmap
	//		switch-to-accumulator
	//		if (has_point_unshadowed)	-> 	accum point unshadowed
	//		if (has_spot_unshadowed)	-> 	accum spot unshadowed
	//		if (was_point_shadowed)		->	accum point shadowed
	//		if (was_spot_shadowed)		->	accum spot shadowed
	//	}
	//	if (left_some_lights_that_doesn't cast shadows)
	//		accumulate them
	HOM.Disable();
	while (LP.v_shadowed.size())
	{
		// if (has_spot_shadowed)
		xr_vector<light*> L_spot_s;
		stats.s_used++;

		// generate spot shadowmap
		clear_shadow_map_spot();
		xr_vector<light*>& source = LP.v_shadowed;
		light* L = source.back();
		u16 sid = L->vis.smap_ID;
		while (true)
		{
			if (source.empty())
				break;
			L = source.back();
			if (L->vis.smap_ID != sid)
				break;
			source.pop_back();
			Lights_LastFrame.push_back(L);

			// render
			set_active_phase(PHASE_SHADOW_DEPTH);
			r_pmask(true, false);
			L->svis.begin();
			r_dsgraph_render_subspace(L->spatial.sector, L->X.S.combine, L->position, TRUE);
			bool bNormal = mapNormal[0].size() || mapMatrix[0].size();
			bool bSpecial = mapNormal[1].size() || mapMatrix[1].size() || mapSorted.size();
			if (bNormal || bSpecial)
			{
				stats.s_merged++;
				L_spot_s.push_back(L);
				render_shadow_map_spot(L);
				RenderBackend.set_xform_world(Fidentity);
				RenderBackend.set_xform_view(L->X.S.view);
				RenderBackend.set_xform_project(L->X.S.project);
				if (ps_r_lighting_flags.test(RFLAG_SUN_DETAILS))
				{
					Details->UpdateVisibleM();
					Details->Render();
				}
				r_dsgraph_render_graph(0);
				L->X.S.transluent = FALSE;
				if (bSpecial)
				{
					L->X.S.transluent = TRUE;
					render_shadow_map_spot_transluent(L);
					r_dsgraph_render_graph(1); // normal level, secondary priority
					r_dsgraph_render_sorted(); // strict-sorted geoms
				}
			}
			else
			{
				stats.s_finalclip++;
			}
			L->svis.end();
			r_pmask(true, false);
		}

		//		switch-to-accumulator
		set_light_accumulator();
		HOM.Disable();

		//		if (has_point_unshadowed)	-> 	accum point unshadowed
		if (!LP.v_point.empty())
		{
			OPTICK_EVENT("CRender::render_lights - accum point");

			light* LightPoint = LP.v_point.back();
			LP.v_point.pop_back();
			LightPoint->vis_update();
			if (LightPoint->vis.visible)
			{
				accumulate_point_lights(LightPoint);
			}
		}

		//		if (has_spot_unshadowed)	-> 	accum spot unshadowed
		if (!LP.v_spot.empty())
		{
			OPTICK_EVENT("CRender::render_lights - accum spot");

			light* LightSpot = LP.v_spot.back();
			LP.v_spot.pop_back();
			LightSpot->vis_update();
			if (LightSpot->vis.visible)
			{
				LR.compute_xf_spot(LightSpot);
				accumulate_spot_lights(LightSpot);
			}
		}

		//		if (was_spot_shadowed)		->	accum spot shadowed
		if (!L_spot_s.empty())
		{
			OPTICK_EVENT("CRender::render_lights - accum spot shadowed");

			for (u32 it = 0; it < L_spot_s.size(); it++)
			{
				accumulate_spot_lights(L_spot_s[it]);
			}

			// if (ps_r_lighting_flags.is(RFLAG_VOLUMETRIC_LIGHTS)
			//	for (u32 it = 0; it < L_spot_s.size(); it++)
			//		RenderTarget->accum_volumetric(L_spot_s[it]);

			L_spot_s.clear();
		}
	}

	// Point lighting (unshadowed, if left)
	if (!LP.v_point.empty())
	{
		OPTICK_EVENT("CRender::render_lights - point");

		xr_vector<light*>& Lvec = LP.v_point;
		for (u32 pid = 0; pid < Lvec.size(); pid++)
		{
			Lvec[pid]->vis_update();
			if (Lvec[pid]->vis.visible)
			{
				accumulate_point_lights(Lvec[pid]);
			}
		}
		Lvec.clear();
	}

	// Spot lighting (unshadowed, if left)
	if (!LP.v_spot.empty())
	{
		OPTICK_EVENT("CRender::render_lights - spot");

		xr_vector<light*>& Lvec = LP.v_spot;
		for (u32 pid = 0; pid < Lvec.size(); pid++)
		{
			Lvec[pid]->vis_update();
			if (Lvec[pid]->vis.visible)
			{
				LR.compute_xf_spot(Lvec[pid]);
				accumulate_spot_lights(Lvec[pid]);
			}
		}
		Lvec.clear();
	}
}
