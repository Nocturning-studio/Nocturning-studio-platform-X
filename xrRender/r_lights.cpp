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
	// 1. ���������������� ���������� � ���������� ������� ����������
	{
		OPTICK_EVENT("CRender::render_lights - Refactor order based");

		xr_vector<light*>& source = LP.v_shadowed;

		// ����� ����������� �������� ��������� ����������
		source.erase(std::remove_if(source.begin(), source.end(),
									[this](light* L) {
										L->vis_update();
										if (!L->vis.visible)
											return true;

										LR.compute_xf_spot(L);
										return false;
									}),
					 source.end());
	}

	// 2. ���������������� �������� shadow maps
	{
		OPTICK_EVENT("CRender::render_lights - refactor");

		xr_vector<light*>& source = LP.v_shadowed;
		if (source.empty())
			return;

		xr_vector<light*> refactored;
		refactored.reserve(source.size());

		// ��������� ������ ���� ��� � ������
		if (source.size() > 8)
		{
			concurrency::parallel_sort(source.begin(), source.end(), pred_area);
		}
		else
		{
			std::sort(source.begin(), source.end(), pred_area);
		}

		for (u16 smap_ID = 0; !source.empty(); smap_ID++)
		{
			LP_smap_pool.initialize(RenderImplementation.o.smapsize);

			// ����� ����������� �������� ��������
			for (auto it = source.begin(); it != source.end();)
			{
				light* L = *it;
				SMAP_Rect R;
				if (LP_smap_pool.push(R, L->X.S.size))
				{
					L->X.S.posX = R.min.x;
					L->X.S.posY = R.min.y;
					L->vis.smap_ID = smap_ID;
					refactored.push_back(L);
					it = source.erase(it);
				}
				else
				{
					// ���� �� ����� ��������� ������� (����� �������), ��������� � ���������� smap_ID
					if (it == source.begin())
						break;
					++it;
				}
			}
		}

		// save (lights are popped from back)
		std::reverse(refactored.begin(), refactored.end());
		LP.v_shadowed = std::move(refactored); // ���������� move ��� �������������
	}

	//////////////////////////////////////////////////////////////////////////
	// 3. ���������������� ������ �����
	HOM.Disable();

	while (!LP.v_shadowed.empty())
	{
		OPTICK_EVENT("CRender::render_lights - Shadow map rendering");

		stats.s_used++;
		clear_shadow_map_spot();

		// ����������� ���������� �� smap_ID ��� batch ���������
		xr_vector<light*> current_batch;
		xr_vector<light*>& source = LP.v_shadowed;
		u16 current_sid = source.back()->vis.smap_ID;

		// ��������� ��� ������ � ���������� smap_ID
		while (!source.empty() && source.back()->vis.smap_ID == current_sid)
		{
			current_batch.push_back(source.back());
			source.pop_back();
		}
		Lights_LastFrame.insert(Lights_LastFrame.end(), current_batch.begin(), current_batch.end());

		// Batch ������ shadow maps ��� ���� ������
		set_active_phase(PHASE_SHADOW_DEPTH);
		r_pmask(true, false);

		for (light* L : current_batch)
		{
			L->svis.begin();
			r_dsgraph_render_subspace(L->spatial.sector, L->X.S.combine, L->position, TRUE);

			bool bNormal = mapNormal[0].size() || mapMatrix[0].size();
			bool bSpecial = mapNormal[1].size() || mapMatrix[1].size() || mapSorted.size();

			if (bNormal || bSpecial)
			{
				stats.s_merged++;
				render_shadow_map_spot(L);
				RenderBackend.set_xform_world(Fidentity);
				RenderBackend.set_xform_view(L->X.S.view);
				RenderBackend.set_xform_project(L->X.S.project);

				if (ps_r_lighting_flags.test(RFLAG_SUN_DETAILS))
				{
					//Details->UpdateVisibleM();
					Details->Render();
				}

				r_dsgraph_render_graph(0);
				L->X.S.transluent = FALSE;

				if (bSpecial)
				{
					L->X.S.transluent = TRUE;
					render_shadow_map_spot_transluent(L);
					r_dsgraph_render_graph(1);
					r_dsgraph_render_sorted();
				}
			}
			else
			{
				stats.s_finalclip++;
			}

			L->svis.end();
		}

		r_pmask(true, false);

		// 4. ���������������� ���������� �����
		{
			OPTICK_EVENT("CRender::render_lights - Light accumulation");

			set_light_accumulator();
			HOM.Disable();

			// ������� ���������� point lights
			if (!LP.v_point.empty())
			{
				OPTICK_EVENT("CRender::render_lights - accum point batch");

				// ������������ ��������� ���������� �� ������
				for (size_t i = 0; i < LP.v_point.size();)
				{
					light* L = LP.v_point[i];
					L->vis_update();

					if (L->vis.visible)
					{
						accumulate_point_lights(L);
						// ����������� ��������
						LP.v_point[i] = LP.v_point.back();
						LP.v_point.pop_back();
					}
					else
					{
						i++;
					}
				}
			}

			// ������� ���������� spot lights
			if (!LP.v_spot.empty())
			{
				OPTICK_EVENT("CRender::render_lights - accum spot batch");

				for (size_t i = 0; i < LP.v_spot.size();)
				{
					light* L = LP.v_spot[i];
					L->vis_update();

					if (L->vis.visible)
					{
						LR.compute_xf_spot(L);
						accumulate_spot_lights(L);
						LP.v_spot[i] = LP.v_spot.back();
						LP.v_spot.pop_back();
					}
					else
					{
						i++;
					}
				}
			}

			// ���������� ������� ���������� �� ������� ������
			if (!current_batch.empty())
			{
				OPTICK_EVENT("CRender::render_lights - accum spot shadowed");

				for (light* L : current_batch)
				{
					accumulate_spot_lights(L);
				}

				// Volumetric lights (���� ��������)
				// if (ps_r_lighting_flags.test(RFLAG_VOLUMETRIC_LIGHTS))
				// {
				//     for (light* L : current_batch)
				//     {
				//         RenderTarget->accum_volumetric(L);
				//     }
				// }

				current_batch.clear();
			}
		}
	}

	// 5. ���������������� ��������� ���������� �������� ����������
	ProcessRemainingLightsOptimized(LP);
}

