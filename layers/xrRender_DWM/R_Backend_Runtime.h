#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include "sh_texture.h"
#include "sh_matrix.h"
#include "sh_constant.h"
#include "sh_rt.h"

#include "render_types.h"

#include "State.h"
#include "StateManager.h"
#include "ShaderResourceStateCache.h"

IC void		R_xforms::set_c_w			(R_constant* C)		
{	
	c_w		= C;	
	RCache.set_c(C,m_w);	
};

IC void		R_xforms::set_c_invw		(R_constant* C)		
{	
	c_invw	= C;	
	apply_invw();			
};

IC void		R_xforms::set_c_v			(R_constant* C)		
{	
	c_v		= C;	
	RCache.set_c(C,m_v);	
};

IC void		R_xforms::set_c_p			(R_constant* C)		
{	
	c_p		= C;	
	RCache.set_c(C,m_p);	
};

IC void		R_xforms::set_c_wv			(R_constant* C)		
{	
	c_wv	= C;	
	RCache.set_c(C,m_wv);	
};

IC void		R_xforms::set_c_vp			(R_constant* C)		
{	
	c_vp	= C;	
	RCache.set_c(C,m_vp);	
};

IC void		R_xforms::set_c_wvp			(R_constant* C)		
{	
	c_wvp	= C;	
	RCache.set_c(C,m_wvp);	
};

IC	void	CBackend::set_xform_world	(const Fmatrix& M)
{ 
	xforms.set_W(M);	
}

IC	void	CBackend::set_xform_view	(const Fmatrix& M)					
{ 
	xforms.set_V(M);	
}

IC	void	CBackend::set_xform_project	(const Fmatrix& M)
{ 
	xforms.set_P(M);	
}

IC	const Fmatrix&	CBackend::get_xform_world	()	
{
	return xforms.get_W();	
}

IC	const Fmatrix&	CBackend::get_xform_view	()	
{ 
	return xforms.get_V();	
}

IC	const Fmatrix&	CBackend::get_xform_project	()	
{ 
	return xforms.get_P();	
}

IC	const Fmatrix&	CBackend::get_xform_wvp		()
{ 
	return xforms.get_WVP();	
}

IC	const Fmatrix&	CBackend::get_xform_vp		()
{ 
	return xforms.get_VP();	
}

IC	ID3DRenderTargetView* CBackend::get_RT(u32 ID)
{
	VERIFY((ID>=0)&&(ID<4));

	return pRT[ID];
}

IC	ID3DDepthStencilView* CBackend::get_ZB				()
{
	return pZB;
}

ICF void	CBackend::set_States		(ID3DState* _state)
{
//	DX10 Manages states using it's own algorithm. Don't mess with it.

	{
		PGO				(Msg("PGO:state_block"));
#ifdef DEBUG
		stat.states		++;
#endif
		state			= _state;
		state->Apply	();
	}
}

#ifdef _EDITOR
IC void CBackend::set_Matrices			(SMatrixList*	_M)
{
	if (M != _M)
	{
		M = _M;
		if (M)	{
			for (u32 it=0; it<M->size(); it++)
			{
				CMatrix*	mat = &*((*M)[it]);
				if (mat && matrices[it]!=mat)
				{
					matrices	[it]	= mat;
					mat->Calculate		();
					set_xform			(D3DTS_TEXTURE0+it,mat->xform);
	//				stat.matrices		++;
				}
			}
		}
	}
}
#endif

IC void CBackend::set_Element			(ShaderElement* S, u32	pass)
{
	SPass&	P		= *(S->passes[pass]);

	set_States		(P.state);
	set_PS			(P.ps);
	set_VS			(P.vs);


	set_GS			(P.gs);
	set_HS			(P.hs);
	set_DS			(P.ds);
	set_CS			(P.cs);
	

	set_Constants	(P.constants);
	set_Textures	(P.T);

#ifdef _EDITOR
	set_Matrices	(P.M);
#endif
}

ICF void CBackend::set_Shader			(Shader* S, u32 pass)
{
	set_Element			(S->E[0],pass);
}


IC void CBackend::set_ComputeElement(ShaderElement* S, u32 pass)
{
	SPass& P = *(S->passes[pass]);

	set_States(P.state);
	set_CS(P.cs);

	set_Constants(P.constants);
	set_Textures(P.T);

#ifdef _EDITOR
	set_Matrices(P.M);
#endif
}

