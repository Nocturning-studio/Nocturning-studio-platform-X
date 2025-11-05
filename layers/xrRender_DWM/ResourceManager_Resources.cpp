#include "stdafx.h"
#pragma hdrstop

#include <D3DX11core.h>

#include "ResourceManager.h"
#include "tss.h"
#include "blender.h"
#include "blender_recorder.h"

#include "BufferUtils.h"

#include "ConstantBuffer.h"

#include "ShaderResourceTraits.h"

SVS* CResourceManager::_CreateVS(LPCSTR Name)
{
	string_path _name;
	xr_strcpy(_name, Name);

	switch (::Render->m_skinning)
	{
	case 0:		xr_strcat(_name, "_0");		break;
	case 1:		xr_strcat(_name, "_1");		break;
	case 2:		xr_strcat(_name, "_2");		break;
	case 3:		xr_strcat(_name, "_3");		break;
	case 4:		xr_strcat(_name, "_4");		break;
	}

	return CreateShader<SVS>(_name, Name);
}

void CResourceManager::_DeleteVS(const SVS* S)
{
	if (DestroyShader<SVS>(S))
	{
		xr_vector<SDeclaration*>::iterator iDecl;
		for (iDecl = v_declarations.begin(); iDecl != v_declarations.end(); ++iDecl)
		{
			xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;
			iLayout = (*iDecl)->vs_to_layout.find(S->signature->signature);

			if (iLayout != (*iDecl)->vs_to_layout.end())
			{
				//	Release vertex layout
				_RELEASE(iLayout->second);
				(*iDecl)->vs_to_layout.erase(iLayout);
			}
		}

		return;
	}

	Msg("! ERROR: Failed to find compiled vertex-shader '%s'", *S->cName);
}

SPS* CResourceManager::_CreatePS(LPCSTR Name) { return CreateShader<SPS>(Name); }
void CResourceManager::_DeletePS(const SPS* S) { DestroyShader(S); }

SGS* CResourceManager::_CreateGS(LPCSTR Name) { return CreateShader<SGS>(Name); }
void CResourceManager::_DeleteGS(const SGS* S) { DestroyShader(S); }

SHS* CResourceManager::_CreateHS(LPCSTR Name) { return CreateShader<SHS>(Name); }
void CResourceManager::_DeleteHS(const SHS* S) { DestroyShader(S); }

SDS* CResourceManager::_CreateDS(LPCSTR Name) { return CreateShader<SDS>(Name); }
void CResourceManager::_DeleteDS(const SDS* S) { DestroyShader(S); }

SCS* CResourceManager::_CreateCS(LPCSTR Name) { return CreateShader<SCS>(Name); }
void CResourceManager::_DeleteCS(const SCS* S) { DestroyShader(S); }

void fix_texture_name(LPSTR fn);

template <class T>
BOOL	reclaim		(xr_vector<T*>& vec, const T* ptr)
{
	xr_vector<T*>::iterator it	= vec.begin	();
	xr_vector<T*>::iterator end	= vec.end	();
	for (; it!=end; it++)
		if (*it == ptr)	{ vec.erase	(it); return TRUE; }
		return FALSE;
}

//--------------------------------------------------------------------------------------------------------------
SState*		CResourceManager::_CreateState		(SimulatorStates& state_code)
{
	// Search equal state-code 
	for (u32 it=0; it<v_states.size(); it++)
	{
		SState*				C		= v_states[it];;
		SimulatorStates&	base	= C->state_code;
		if (base.equal(state_code))	return C;
	}

	// Create New
	v_states.push_back				(xr_new<SState>());
	v_states.back()->dwFlags		|= xr_resource_flagged::RF_REGISTERED;

	v_states.back()->state			= ID3DState::Create(state_code);

	v_states.back()->state_code		= state_code;
	return v_states.back();
}
void		CResourceManager::_DeleteState		(const SState* state)
{
	if (0==(state->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_states,state))						return;
	Msg	("! ERROR: Failed to find compiled stateblock");
}

