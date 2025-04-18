#include "stdafx.h"
#include "flod.h"

#ifdef _EDITOR
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif

extern float r_ssaLOD_A;
extern float r_ssaLOD_B;

ICF bool pred_dot(const std::pair<float, u32>& _1, const std::pair<float, u32>& _2)
{
	OPTICK_EVENT("pred_dot");

	return _1.first < _2.first;
}

void R_dsgraph_structure::r_dsgraph_render_lods(bool _setup_zb, bool _clear)
{
	OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_render_lods");

	if (_setup_zb)
		mapLOD.getLR(lstLODs); // front-to-back
	else
		mapLOD.getRL(lstLODs); // back-to-front
	if (lstLODs.empty())
		return;

	// *** Fill VB and generate groups
	u32 shid = _setup_zb ? SE_R1_LMODELS : SE_R1_NORMAL_LQ;
	FLOD* firstV = (FLOD*)lstLODs[0].pVisual;
	// Msg						("dbg_lods: shid[%d],firstV[%X]",shid,u32((void*)firstV));
	// Msg						("dbg_lods: shader[%X]",u32((void*)firstV->shader._get()));
	ref_selement cur_S = firstV->shader->E[shid];
	// Msg						("dbg_lods: shader_E[%X]",u32((void*)cur_S._get()));
	int cur_count = 0;
	u32 vOffset;
	FLOD::_hw* V = (FLOD::_hw*)RenderBackend.Vertex.Lock(lstLODs.size() * 4, firstV->geom->vb_stride, vOffset);
	float ssaRange = r_ssaLOD_A - r_ssaLOD_B;
	if (ssaRange < EPS_S)
		ssaRange = EPS_S;
	for (u32 i = 0; i < lstLODs.size(); i++)
	{
		OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_render_lods - sort");

		// sort out redundancy
		R_dsgraph::_LodItem& P = lstLODs[i];
		if (P.pVisual->shader->E[shid] == cur_S)
			cur_count++;
		else
		{
			lstLODgroups.push_back(cur_count);
			cur_S = P.pVisual->shader->E[shid];
			cur_count = 1;
		}

		// calculate alpha
		float ssaDiff = P.ssa - r_ssaLOD_B;
		float scale = ssaDiff / ssaRange;
		int iA = iFloor((1 - scale) * 255.f);
		u32 uA = u32(clampr(iA, 0, 255));

		// calculate direction and shift
		FLOD* lodV = (FLOD*)P.pVisual;
		Fvector Ldir, shift;
		Ldir.sub(lodV->vis.sphere.P, Device.vCameraPosition).normalize();
		shift.mul(Ldir, -.5f * lodV->vis.sphere.R);

		// gen geometry
		FLOD::_face* facets = lodV->facets;
		svector<std::pair<float, u32>, 8> selector;
		for (u32 s = 0; s < 8; s++)
			selector.push_back(mk_pair(Ldir.dotproduct(facets[s].N), s));
		concurrency::parallel_sort(selector.begin(), selector.end(), pred_dot);

		float dot_best = selector[selector.size() - 1].first;
		float dot_next = selector[selector.size() - 2].first;
		float dot_next_2 = selector[selector.size() - 3].first;
		u32 id_best = selector[selector.size() - 1].second;
		u32 id_next = selector[selector.size() - 2].second;

		// Now we have two "best" planes, calculate factor, and approx normal
		float fA = dot_best, fB = dot_next, fC = dot_next_2;
		float alpha = 0.5f + 0.5f * (1 - (fB - fC) / (fA - fC));
		int iF = iFloor(alpha * 255.5f);
		u32 uF = u32(clampr(iF, 0, 255));

		// Fill VB
		FLOD::_face& FA = facets[id_best];
		FLOD::_face& FB = facets[id_next];
		static int vid[4] = {3, 0, 2, 1};
		for (u32 vit = 0; vit < 4; vit++)
		{
			int id = vid[vit];
			V->p0.add(FB.v[id].v, shift);
			V->p1.add(FA.v[id].v, shift);
			V->n0 = FB.N;
			V->n1 = FA.N;
			V->sun_af = color_rgba(FB.v[id].c_sun, FA.v[id].c_sun, uA, uF);
			V->t0 = FB.v[id].t;
			V->t1 = FA.v[id].t;
			V->rgbh0 = FB.v[id].c_rgb_hemi;
			V->rgbh1 = FA.v[id].c_rgb_hemi;
			V++;
		}
	}
	lstLODgroups.push_back(cur_count);
	RenderBackend.Vertex.Unlock(lstLODs.size() * 4, firstV->geom->vb_stride);

	// *** Render
	OPTICK_EVENT("R_dsgraph_structure::r_dsgraph_render_lods - render");

	int current = 0;
	RenderBackend.set_xform_world(Fidentity);
	for (u32 g = 0; g < lstLODgroups.size(); g++)
	{
		int p_count = lstLODgroups[g];
		RenderBackend.set_Element(lstLODs[current].pVisual->shader->E[shid]);
		RenderBackend.set_Geometry(firstV->geom);
		RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4 * p_count, 0, 2 * p_count);
		RenderBackend.stat.r.s_flora_lods.add(4 * p_count);
		current += p_count;
		vOffset += 4 * p_count;
	}

	lstLODs.clear();
	lstLODgroups.clear();

	if (_clear)
		mapLOD.clear();
}