ICF void CBackend::set_ComputeShader(Shader* S, u32 pass)
{
	set_ComputeElement(S->E[0],pass);
}

#pragma todo(Implement set_xform())
IC void CBackend::set_xform(u32 ID, const Fmatrix& M)
{
	stat.xforms_count++;
	//	TODO: DX10: Implement CBackend::set_xform
	//VERIFY(!"Implement CBackend::set_xform");
}

IC void CBackend::set_RT(ID3DRenderTargetView* RT, u32 ID)
{
	if (RT != pRT[ID])
	{
		PGO(Msg("PGO:setRT"));
		stat.target_rt++;
		pRT[ID] = RT;
		//	Mark RT array dirty
		//HW.pDevice->OMSetRenderTargets(sizeof(pRT)/sizeof(pRT[0]), pRT, 0);
		//HW.pDevice->OMSetRenderTargets(sizeof(pRT)/sizeof(pRT[0]), pRT, pZB);
		//	Reset all RT's here to allow RT to be bounded as input
		if (!m_bChangedRTorZB)
			HW.pContext->OMSetRenderTargets(0, 0, 0);

		m_bChangedRTorZB = true;
	}
}

IC void	CBackend::set_ZB(ID3DDepthStencilView* ZB)
{
	if (ZB != pZB)
	{
		PGO(Msg("PGO:setZB"));
		stat.target_zb++;
		pZB = ZB;
		//HW.pDevice->OMSetRenderTargets(0, 0, pZB);
		//HW.pDevice->OMSetRenderTargets(sizeof(pRT)/sizeof(pRT[0]), pRT, pZB);
		//	Reset all RT's here to allow RT to be bounded as input
		if (!m_bChangedRTorZB)
			HW.pContext->OMSetRenderTargets(0, 0, 0);
		m_bChangedRTorZB = true;
	}
}

ICF void CBackend::set_Format(SDeclaration* _decl)
{
	if (decl != _decl)
	{
		PGO(Msg("PGO:v_format:%x", _decl));
#ifdef DEBUG
		stat.decl++;
#endif
		decl = _decl;
	}
}

ICF void CBackend::set_PS(ID3DPixelShader* _ps, LPCSTR _n)
{
	if (ps != _ps)
	{
		PGO(Msg("PGO:Pshader:%x", _ps));
		stat.ps++;
		ps = _ps;

		HW.pContext->PSSetShader(ps, 0, 0);


#ifdef DEBUG
		ps_name = _n;
#endif
	}
}

ICF void CBackend::set_GS(ID3DGeometryShader* _gs, LPCSTR _n)
{
	if (gs != _gs)
	{
		PGO(Msg("PGO:Gshader:%x", _ps));
		//	TODO: DX10: Get statistics for G Shader change
		//stat.gs			++;
		gs = _gs;

		HW.pContext->GSSetShader(gs, 0, 0);


#ifdef DEBUG
		gs_name = _n;
#endif
	}
}

ICF void CBackend::set_HS(ID3DHullShader* _hs, LPCSTR _n)
{
	if (hs != _hs)
	{
		PGO(Msg("PGO:Hshader:%x", _ps));
		//	TODO: DX10: Get statistics for H Shader change
		//stat.hs			++;
		hs = _hs;
		HW.pContext->HSSetShader(hs, 0, 0);

#ifdef DEBUG
		hs_name = _n;
#endif
	}
}

ICF void CBackend::set_DS(ID3DDomainShader* _ds, LPCSTR _n)
{
	if (ds != _ds)
	{
		PGO(Msg("PGO:Dshader:%x", _ps));
		//	TODO: DX10: Get statistics for D Shader change
		//stat.ds			++;
		ds = _ds;
		HW.pContext->DSSetShader(ds, 0, 0);

#ifdef DEBUG
		ds_name = _n;
#endif
	}
}

ICF void CBackend::set_CS(ID3DComputeShader* _cs, LPCSTR _n)
{
	if (cs != _cs)
	{
		PGO(Msg("PGO:Cshader:%x", _ps));
		//	TODO: DX10: Get statistics for D Shader change
		//stat.cs			++;
		cs = _cs;
		HW.pContext->CSSetShader(cs, 0, 0);

#ifdef DEBUG
		cs_name = _n;
#endif
	}
}