//--------------------------------------------------------------------------------------------------------------
SPass*		CResourceManager::_CreatePass			(const SPass& proto)
{
	for (u32 it=0; it<v_passes.size(); it++)
		if (v_passes[it]->equal(proto))
			return v_passes[it];

	SPass*	P					=	xr_new<SPass>();
	P->dwFlags					|=	xr_resource_flagged::RF_REGISTERED;
	P->state					=	proto.state;
	P->ps						=	proto.ps;
	P->vs						=	proto.vs;
	P->gs						=	proto.gs;

	P->hs						=	proto.hs;
	P->ds						=	proto.ds;
	P->cs						=	proto.cs;

	P->constants				=	proto.constants;
	P->T						=	proto.T;
#ifdef _EDITOR
	P->M						=	proto.M;
#endif
	P->C						=	proto.C;

	v_passes.push_back			(P);
	return v_passes.back();
}

void		CResourceManager::_DeletePass			(const SPass* P)
{
	if (0==(P->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_passes,P))						return;
	Msg	("! ERROR: Failed to find compiled pass");
}

//--------------------------------------------------------------------------------------------------------------
static BOOL	dcl_equal			(D3DVERTEXELEMENT9* a, D3DVERTEXELEMENT9* b)
{
	// check sizes
	u32 a_size	= D3DXGetDeclLength(a);
	u32 b_size	= D3DXGetDeclLength(b);
	if (a_size!=b_size)	return FALSE;
	return 0==memcmp	(a,b,a_size*sizeof(D3DVERTEXELEMENT9));
}

struct VertexFormatPairs
{
	D3DDECLTYPE	m_dx9FMT;
	DXGI_FORMAT	m_dx10FMT;
};

VertexFormatPairs	VertexFormatList[] =
{
	{ D3DDECLTYPE_FLOAT1,	DXGI_FORMAT_R32_FLOAT },
	{ D3DDECLTYPE_FLOAT2,	DXGI_FORMAT_R32G32_FLOAT },
	{ D3DDECLTYPE_FLOAT3,	DXGI_FORMAT_R32G32B32_FLOAT },
	{ D3DDECLTYPE_FLOAT4,	DXGI_FORMAT_R32G32B32A32_FLOAT },
	{ D3DDECLTYPE_D3DCOLOR,	DXGI_FORMAT_R8G8B8A8_UNORM },	// Warning. Explicit RGB component swizzling is nesessary	//	Not available 
	{ D3DDECLTYPE_UBYTE4,	DXGI_FORMAT_R8G8B8A8_UINT },	// Note: Shader gets UINT values, but if Direct3D 9 style integral floats are needed (0.0f, 1.0f... 255.f), UINT can just be converted to float32 in shader. 
	{ D3DDECLTYPE_SHORT2,	DXGI_FORMAT_R16G16_SINT },		// Note: Shader gets SINT values, but if Direct3D 9 style integral floats are needed, SINT can just be converted to float32 in shader. 
	{ D3DDECLTYPE_SHORT4,	DXGI_FORMAT_R16G16B16A16_SINT },// Note: Shader gets SINT values, but if Direct3D 9 style integral floats are needed, SINT can just be converted to float32 in shader. 
	{ D3DDECLTYPE_UBYTE4N,	DXGI_FORMAT_R8G8B8A8_UNORM },
	{ D3DDECLTYPE_SHORT2N,	DXGI_FORMAT_R16G16_SNORM },
	{ D3DDECLTYPE_SHORT4N,	DXGI_FORMAT_R16G16B16A16_SNORM },
	{ D3DDECLTYPE_USHORT2N,	DXGI_FORMAT_R16G16_UNORM },
	{ D3DDECLTYPE_USHORT4N,	DXGI_FORMAT_R16G16B16A16_UNORM },
	//D3DDECLTYPE_UDEC3 Not available 
	//D3DDECLTYPE_DEC3N Not available 
	{ D3DDECLTYPE_FLOAT16_2,DXGI_FORMAT_R16G16_FLOAT },
	{ D3DDECLTYPE_FLOAT16_4,DXGI_FORMAT_R16G16B16A16_FLOAT }
};

DXGI_FORMAT	ConvertVertexFormat(D3DDECLTYPE dx9FMT)
{
	int arrayLength = sizeof(VertexFormatList) / sizeof(VertexFormatList[0]);
	for (int i = 0; i < arrayLength; ++i)
	{
		if (VertexFormatList[i].m_dx9FMT == dx9FMT)
			return VertexFormatList[i].m_dx10FMT;
	}

	VERIFY(!"ConvertVertexFormat didn't find appropriate dx10 vertex format!");
	return DXGI_FORMAT_UNKNOWN;
}

