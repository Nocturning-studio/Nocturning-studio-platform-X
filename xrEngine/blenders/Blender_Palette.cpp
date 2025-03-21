#include "stdafx.h"
#pragma hdrstop

#include "Blender.h"

#include <ppl.h>

//////////////////////////////////////////////////////////////////////
#include "blender_clsid.h"
IC bool p_sort(IBlender* A, IBlender* B)
{
	return stricmp(A->getComment(), B->getComment()) < 0;
}

#ifdef __BORLANDC__
#define TYPES_EQUAL(A, B) (typeid(A) == typeid(B))
#else
#define TYPES_EQUAL(A, B) (typeid(A).raw_name() == typeid(B).raw_name())
#endif

void IBlender::CreatePalette(xr_vector<IBlender*>& palette)
{
	// Create palette itself
	R_ASSERT(palette.empty());
	palette.push_back(Create(B_DEFAULT));
	palette.push_back(Create(B_DEFAULT_AREF));
	palette.push_back(Create(B_VERT));
	palette.push_back(Create(B_VERT_AREF));
	palette.push_back(Create(B_SCREEN_SET));
	palette.push_back(Create(B_SCREEN_GRAY));
	palette.push_back(Create(B_EDITOR_WIRE));
	palette.push_back(Create(B_EDITOR_SEL));
	palette.push_back(Create(B_LIGHT));
	palette.push_back(Create(B_LaEmB));
	palette.push_back(Create(B_LmEbB));
	palette.push_back(Create(B_BmmD));
	palette.push_back(Create(B_B));
	palette.push_back(Create(B_SHADOW_WORLD));
	palette.push_back(Create(B_BLUR));
	palette.push_back(Create(B_MODEL));
	palette.push_back(Create(B_MODEL_EbB));
	palette.push_back(Create(B_DETAIL));
	palette.push_back(Create(B_TREE));
	palette.push_back(Create(B_PARTICLE));

	// Remove duplicated classes (some of them are really the same in different renderers)
	for (u32 i = 0; i < palette.size(); i++)
	{
		IBlender* A = palette[i];
		for (u32 j = i + 1; j < palette.size(); j++)
		{
			IBlender* B = palette[j];
			if (TYPES_EQUAL(*A, *B))
			{
				xr_delete(palette[j]);
				j--;
			}
		}
	}

	// Sort by desc and return
	concurrency::parallel_sort(palette.begin(), palette.end(), p_sort);
}

#ifndef _EDITOR
// Engine
#include "..\render.h"
IBlender* IBlender::Create(CLASS_ID cls)
{
	return ::Render->blender_create(cls);
}
void IBlender::Destroy(IBlender*& B)
{
	::Render->blender_destroy(B);
}
#else

// Editor
#include "../xrRender_R1/blenderdefault.h"
#include "../xrRender_R1/blender_default_aref.h"
#include "../xrRender_R1/blender_vertex.h"
#include "../xrRender_R1/blender_vertex_aref.h"
#include "blender_screen_set.h"
#include "../xrRender_R1/blender_screen_gray.h"
#include "blender_editor_wire.h"
#include "blender_editor_selection.h"
#include "blender_light.h"
#include "../xrRender_R1/blender_LaEmB.h"
#include "blender_Lm(EbB).h"
#include "blender_BmmD.h"
#include "blender_B.h"
#include "blender_shadow_texture.h"
#include "../xrRender_R1/blender_shadow_world.h"
#include "../xrRender_R1/blender_blur.h"
#include "../xrRender_R1/blender_model.h"
#include "blender_model_ebb.h"
#include "blender_detail_still.h"
#include "blender_tree.h"
#include "blender_particle.h"

IBlender* IBlender::Create(CLASS_ID cls)
{
	switch (cls)
	{
	case B_DEFAULT:
		return xr_new<CBlender_default>();
	case B_DEFAULT_AREF:
		return xr_new<CBlender_default_aref>();
	case B_VERT:
		return xr_new<CBlender_Vertex>();
	case B_VERT_AREF:
		return xr_new<CBlender_Vertex_aref>();
	case B_SCREEN_SET:
		return xr_new<CBlender_Screen_SET>();
	case B_SCREEN_GRAY:
		return xr_new<CBlender_Screen_GRAY>();
	case B_EDITOR_WIRE:
		return xr_new<CBlender_Editor_Wire>();
	case B_EDITOR_SEL:
		return xr_new<CBlender_Editor_Selection>();
	case B_LIGHT:
		return xr_new<CBlender_LIGHT>();
	case B_LaEmB:
		return xr_new<CBlender_LaEmB>();
	case B_LmEbB:
		return xr_new<CBlender_LmEbB>();
	case B_B:
		return xr_new<CBlender_B>();
	case B_BmmD:
		return xr_new<CBlender_BmmD>();
	case B_SHADOW_WORLD:
		return xr_new<CBlender_ShWorld>();
	case B_BLUR:
		return xr_new<CBlender_Blur>();
	case B_MODEL:
		return xr_new<CBlender_Model>();
	case B_MODEL_EbB:
		return xr_new<CBlender_Model_EbB>();
	case B_DETAIL:
		return xr_new<CBlender_Detail_Still>();
	case B_TREE:
		return xr_new<CBlender_Tree>();
	case B_PARTICLE:
		return xr_new<CBlender_Particle>();
	}
	return 0;
}

void IBlender::Destroy(IBlender*& B)
{
	xr_delete(B);
}
#endif
