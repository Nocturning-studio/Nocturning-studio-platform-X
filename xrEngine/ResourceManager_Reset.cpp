#include "stdafx.h"
#pragma hdrstop

#include "ResourceManager.h"
#ifndef _EDITOR
#include "Render.h"
#endif

#include <ppl.h>

void CResourceManager::reset_begin()
{
	// destroy everything, renderer may use
	::Render->reset_begin();

	// destroy state-blocks
	for (u32 _it = 0; _it < v_states.size(); _it++)
		_RELEASE(v_states[_it]->state);

	// destroy RTs
	for (map_RTIt rt_it = m_rtargets.begin(); rt_it != m_rtargets.end(); rt_it++)
		rt_it->second->reset_begin();
	for (map_RTCIt rtc_it = m_rtargets_c.begin(); rtc_it != m_rtargets_c.end(); rtc_it++)
		rtc_it->second->reset_begin();

	// destroy DStreams
	RenderBackend.old_QuadIB = RenderBackend.QuadIB;
	_RELEASE(RenderBackend.QuadIB);
	RenderBackend.Index.reset_begin();
	RenderBackend.Vertex.reset_begin();

	DeferredUnload();
}

bool cmp_rt(const CRT* A, const CRT* B)
{
	return A->_order < B->_order;
}
bool cmp_rtc(const CRTC* A, const CRTC* B)
{
	return A->_order < B->_order;
}

void CResourceManager::reset_end()
{
	// create RDStreams
	RenderBackend.Vertex.reset_end();
	RenderBackend.Index.reset_end();
	Evict();
	RenderBackend.CreateQuadIB();

	// remark geom's which point to dynamic VB/IB
	{
		for (u32 _it = 0; _it < v_geoms.size(); _it++)
		{
			SGeometry* _G = v_geoms[_it];
			if (_G->vb == RenderBackend.Vertex.old_pVB)
			{
				_G->vb = RenderBackend.Vertex.Buffer();
			}

			// Here we may recover the buffer using one of
			// RenderBackend's index buffers.
			// Do not remove else.
			if (_G->ib == RenderBackend.Index.old_pIB)
			{
				_G->ib = RenderBackend.Index.Buffer();
			}
			else if (_G->ib == RenderBackend.old_QuadIB)
			{
				_G->ib = RenderBackend.QuadIB;
			}
		}
	}

	// create RTs in the same order as them was first created
	{
		// RT
#pragma todo("container is created in stack!")
		xr_vector<CRT*> rt;
		for (map_RTIt rt_it = m_rtargets.begin(); rt_it != m_rtargets.end(); rt_it++)
			rt.push_back(rt_it->second);
		concurrency::parallel_sort(rt.begin(), rt.end(), cmp_rt);
		for (u32 _it = 0; _it < rt.size(); _it++)
			rt[_it]->reset_end();
	}
	{
		// RTc
#pragma todo("container is created in stack!")
		xr_vector<CRTC*> rt;
		for (map_RTCIt rt_it = m_rtargets_c.begin(); rt_it != m_rtargets_c.end(); rt_it++)
			rt.push_back(rt_it->second);
		concurrency::parallel_sort(rt.begin(), rt.end(), cmp_rtc);
		for (u32 _it = 0; _it < rt.size(); _it++)
			rt[_it]->reset_end();
	}

	// create state-blocks
	{
		for (u32 _it = 0; _it < v_states.size(); _it++)
			v_states[_it]->state = v_states[_it]->state_code.record();
	}

	DeferredUpload();

	// create everything, renderer may use
	::Render->reset_end();
}

template <class C> void mdump(C c)
{
	if (0 == c.size())
		return;
	for (C::iterator I = c.begin(); I != c.end(); I++)
		Msg("*        : %3d: %s", I->second->dwReference, I->second->cName.c_str());
}

CResourceManager::~CResourceManager()
{
	DestroyNecessaryTextures();
	Dump(false);
}

void CResourceManager::Dump(bool bBrief)
{
	Msg("* RM_Dump: textures  : %d", m_textures.size());
	Msg("* RM_Dump: rtargets  : %d", m_rtargets.size());
	Msg("* RM_Dump: rtargetsc : %d", m_rtargets_c.size());
	Msg("* RM_Dump: vs        : %d", m_vs.size());
	Msg("* RM_Dump: ps        : %d", m_ps.size());
	Msg("* RM_Dump: dcl       : %d", v_declarations.size());
	Msg("* RM_Dump: states    : %d", v_states.size());
	Msg("* RM_Dump: tex_list  : %d", lst_textures.size());
	Msg("* RM_Dump: matrices  : %d", lst_matrices.size());
	Msg("* RM_Dump: lst_constants: %d", lst_constants.size());
	Msg("* RM_Dump: v_passes  : %d", v_passes.size());
	Msg("* RM_Dump: v_elements: %d", v_elements.size());
	Msg("* RM_Dump: v_shaders : %d", v_shaders.size());
}