struct VertexSemanticPairs
{
	D3DDECLUSAGE	m_dx9Semantic;
	LPCSTR			m_dx10Semantic;
};

VertexSemanticPairs	VertexSemanticList[] =
{
	{ D3DDECLUSAGE_POSITION,		"POSITION" },		//	0
	{ D3DDECLUSAGE_BLENDWEIGHT,		"BLENDWEIGHT" },	// 1
	{ D3DDECLUSAGE_BLENDINDICES,	"BLENDINDICES" },	// 2
	{ D3DDECLUSAGE_NORMAL,			"NORMAL" },			// 3
	{ D3DDECLUSAGE_PSIZE,			"PSIZE" },			// 4
	{ D3DDECLUSAGE_TEXCOORD,		"TEXCOORD" },		// 5
	{ D3DDECLUSAGE_TANGENT,			"TANGENT" },		// 6
	{ D3DDECLUSAGE_BINORMAL,		"BINORMAL" },		// 7
	//D3DDECLUSAGE_TESSFACTOR,    // 8
	{ D3DDECLUSAGE_POSITIONT,		"POSITIONT" },		// 9
	{ D3DDECLUSAGE_COLOR,			"COLOR" },			// 10
	//D3DDECLUSAGE_FOG,           // 11
	//D3DDECLUSAGE_DEPTH,         // 12
	//D3DDECLUSAGE_SAMPLE,        // 13
};

LPCSTR	ConvertSemantic(D3DDECLUSAGE Semantic)
{
	int arrayLength = sizeof(VertexSemanticList) / sizeof(VertexSemanticList[0]);
	for (int i = 0; i < arrayLength; ++i)
	{
		if (VertexSemanticList[i].m_dx9Semantic == Semantic)
			return VertexSemanticList[i].m_dx10Semantic;
	}

	VERIFY(!"ConvertSemantic didn't find appropriate dx10 input semantic!");
	return 0;
}

void ConvertVertexDeclaration(const xr_vector<D3DVERTEXELEMENT9>& declIn, xr_vector<D3D_INPUT_ELEMENT_DESC>& declOut)
{
	int iDeclSize = declIn.size() - 1;
	declOut.resize(iDeclSize + 1);

	for (int i = 0; i < iDeclSize; ++i)
	{
		const D3DVERTEXELEMENT9& descIn = declIn[i];
		D3D_INPUT_ELEMENT_DESC& descOut = declOut[i];

		descOut.SemanticName = ConvertSemantic((D3DDECLUSAGE)descIn.Usage);
		descOut.SemanticIndex = descIn.UsageIndex;
		descOut.Format = ConvertVertexFormat((D3DDECLTYPE)descIn.Type);
		descOut.InputSlot = descIn.Stream;
		descOut.AlignedByteOffset = descIn.Offset;
		descOut.InputSlotClass = D3D_INPUT_PER_VERTEX_DATA;
		descOut.InstanceDataStepRate = 0;
	}

	ZeroMemory(&declOut[iDeclSize], sizeof(declOut[iDeclSize]));
}

SDeclaration*	CResourceManager::_CreateDecl	(D3DVERTEXELEMENT9* dcl)
{
	// Search equal code
	for (u32 it=0; it<v_declarations.size(); it++)
	{
		SDeclaration*		D		= v_declarations[it];;
		if (dcl_equal(dcl,&*D->dcl_code.begin()))	return D;
	}

	// Create _new
	SDeclaration* D			= xr_new<SDeclaration>();
	u32 dcl_size			= D3DXGetDeclLength(dcl)+1;
	//	Don't need it for DirectX 10 here
	//CHK_DX					(HW.pDevice->CreateVertexDeclaration(dcl,&D->dcl));
	D->dcl_code.assign		(dcl,dcl+dcl_size);
	ConvertVertexDeclaration(D->dcl_code, D->dx10_dcl_code);
	D->dwFlags				|= xr_resource_flagged::RF_REGISTERED;
	v_declarations.push_back(D);
	return D;
}

void		CResourceManager::_DeleteDecl		(const SDeclaration* dcl)
{
	if (0==(dcl->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_declarations,dcl))					return;
	Msg	("! ERROR: Failed to find compiled vertex-declarator");
}