ICF	bool CBackend::is_TessEnabled()
{
	return HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0 && (ds != 0 || hs != 0);
}



ICF void CBackend::set_VS(ID3DVertexShader* _vs, LPCSTR _n)
{
	if (vs != _vs)
	{
		PGO(Msg("PGO:Vshader:%x", _vs));
		stat.vs++;
		vs = _vs;

		HW.pContext->VSSetShader(vs, 0, 0);


#ifdef DEBUG
		vs_name = _n;
#endif
	}
}

ICF void CBackend::set_Vertices(ID3DBuffer* _vb, u32 _vb_stride)
{
	if ((vb != _vb) || (vb_stride != _vb_stride))
	{
		PGO(Msg("PGO:VB:%x,%d", _vb, _vb_stride));
#ifdef DEBUG
		stat.vb++;
#endif
		vb = _vb;
		vb_stride = _vb_stride;
		//CHK_DX			(HW.pDevice->SetStreamSource(0,vb,0,vb_stride));
		//UINT StreamNumber,
		//IDirect3DVertexBuffer9 * pStreamData,
		//UINT OffsetInBytes,
		//UINT Stride

		//UINT StartSlot,
		//UINT NumBuffers,
		//ID3DxxBuffer *const *ppVertexBuffers,
		//const UINT *pStrides,
		//const UINT *pOffsets
		u32	iOffset = 0;
		HW.pContext->IASetVertexBuffers(0, 1, &vb, &_vb_stride, &iOffset);
	}
}

ICF void CBackend::set_Indices(ID3DBuffer* _ib)
{
	if (ib != _ib)
	{
		PGO(Msg("PGO:IB:%x", _ib));
#ifdef DEBUG
		stat.ib++;
#endif
		ib = _ib;
		HW.pContext->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
	}
}

