#include "stdafx.h"
#pragma hdrstop

#pragma warning(push)
#pragma warning(disable : 4995)
#include <DXSDK/d3dx9.h>
#pragma warning(pop)

#include "frustum.h"

#include "dx11/StateManager.h"
#include "dx11/ShaderResourceStateCache.h"

void CBackend::OnFrameEnd()
{
	HW.pContext11->ClearState();
	Invalidate();
}

void CBackend::OnFrameBegin()
{
	PGO(Msg("PGO:*****frame[%d]*****", RDEVICE.dwFrame));
	Memory.mem_fill(&stat, 0, sizeof(stat));
	Vertex.Flush();
	Index.Flush();
	set_Stencil(FALSE);
}

void CBackend::Invalidate()
{
	pRT[0] = NULL;
	pRT[1] = NULL;
	pRT[2] = NULL;
	pRT[3] = NULL;
	pZB = NULL;

	decl = NULL;
	vb = NULL;
	ib = NULL;
	vb_stride = 0;

	state = NULL;
	ps = NULL;
	vs = NULL;

	ctable = NULL;

	T = NULL;
	M = NULL;
	C = NULL;

	stencil_enable = u32(-1);
	stencil_func = u32(-1);
	stencil_ref = u32(-1);
	stencil_mask = u32(-1);
	stencil_writemask = u32(-1);
	stencil_fail = u32(-1);
	stencil_pass = u32(-1);
	stencil_zfail = u32(-1);
	cull_mode = u32(-1);
	z_enable = u32(-1);
	z_func = u32(-1);
	alpha_ref = u32(-1);
	colorwrite_mask = u32(-1);

	//	Since constant buffers are unmapped (for DirecX 10)
	//	transform setting handlers should be unmapped too.
	xforms.unmap();

	m_pInputLayout = NULL;
	m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	m_bChangedRTorZB = false;
	m_pInputSignature = NULL;
	for (int i = 0; i < MaxCBuffers; ++i)
	{
		m_aPixelConstants[i] = 0;
		m_aVertexConstants[i] = 0;
	}
	StateManager.Reset();
	//	Redundant call. Just no note that we need to unmap const
	//	if we create dedicated class.
	StateManager.UnmapConstants();
	SSManager.ResetDeviceState();
	SRVSManager.ResetDeviceState();

	for (u32 ps_it = 0; ps_it < mtMaxPixelShaderTextures;)
		textures_ps[ps_it++] = 0;
	for (u32 vs_it = 0; vs_it < mtMaxVertexShaderTextures;)
		textures_vs[vs_it++] = 0;

#ifdef _EDITOR
	for (u32 m_it = 0; m_it < 8;)
		matrices[m_it++] = 0;
#endif
}

void CBackend::set_ClipPlanes(u32 _enable, Fplane* _planes /*=NULL */, u32 count /* =0*/)
{
	//	TODO: DX10: Implement in the corresponding vertex shaders
	//	Use this to set up location, were shader setup code will get data
	// VERIFY(!"CBackend::set_ClipPlanes not implemented!");
	return;
}

#ifndef DEDICATED_SREVER
void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */)
{
	if (0 == HW.Caps.geometry.dwClipPlanes)
		return;
	if (!_enable)
	{
		//	TODO: DX10: Implement in the corresponding vertex shaders
		//	Use this to set up location, were shader setup code will get data
		// VERIFY(!"CBackend::set_ClipPlanes not implemented!");
		return;
	}
	VERIFY(_xform && fmask);
	CFrustum F;
	F.CreateFromMatrix(*_xform, fmask);
	set_ClipPlanes(_enable, F.planes, F.p_count);
}

void CBackend::set_Textures(STextureList* _T)
{
	if (T == _T)
		return;
	T = _T;
	//	If resources weren't set at all we should clear from resource #0.
	int _last_ps = -1;
	int _last_vs = -1;
	STextureList::iterator _it = _T->begin();
	STextureList::iterator _end = _T->end();

	for (; _it != _end; _it++)
	{
		std::pair<u32, ref_texture>& loader = *_it;
		u32 load_id = loader.first;
		CTexture* load_surf = &*loader.second;
		//		if (load_id < 256)		{
		if (load_id < CTexture::rstVertex)
		{
			//	Set up pixel shader resources
			VERIFY(load_id < mtMaxPixelShaderTextures);
			// ordinary pixel surface
			if ((int)load_id > _last_ps)
				_last_ps = load_id;
			if (textures_ps[load_id] != load_surf)
			{
				textures_ps[load_id] = load_surf;
#ifdef DEBUG
				stat.textures++;
#endif
				if (load_surf)
				{
					PGO(Msg("PGO:tex%d:%s", load_id, load_surf->cName.c_str()));
					load_surf->bind(load_id);
					//					load_surf->Apply	(load_id);
				}
			}
		}
		else
		//if (load_id < CTexture::rstGeometry)
		if (load_id < CTexture::rstInvalid)
		{
			//	Set up pixel shader resources
			VERIFY(load_id < CTexture::rstVertex + mtMaxVertexShaderTextures);

			// vertex only //d-map or vertex
			u32 load_id_remapped = load_id - CTexture::rstVertex;
			if ((int)load_id_remapped > _last_vs)
				_last_vs = load_id_remapped;
			if (textures_vs[load_id_remapped] != load_surf)
			{
				textures_vs[load_id_remapped] = load_surf;
#ifdef DEBUG
				stat.textures++;
#endif
				if (load_surf)
				{
					PGO(Msg("PGO:tex%d:%s", load_id, load_surf->cName.c_str()));
					load_surf->bind(load_id);
					//					load_surf->Apply	(load_id);
				}
			}
		}
		else
			VERIFY("Invalid enum");
	}

	// clear remaining stages (PS)
	for (++_last_ps; _last_ps < mtMaxPixelShaderTextures; _last_ps++)
	{
		if (!textures_ps[_last_ps])
			continue;

		textures_ps[_last_ps] = 0;
		//	TODO: DX10: Optimise: set all resources at once
		ID3D11ShaderResourceView* pRes = 0;
		// HW.pDevice->PSSetShaderResources(_last_ps, 1, &pRes);
		SRVSManager.SetPSResource(_last_ps, pRes);
	}
	// clear remaining stages (VS)
	for (++_last_vs; _last_vs < mtMaxVertexShaderTextures; _last_vs++)
	{
		if (!textures_vs[_last_vs])
			continue;

		textures_vs[_last_vs] = 0;
		//	TODO: DX10: Optimise: set all resources at once
		ID3D11ShaderResourceView* pRes = 0;
		// HW.pDevice->VSSetShaderResources(_last_vs, 1, &pRes);
		SRVSManager.SetVSResource(_last_vs, pRes);
	}
}
#else

void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */)
{
}
void CBackend::set_Textures(STextureList* _T)
{
}

#endif