// ��������������� ������� ��� ��������� ���������� ����������
void CRender::ProcessRemainingLightsOptimized(light_Package& LP)
{
	OPTICK_EVENT("CRender::ProcessRemainingLightsOptimized");

	// Point lights
	if (!LP.v_point.empty())
	{
		OPTICK_EVENT("CRender::render_lights - remaining point");

		// �������� ���������� ���������
		for (light* L : LP.v_point)
		{
			L->vis_update();
		}

		// ���������� � ���������� � ����� �������
		LP.v_point.erase(std::remove_if(LP.v_point.begin(), LP.v_point.end(),
										[this](light* L) {
											if (L->vis.visible)
											{
												accumulate_point_lights(L);
												return true;
											}
											return false;
										}),
						 LP.v_point.end());
	}

	// Spot lights
	if (!LP.v_spot.empty())
	{
		OPTICK_EVENT("CRender::render_lights - remaining spot");

		// ��������������� ���������� ������ ��� ������� ����������
		for (light* L : LP.v_spot)
		{
			L->vis_update();
			if (L->vis.visible)
			{
				LR.compute_xf_spot(L);
			}
		}

		// ����������
		LP.v_spot.erase(std::remove_if(LP.v_spot.begin(), LP.v_spot.end(),
									   [this](light* L) {
										   if (L->vis.visible)
										   {
											   accumulate_spot_lights(L);
											   return true;
										   }
										   return false;
									   }),
						LP.v_spot.end());
	}
}