IC D3D_PRIMITIVE_TOPOLOGY TranslateTopology(D3DPRIMITIVETYPE T)
{
	static	D3D_PRIMITIVE_TOPOLOGY translateTable[] =
	{
		D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,		//	None
		D3D_PRIMITIVE_TOPOLOGY_POINTLIST,		//	D3DPT_POINTLIST = 1,
		D3D_PRIMITIVE_TOPOLOGY_LINELIST,		//	D3DPT_LINELIST = 2,
		D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,		//	D3DPT_LINESTRIP = 3,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,	//	D3DPT_TRIANGLELIST = 4,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,	//	D3DPT_TRIANGLESTRIP = 5,
		D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,		//	D3DPT_TRIANGLEFAN = 6,
	};

	VERIFY(T < sizeof(translateTable) / sizeof(translateTable[0]));
	VERIFY(T >= 0);

	D3D_PRIMITIVE_TOPOLOGY	result = translateTable[T];

	VERIFY(result != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

	return result;
}

IC u32 GetIndexCount(D3DPRIMITIVETYPE T, u32 iPrimitiveCount)
{
	switch (T)
	{
	case D3DPT_POINTLIST:
		return iPrimitiveCount;
	case D3DPT_LINELIST:
		return iPrimitiveCount * 2;
	case D3DPT_LINESTRIP:
		return iPrimitiveCount + 1;
	case D3DPT_TRIANGLELIST:
		return iPrimitiveCount * 3;
	case D3DPT_TRIANGLESTRIP:
		return iPrimitiveCount + 2;
	default: NODEFAULT;
#ifdef DEBUG
		return 0;
#endif // #ifdef DEBUG
	}
}

IC void CBackend::ApplyPrimitieTopology(D3D_PRIMITIVE_TOPOLOGY Topology)
{
	if (m_PrimitiveTopology != Topology)
	{
		m_PrimitiveTopology = Topology;
		HW.pContext->IASetPrimitiveTopology(m_PrimitiveTopology);
	}
}


IC void CBackend::Compute(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	stat.calls++;

	SRVSManager.Apply();
	StateManager.Apply();
	//	State manager may alter constants
	constants.flush();
	HW.pContext->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

// Perform 1 uav compute with save old uav rt params 
void CBackend::ComputeUAVWithRestore(ID3DUnorderedAccessView* pUAV, int ThreadGroupCountX, int ThreadGroupCountY, int ThreadGroupCountZ)
{
	UINT UAVInitialCounts = 1;

	ID3DUnorderedAccessView* olduav[1] = { 0 };
	ID3DRenderTargetView* oldrtv[8] = { 0,0,0,0,0,0,0,0 };
	ID3DDepthStencilView* olddsv;

	HW.pContext->OMGetRenderTargets(8, oldrtv, &olddsv);

	ID3DRenderTargetView* rtv[8] = { 0 };
	ID3DShaderResourceView* srv[16] = { 0 };

	HW.pContext->OMSetRenderTargets(8, rtv, NULL);
	HW.pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, &UAVInitialCounts);

	Compute(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	HW.pContext->CSSetUnorderedAccessViews(0, 1, olduav, &UAVInitialCounts);
	HW.pContext->CSSetShaderResources(0, 16, srv);
	HW.pContext->OMSetRenderTargets(8, oldrtv, olddsv);
}

void CBackend::ComputeWithRestore(ref_rt& _1, int ThreadGroupCountX, int ThreadGroupCountY, int ThreadGroupCountZ)
{
	UINT UAVInitialCounts = 1;

	ID3DUnorderedAccessView* olduav[1] = { 0 };
	ID3DRenderTargetView* oldrtv[8] = { 0,0,0,0,0,0,0,0 };
	ID3DDepthStencilView* olddsv;

	HW.pContext->OMGetRenderTargets(8, oldrtv, &olddsv);

	ID3DRenderTargetView* rtv[8] = { 0 };
	ID3DShaderResourceView* srv[16] = { 0 };

	HW.pContext->OMSetRenderTargets(8, rtv, NULL);
	HW.pContext->CSSetUnorderedAccessViews(0, 1, &_1->pUAView, &UAVInitialCounts);

	Compute(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	HW.pContext->CSSetUnorderedAccessViews(0, 1, olduav, &UAVInitialCounts);
	HW.pContext->CSSetShaderResources(0, 16, srv);
	HW.pContext->OMSetRenderTargets(8, oldrtv, olddsv);
}

IC void CBackend::Render(D3DPRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
	//VERIFY(vs);
	//HW.pDevice->VSSetShader(vs);
	//HW.pDevice->GSSetShader(0);

	D3D_PRIMITIVE_TOPOLOGY Topology = TranslateTopology(T);
	u32	iIndexCount = GetIndexCount(T, PC);

	//!!! HACK !!!
	if (hs != 0 || ds != 0)
	{
		R_ASSERT(Topology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Topology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}

	stat.calls++;
	stat.verts += countV;
	stat.polys += PC;

	ApplyPrimitieTopology(Topology);

	//CHK_DX(HW.pDevice->DrawIndexedPrimitive(T,baseV, startV, countV,startI,PC));
	//D3DPRIMITIVETYPE Type,
	//INT BaseVertexIndex,
	//UINT MinIndex,
	//UINT NumVertices,
	//UINT StartIndex,
	//UINT PriResmitiveCount

	//UINT IndexCount,
	//UINT StartIndexLocation,
	//INT BaseVertexLocation
	SRVSManager.Apply();
	ApplyRTandZB();
	ApplyVertexLayout();
	StateManager.Apply();
	//	State manager may alter constants
	constants.flush();
	//	Msg("DrawIndexed: Start");
	//	Msg("iIndexCount=%d, startI=%d, baseV=%d", iIndexCount, startI, baseV);
	HW.pContext->DrawIndexed(iIndexCount, startI, baseV);
	//	Msg("DrawIndexed: End\n");

	PGO(Msg("PGO:DIP:%dv/%df", countV, PC));
}

IC void CBackend::Render(D3DPRIMITIVETYPE T, u32 startV, u32 PC)
{
	//	TODO: DX10: Remove triangle fan usage from the engine
	if (T == D3DPT_TRIANGLEFAN)
		return;

	//VERIFY(vs);
	//HW.pDevice->VSSetShader(vs);

	D3D_PRIMITIVE_TOPOLOGY Topology = TranslateTopology(T);
	u32	iVertexCount = GetIndexCount(T, PC);

	stat.calls++;
	stat.verts += 3 * PC;
	stat.polys += PC;

	ApplyPrimitieTopology(Topology);
	SRVSManager.Apply();
	ApplyRTandZB();
	ApplyVertexLayout();
	StateManager.Apply();
	//	State manager may alter constants
	constants.flush();
	//	Msg("Draw: Start");
	//	Msg("iVertexCount=%d, startV=%d", iVertexCount, startV);
		//CHK_DX				(HW.pDevice->DrawPrimitive(T, startV, PC));
	HW.pContext->Draw(iVertexCount, startV);
	//	Msg("Draw: End\n");
	PGO(Msg("PGO:DIP:%dv/%df", 3 * PC, PC));
}

IC void CBackend::set_Geometry(SGeometry* _geom)
{
	set_Format(&*_geom->dcl);

	set_Vertices(_geom->vb, _geom->vb_stride);
	set_Indices(_geom->ib);
}

IC void	CBackend::set_Scissor(Irect* R)
{
	if (R)
	{
		StateManager.EnableScissoring();
		RECT* clip = (RECT*)R;
		HW.pContext->RSSetScissorRects(1, clip);
	}
	else
	{
		StateManager.EnableScissoring(FALSE);
		HW.pContext->RSSetScissorRects(0, 0);
	}
}

IC void CBackend::set_Stencil(u32 _enable, u32 _func, u32 _ref, u32 _mask, u32 _writemask, u32 _fail, u32 _pass, u32 _zfail)
{
	StateManager.SetStencil(_enable, _func, _ref, _mask, _writemask, _fail, _pass, _zfail);
}

IC  void CBackend::set_Z(u32 _enable)
{
	StateManager.SetDepthEnable(_enable);
}

IC  void CBackend::set_ZFunc(u32 _func)
{
	StateManager.SetDepthFunc(_func);
}

IC  void CBackend::set_AlphaRef(u32 _value)
{
	VERIFY(!"Not implemented.");
}

IC void	CBackend::set_ColorWriteEnable(u32 _mask)
{
	StateManager.SetColorWriteEnable(_mask);
}
ICF void CBackend::set_CullMode(u32 _mode)
{
	StateManager.SetCullMode(_mode);
}

IC void CBackend::ApplyVertexLayout()
{
	VERIFY(vs);
	VERIFY(decl);
	VERIFY(m_pInputSignature);

	xr_map<ID3DBlob*, ID3DInputLayout*>::iterator	it;

	it = decl->vs_to_layout.find(m_pInputSignature);

	if (it == decl->vs_to_layout.end())
	{
		ID3DInputLayout* pLayout;

		CHK_DX(HW.pDevice->CreateInputLayout(
			&decl->dx10_dcl_code[0],
			decl->dx10_dcl_code.size() - 1,
			m_pInputSignature->GetBufferPointer(),
			m_pInputSignature->GetBufferSize(),
			&pLayout
		));

		it = decl->vs_to_layout.insert(
			std::pair<ID3DBlob*, ID3DInputLayout*>(m_pInputSignature, pLayout)).first;
	}

	if (m_pInputLayout != it->second)
	{
		m_pInputLayout = it->second;
		HW.pContext->IASetInputLayout(m_pInputLayout);
	}
}

ICF void CBackend::set_VS(ref_vs& _vs)
{
	m_pInputSignature = _vs->signature->signature;
	set_VS(_vs->sh, _vs->cName.c_str());
}

ICF void CBackend::set_VS(SVS* _vs)
{
	m_pInputSignature = _vs->signature->signature;
	set_VS(_vs->sh, _vs->cName.c_str());
}

IC bool CBackend::CBuffersNeedUpdate(ref_cbuffer buf1[MaxCBuffers], ref_cbuffer buf2[MaxCBuffers], u32& uiMin, u32& uiMax)
{
	bool	bRes = false;
	int i = 0;
	while ((i < MaxCBuffers) && (buf1[i] == buf2[i]))
		++i;

	uiMin = i;

	for (; i < MaxCBuffers; ++i)
	{
		if (buf1[i] != buf2[i])
		{
			bRes = true;
			uiMax = i;
		}
	}

	return bRes;
}

IC void CBackend::set_Constants(R_constant_table* C)
{
	// caching
	if (ctable == C)	return;
	ctable = C;
	xforms.unmap();
	hemi.unmap();
	tree.unmap();

	LOD.unmap();

	StateManager.UnmapConstants();
	if (0 == C)		return;

	PGO(Msg("PGO:c-table"));


	//	Setup constant tables
	{
		ref_cbuffer	aPixelConstants[MaxCBuffers];
		ref_cbuffer	aVertexConstants[MaxCBuffers];
		ref_cbuffer	aGeometryConstants[MaxCBuffers];

		ref_cbuffer	aHullConstants[MaxCBuffers];
		ref_cbuffer	aDomainConstants[MaxCBuffers];
		ref_cbuffer	aComputeConstants[MaxCBuffers];


		for (int i = 0; i < MaxCBuffers; ++i)
		{
			aPixelConstants[i] = m_aPixelConstants[i];
			aVertexConstants[i] = m_aVertexConstants[i];
			aGeometryConstants[i] = m_aGeometryConstants[i];


			aHullConstants[i] = m_aHullConstants[i];
			aDomainConstants[i] = m_aDomainConstants[i];
			aComputeConstants[i] = m_aComputeConstants[i];


			m_aPixelConstants[i] = 0;
			m_aVertexConstants[i] = 0;
			m_aGeometryConstants[i] = 0;


			m_aHullConstants[i] = 0;
			m_aDomainConstants[i] = 0;
			m_aComputeConstants[i] = 0;

		}
		R_constant_table::cb_table::iterator	it = C->m_CBTable.begin();
		R_constant_table::cb_table::iterator	end = C->m_CBTable.end();
		for (; it != end; ++it)
		{
			//ID3DxxBuffer*	pBuffer = (it->second)->GetBuffer();
			u32				uiBufferIndex = it->first;

			if ((uiBufferIndex & CB_BufferTypeMask) == CB_BufferPixelShader)
			{
				VERIFY((uiBufferIndex & CB_BufferIndexMask) < MaxCBuffers);
				m_aPixelConstants[uiBufferIndex & CB_BufferIndexMask] = it->second;
			}
			else if ((uiBufferIndex & CB_BufferTypeMask) == CB_BufferVertexShader)
			{
				VERIFY((uiBufferIndex & CB_BufferIndexMask) < MaxCBuffers);
				m_aVertexConstants[uiBufferIndex & CB_BufferIndexMask] = it->second;
			}
			else if ((uiBufferIndex & CB_BufferTypeMask) == CB_BufferGeometryShader)
			{
				VERIFY((uiBufferIndex & CB_BufferIndexMask) < MaxCBuffers);
				m_aGeometryConstants[uiBufferIndex & CB_BufferIndexMask] = it->second;
			}

			else if ((uiBufferIndex & CB_BufferTypeMask) == CB_BufferHullShader)
			{
				VERIFY((uiBufferIndex & CB_BufferIndexMask) < MaxCBuffers);
				m_aHullConstants[uiBufferIndex & CB_BufferIndexMask] = it->second;
			}
			else if ((uiBufferIndex & CB_BufferTypeMask) == CB_BufferDomainShader)
			{
				VERIFY((uiBufferIndex & CB_BufferIndexMask) < MaxCBuffers);
				m_aDomainConstants[uiBufferIndex & CB_BufferIndexMask] = it->second;
			}
			else if ((uiBufferIndex & CB_BufferTypeMask) == CB_BufferComputeShader)
			{
				VERIFY((uiBufferIndex & CB_BufferIndexMask) < MaxCBuffers);
				m_aComputeConstants[uiBufferIndex & CB_BufferIndexMask] = it->second;
			}

			else
				VERIFY("Invalid enumeration");
		}

		ID3DBuffer* tempBuffer[MaxCBuffers];

		u32 uiMin;
		u32 uiMax;

		if (CBuffersNeedUpdate(m_aPixelConstants, aPixelConstants, uiMin, uiMax))
		{
			++uiMax;

			for (u32 i = uiMin; i < uiMax; ++i)
			{
				if (m_aPixelConstants[i])
					tempBuffer[i] = m_aPixelConstants[i]->GetBuffer();
				else
					tempBuffer[i] = 0;
			}

			HW.pContext->PSSetConstantBuffers(uiMin, uiMax - uiMin, &tempBuffer[uiMin]);
		}


		if (CBuffersNeedUpdate(m_aVertexConstants, aVertexConstants, uiMin, uiMax))
		{
			++uiMax;

			for (u32 i = uiMin; i < uiMax; ++i)
			{
				if (m_aVertexConstants[i])
					tempBuffer[i] = m_aVertexConstants[i]->GetBuffer();
				else
					tempBuffer[i] = 0;
			}
			HW.pContext->VSSetConstantBuffers(uiMin, uiMax - uiMin, &tempBuffer[uiMin]);
		}


		if (CBuffersNeedUpdate(m_aGeometryConstants, aGeometryConstants, uiMin, uiMax))
		{
			++uiMax;

			for (u32 i = uiMin; i < uiMax; ++i)
			{
				if (m_aGeometryConstants[i])
					tempBuffer[i] = m_aGeometryConstants[i]->GetBuffer();
				else
					tempBuffer[i] = 0;
			}
			HW.pContext->GSSetConstantBuffers(uiMin, uiMax - uiMin, &tempBuffer[uiMin]);
		}


		if (CBuffersNeedUpdate(m_aHullConstants, aHullConstants, uiMin, uiMax))
		{
			++uiMax;

			for (u32 i = uiMin; i < uiMax; ++i)
			{
				if (m_aHullConstants[i])
					tempBuffer[i] = m_aHullConstants[i]->GetBuffer();
				else
					tempBuffer[i] = 0;
			}
			HW.pContext->HSSetConstantBuffers(uiMin, uiMax - uiMin, &tempBuffer[uiMin]);
		}

		if (CBuffersNeedUpdate(m_aDomainConstants, aDomainConstants, uiMin, uiMax))
		{
			++uiMax;

			for (u32 i = uiMin; i < uiMax; ++i)
			{
				if (m_aDomainConstants[i])
					tempBuffer[i] = m_aDomainConstants[i]->GetBuffer();
				else
					tempBuffer[i] = 0;
			}
			HW.pContext->DSSetConstantBuffers(uiMin, uiMax - uiMin, &tempBuffer[uiMin]);
		}

		if (CBuffersNeedUpdate(m_aComputeConstants, aComputeConstants, uiMin, uiMax))
		{
			++uiMax;

			for (u32 i = uiMin; i < uiMax; ++i)
			{
				if (m_aComputeConstants[i])
					tempBuffer[i] = m_aComputeConstants[i]->GetBuffer();
				else
					tempBuffer[i] = 0;
			}
			HW.pContext->CSSetConstantBuffers(uiMin, uiMax - uiMin, &tempBuffer[uiMin]);
		}

		/*
		for (int i=0; i<MaxCBuffers; ++i)
		{
			if (m_aPixelConstants[i])
				tempBuffer[i] = m_aPixelConstants[i]->GetBuffer();
			else
				tempBuffer[i] = 0;
		}
		HW.pDevice->PSSetConstantBuffers(0, MaxCBuffers, tempBuffer);

		for (int i=0; i<MaxCBuffers; ++i)
		{
			if (m_aVertexConstants[i])
				tempBuffer[i] = m_aVertexConstants[i]->GetBuffer();
			else
				tempBuffer[i] = 0;
		}
		HW.pDevice->VSSetConstantBuffers(0, MaxCBuffers, tempBuffer);

		for (int i=0; i<MaxCBuffers; ++i)
		{
			if (m_aGeometryConstants[i])
				tempBuffer[i] = m_aGeometryConstants[i]->GetBuffer();
			else
				tempBuffer[i] = 0;
		}
		HW.pDevice->GSSetConstantBuffers(0, MaxCBuffers, tempBuffer);
		*/
	}

	// process constant-loaders
	R_constant_table::c_table::iterator	it = C->table.begin();
	R_constant_table::c_table::iterator	end = C->table.end();
	for (; it != end; it++)
	{
		R_constant* Cs = &**it;
		VERIFY(Cs);
		if (Cs && Cs->handler)
			Cs->handler->setup(Cs);
	}
}

ICF void CBackend::ApplyRTandZB()
{
	if (m_bChangedRTorZB)
	{
		m_bChangedRTorZB = false;
		HW.pContext->OMSetRenderTargets(sizeof(pRT) / sizeof(pRT[0]), pRT, pZB);
	}
}

IC	void CBackend::get_ConstantDirect(shared_str& n, u32 DataSize, void** pVData, void** pGData, void** pPData)
{
	ref_constant C = get_c(n);

	if (C)
		constants.access_direct(&*C, DataSize, pVData, pGData, pPData);
	else
	{
		if (pVData)	*pVData = 0;
		if (pGData)	*pGData = 0;
		if (pPData)	*pPData = 0;
	}
}

#endif
