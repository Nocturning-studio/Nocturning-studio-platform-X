// IRender_Visual.cpp: implementation of the IRender_Visual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR
#include "render.h"
#endif
#include "fbasicvisual.h"
#include "fmesh.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IRender_Mesh::~IRender_Mesh()
{
	OPTICK_EVENT("IRender_Mesh::~IRender_Mesh");

	_RELEASE(p_rm_Vertices);
	_RELEASE(p_rm_Indices);
}

IRender_Visual::IRender_Visual()
{
	OPTICK_EVENT("IRender_Visual::IRender_Visual");

	Type = 0;
	shader = 0;
	vis.clear();
}

IRender_Visual::~IRender_Visual()
{
	OPTICK_EVENT("IRender_Visual::~IRender_Visual");

}

void IRender_Visual::Release()
{
	OPTICK_EVENT("IRender_Visual::Release");

}

CStatTimer tscreate;

void IRender_Visual::Load(const char* N, IReader* data, u32)
{
	OPTICK_EVENT("IRender_Visual::Load");

#ifdef DEBUG
	dbg_name = N;
#endif

	// header
	VERIFY(data);
	ogf_header hdr;
	if (data->r_chunk_safe(OGF_HEADER, &hdr, sizeof(hdr)))
	{
		R_ASSERT3(hdr.format_version == xrOGF_FormatVersion, "Invalid visual version for ogf visual with name %s", N);
		Type = hdr.type;
		if (hdr.shader_id)
			shader = ::Render->getShader(hdr.shader_id);
		vis.box.set(hdr.bb.min, hdr.bb.max);
		vis.sphere.set(hdr.bs.c, hdr.bs.r);
	}
	else
	{
		Msg("Please repair ogf visual with name %s", N);
		FATAL("Invalid visual");
	}

	// Shader
	if (data->find_chunk(OGF_TEXTURE))
	{
		string256 fnT, fnS;
		data->r_stringZ(fnT, sizeof(fnT));
		data->r_stringZ(fnS, sizeof(fnS));
		shader.create(fnS, fnT);
	}

	// desc
#ifdef _EDITOR
	if (data->find_chunk(OGF_S_DESC))
		desc.Load(*data);
#endif
}

#define PCOPY(a) a = pFrom->a
void IRender_Visual::Copy(IRender_Visual* pFrom)
{
	OPTICK_EVENT("IRender_Visual::Copy");

	PCOPY(Type);
	PCOPY(shader);
	PCOPY(vis);
#ifdef _EDITOR
	PCOPY(desc);
#endif
#ifdef DEBUG
	PCOPY(dbg_name);
#endif
}