//--------------------------------------------------------------------------------------------------------------
R_constant_table*	CResourceManager::_CreateConstantTable	(R_constant_table& C)
{
	if (C.empty())		return NULL;

	for (u32 it=0; it<v_constant_tables.size(); it++)
		if (v_constant_tables[it]->equal(C))	return v_constant_tables[it];
	v_constant_tables.push_back			(xr_new<R_constant_table>(C));
	v_constant_tables.back()->dwFlags	|=	xr_resource_flagged::RF_REGISTERED;
	return v_constant_tables.back		();
}
void				CResourceManager::_DeleteConstantTable	(const R_constant_table* C)
{
	if (0==(C->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_constant_tables,C))				return;
	Msg	("! ERROR: Failed to find compiled constant-table");
}

//--------------------------------------------------------------------------------------------------------------
CRT*	CResourceManager::_CreateRT		(LPCSTR name, u32 w, u32 h, DXGI_FORMAT f, VIEW_TYPE view, u32 samples)
{
	R_ASSERT(name && name[0] && w && h);

	// ***** first pass - search already created RT
	LPSTR N = LPSTR(name);
	map_RT::iterator I = m_rtargets.find	(N);
	if (I!=m_rtargets.end())	return		I->second;
	else
	{
		CRT *RT					=	xr_new<CRT>();
		RT->dwFlags				|=	xr_resource_flagged::RF_REGISTERED;
		m_rtargets.insert		(mk_pair(RT->set_name(name),RT));

		if (Device.b_is_Ready)	RT->create	(name, w, h, f, view, samples);

		return					RT;
	}
}
void	CResourceManager::_DeleteRT		(const CRT* RT)
{
	if (0==(RT->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	LPSTR N				= LPSTR		(*RT->cName);
	map_RT::iterator I	= m_rtargets.find	(N);
	if (I!=m_rtargets.end())	{
		m_rtargets.erase(I);
		return;
	}
	Msg	("! ERROR: Failed to find render-target '%s'",*RT->cName);
}
//--------------------------------------------------------------------------------------------------------------
void	CResourceManager::DBG_VerifyGeoms	()
{
}

SGeometry*	CResourceManager::CreateGeom	(D3DVERTEXELEMENT9* decl, ID3DBuffer* vb, ID3DBuffer* ib)
{
	R_ASSERT			(decl && vb);

	SDeclaration* dcl	= _CreateDecl			(decl);
	u32 vb_stride		= D3DXGetDeclVertexSize	(decl,0);

	// ***** first pass - search already loaded shader
	for (u32 it=0; it<v_geoms.size(); it++)
	{
		SGeometry& G	= *(v_geoms[it]);
		if ((G.dcl==dcl) && (G.vb==vb) && (G.ib==ib) && (G.vb_stride==vb_stride))	return v_geoms[it];
	}

	SGeometry *Geom		=	xr_new<SGeometry>	();
	Geom->dwFlags		|=	xr_resource_flagged::RF_REGISTERED;
	Geom->dcl			=	dcl;
	Geom->vb			=	vb;
	Geom->vb_stride		=	vb_stride;
	Geom->ib			=	ib;
	v_geoms.push_back	(Geom);
	return	Geom;
}
SGeometry*	CResourceManager::CreateGeom		(u32 FVF, ID3DBuffer* vb, ID3DBuffer* ib)
{
	D3DVERTEXELEMENT9	dcl	[MAX_FVF_DECL_SIZE];
	CHK_DX				(D3DXDeclaratorFromFVF(FVF,dcl));
	SGeometry* g		=  CreateGeom	(dcl,vb,ib);
	return	g;
}

void		CResourceManager::DeleteGeom		(const SGeometry* Geom)
{
	if (0==(Geom->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_geoms,Geom))							return;
	Msg	("! ERROR: Failed to find compiled geometry-declaration");
}

//--------------------------------------------------------------------------------------------------------------
CTexture* CResourceManager::_CreateTexture	(LPCSTR _Name)
{
	// DBG_VerifyTextures	();
	if (0==xr_strcmp(_Name,"null"))	return 0;
	R_ASSERT		(_Name && _Name[0]);
	string_path		Name;
	xr_strcpy			(Name,_Name); //. andy if (strext(Name)) *strext(Name)=0;
	fix_texture_name (Name);
	// ***** first pass - search already loaded texture
	LPSTR N			= LPSTR(Name);
	map_TextureIt I = m_textures.find	(N);
	if (I!=m_textures.end())	return	I->second;
	else
	{
		CTexture *	T		=	xr_new<CTexture>();
		T->dwFlags			|=	xr_resource_flagged::RF_REGISTERED;
		m_textures.insert	(mk_pair(T->set_name(Name),T));
		T->Preload			();
		if (Device.b_is_Ready && !bDeferredLoad) T->Load();
		return		T;
	}
}
void	CResourceManager::_DeleteTexture		(const CTexture* T)
{
	// DBG_VerifyTextures	();

	if (0==(T->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	LPSTR N					= LPSTR		(*T->cName);
	map_Texture::iterator I	= m_textures.find	(N);
	if (I!=m_textures.end())	{
		m_textures.erase(I);
		return;
	}
	Msg	("! ERROR: Failed to find texture surface '%s'",*T->cName);
}

#ifdef DEBUG
void	CResourceManager::DBG_VerifyTextures	()
{
	map_Texture::iterator I		= m_textures.begin	();
	map_Texture::iterator E		= m_textures.end	();
	for (; I!=E; I++) 
	{
		R_ASSERT(I->first);
		R_ASSERT(I->second);
		R_ASSERT(I->second->cName);
		R_ASSERT(0==xr_strcmp(I->first,*I->second->cName));
	}
}
#endif

//--------------------------------------------------------------------------------------------------------------
CMatrix*	CResourceManager::_CreateMatrix	(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);
	if (0==stricmp(Name,"$null"))	return NULL;

	LPSTR N = LPSTR(Name);
	map_Matrix::iterator I = m_matrices.find	(N);
	if (I!=m_matrices.end())	return I->second;
	else
	{
		CMatrix* M			=	xr_new<CMatrix>();
		M->dwFlags			|=	xr_resource_flagged::RF_REGISTERED;
		M->dwReference		=	1;
		m_matrices.insert	(mk_pair(M->set_name(Name),M));
		return			M;
	}
}
void	CResourceManager::_DeleteMatrix		(const CMatrix* M)
{
	if (0==(M->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	LPSTR N					= LPSTR		(*M->cName);
	map_Matrix::iterator I	= m_matrices.find	(N);
	if (I!=m_matrices.end())	{
		m_matrices.erase(I);
		return;
	}
	Msg	("! ERROR: Failed to find xform-def '%s'",*M->cName);
}
void	CResourceManager::ED_UpdateMatrix		(LPCSTR Name, CMatrix* data)
{
	CMatrix*	M	= _CreateMatrix	(Name);
	*M				= *data;
}
//--------------------------------------------------------------------------------------------------------------
CConstant*	CResourceManager::_CreateConstant	(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);
	if (0==stricmp(Name,"$null"))	return NULL;

	LPSTR N = LPSTR(Name);
	map_Constant::iterator I	= m_constants.find	(N);
	if (I!=m_constants.end())	return I->second;
	else
	{
		CConstant* C		=	xr_new<CConstant>();
		C->dwFlags			|=	xr_resource_flagged::RF_REGISTERED;
		C->dwReference		=	1;
		m_constants.insert	(mk_pair(C->set_name(Name),C));
		return	C;
	}
}
void	CResourceManager::_DeleteConstant		(const CConstant* C)
{
	if (0==(C->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	LPSTR N				= LPSTR				(*C->cName);
	map_Constant::iterator I	= m_constants.find	(N);
	if (I!=m_constants.end())	{
		m_constants.erase(I);
		return;
	}
	Msg	("! ERROR: Failed to find R1-constant-def '%s'",*C->cName);
}

void	CResourceManager::ED_UpdateConstant	(LPCSTR Name, CConstant* data)
{
	CConstant*	C	= _CreateConstant	(Name);
	*C				= *data;
}

//--------------------------------------------------------------------------------------------------------------
bool	cmp_tl	(const std::pair<u32,ref_texture>& _1, const std::pair<u32,ref_texture>& _2)	{
	return _1.first < _2.first;
}
STextureList*	CResourceManager::_CreateTextureList(STextureList& L)
{
	std::sort	(L.begin(),L.end(),cmp_tl);
	for (u32 it=0; it<lst_textures.size(); it++)
	{
		STextureList*	base		= lst_textures[it];
		if (L.equal(*base))			return base;
	}
	STextureList*	lst		=	xr_new<STextureList>(L);
	lst->dwFlags			|=	xr_resource_flagged::RF_REGISTERED;
	lst_textures.push_back	(lst);
	return lst;
}
void			CResourceManager::_DeleteTextureList(const STextureList* L)
{
	if (0==(L->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(lst_textures,L))					return;
	Msg	("! ERROR: Failed to find compiled list of textures");
}
//--------------------------------------------------------------------------------------------------------------
SMatrixList*	CResourceManager::_CreateMatrixList(SMatrixList& L)
{
	BOOL bEmpty = TRUE;
	for (u32 i=0; i<L.size(); i++)	if (L[i]) { bEmpty=FALSE; break; }
	if (bEmpty)	return NULL;

	for (u32 it=0; it<lst_matrices.size(); it++)
	{
		SMatrixList*	base		= lst_matrices[it];
		if (L.equal(*base))			return base;
	}
	SMatrixList*	lst		=	xr_new<SMatrixList>(L);
	lst->dwFlags			|=	xr_resource_flagged::RF_REGISTERED;
	lst_matrices.push_back	(lst);
	return lst;
}
void			CResourceManager::_DeleteMatrixList ( const SMatrixList* L )
{
	if (0==(L->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(lst_matrices,L))					return;
	Msg	("! ERROR: Failed to find compiled list of xform-defs");
}
//--------------------------------------------------------------------------------------------------------------
SConstantList*	CResourceManager::_CreateConstantList(SConstantList& L)
{
	BOOL bEmpty = TRUE;
	for (u32 i=0; i<L.size(); i++)	if (L[i]) { bEmpty=FALSE; break; }
	if (bEmpty)	return NULL;

	for (u32 it=0; it<lst_constants.size(); it++)
	{
		SConstantList*	base		= lst_constants[it];
		if (L.equal(*base))			return base;
	}
	SConstantList*	lst		=	xr_new<SConstantList>(L);
	lst->dwFlags			|=	xr_resource_flagged::RF_REGISTERED;
	lst_constants.push_back	(lst);
	return lst;
}
void			CResourceManager::_DeleteConstantList(const SConstantList* L )
{
	if (0==(L->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(lst_constants,L))					return;
	Msg	("! ERROR: Failed to find compiled list of r1-constant-defs");
}
//--------------------------------------------------------------------------------------------------------------
dx10ConstantBuffer* CResourceManager::_CreateConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable)
{
	VERIFY(pTable);
	dx10ConstantBuffer	*pTempBuffer = xr_new<dx10ConstantBuffer>(pTable);

	for (u32 it=0; it<v_constant_buffer.size(); it++)
	{
		dx10ConstantBuffer*	buf		= v_constant_buffer[it];
		if (pTempBuffer->Similar(*buf))			
		{
			xr_delete(pTempBuffer);
			return buf;
		}
	}

	pTempBuffer->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_constant_buffer.push_back	(pTempBuffer);
	return pTempBuffer;
}
//--------------------------------------------------------------------------------------------------------------
void CResourceManager::_DeleteConstantBuffer(const dx10ConstantBuffer* pBuffer)
{
	if (0==(pBuffer->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_constant_buffer,pBuffer))						return;
	Msg	("! ERROR: Failed to find compiled constant buffer");
}

//--------------------------------------------------------------------------------------------------------------
SInputSignature* CResourceManager::_CreateInputSignature(ID3DBlob* pBlob)
{
	VERIFY(pBlob);

	for (u32 it=0; it<v_input_signature.size(); it++)
	{
		SInputSignature*	sign		= v_input_signature[it];
		if ( (pBlob->GetBufferSize() == sign->signature->GetBufferSize()) &&
			(!(memcmp(pBlob->GetBufferPointer(), sign->signature->GetBufferPointer(), pBlob->GetBufferSize()))))
		{
			return sign;
		}
	}

	SInputSignature	*pSign = xr_new<SInputSignature>(pBlob);

	pSign->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_input_signature.push_back	(pSign);

	return pSign;
}
//--------------------------------------------------------------------------------------------------------------
void CResourceManager::_DeleteInputSignature(const SInputSignature* pSignature)
{
	if (0==(pSignature->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_input_signature, pSignature))						return;
	Msg	("! ERROR: Failed to find compiled constant buffer");
}