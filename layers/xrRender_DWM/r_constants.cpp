#include "stdafx.h"
#pragma hdrstop

#include "d3d11shader.h"

#include "ResourceManager.h"

#include "../xrCore/xrPool.h"
#include "r_constants.h"
#include "ConstantBuffer.h"
#include "dxRenderDeviceRender.h"

// pool
//.static	poolSS<R_constant,512>			g_constant_allocator;

//R_constant_table::~R_constant_table	()	{	dxRenderDeviceRender::Instance().Resources->_DeleteConstantTable(this);	}


R_constant_table::~R_constant_table	()	
{	
	//dxRenderDeviceRender::Instance().Resources->_DeleteConstantTable(this);	
	DEV->_DeleteConstantTable(this);
}


void	R_constant_table::fatal			(LPCSTR S)
{
	FATAL	(S);
}

// predicates
IC bool	p_search	(ref_constant C, LPCSTR S)
{
	return xr_strcmp(*C->name,S)<0;
}
IC bool	p_sort		(ref_constant C1, ref_constant C2)
{
	return xr_strcmp(C1->name,C2->name)<0;
}

ref_constant R_constant_table::get	(LPCSTR S)
{
	// assumption - sorted by name
	c_table::iterator I	= std::lower_bound(table.begin(),table.end(),S,p_search);
	if (I==table.end() || (0!=xr_strcmp(*(*I)->name,S)))	return 0;
	else												return *I;
}
ref_constant R_constant_table::get	(shared_str& S)
{
	// linear search, but only ptr-compare
	c_table::iterator I	= table.begin	();
	c_table::iterator E	= table.end		();
	for (; I!=E; ++I)	{
		ref_constant	C		= *I;
		if (C->name.equal(S))	return C;
	}
	return	0;
}

/// !!!!!!!!FIX THIS FOR DX11!!!!!!!!!
void R_constant_table::merge(R_constant_table* T)
{
	if (0==T)		return;

	// Real merge
	for (u32 it=0; it<T->table.size(); it++)
	{
		ref_constant src		=	T->table[it];
		ref_constant C			=	get	(*src->name);
		if (!C)	
		{
			C					=	xr_new<R_constant>();//.g_constant_allocator.create();
			C->name				=	src->name;
			C->destination		=	src->destination;
			C->type				=	src->type;
			C->ps				=	src->ps;
			C->vs				=	src->vs;

			C->gs				=	src->gs;

			C->hs				=	src->hs;
			C->ds				=	src->ds;
			C->cs				=	src->cs;


			C->samp				=	src->samp;
			table.push_back		(C);
		} 
		else 
		{
			VERIFY2(!(C->destination&src->destination&RC_dest_sampler), "Can't have samplers or textures with the same name for PS, VS and GS.");
			C->destination		|=	src->destination;
			VERIFY	(C->type	==	src->type);
			R_constant_load& sL	=	src->get_load(src->destination);
			R_constant_load& dL	=	C->get_load(src->destination);
			dL.index			=	sL.index;
			dL.cls				=	sL.cls;
		}
	}

	// Sort
	std::sort		(table.begin(),table.end(),p_sort);


	//	TODO:	DX10:	Implement merge with validity check
	m_CBTable.reserve( m_CBTable.size() + T->m_CBTable.size());
	for ( u32 i = 0; i < T->m_CBTable.size(); ++i )
		m_CBTable.push_back(T->m_CBTable[i]);
	
}

void R_constant_table::clear	()
{
	//.
	for (u32 it=0; it<table.size(); it++)
		table[it]	= 0;//.g_constant_allocator.destroy(table[it]);
	table.clear		();

	m_CBTable.clear();

}

BOOL R_constant_table::equal(R_constant_table& C)
{
	if (table.size() != C.table.size())	return FALSE;
	u32 size			= table.size();
	for (u32 it=0; it<size; it++)
	{
		if (!table[it]->equal(&*C.table[it]))	return FALSE;
	}

	return TRUE;
}

BOOL R_constant_table::parseConstants(ID3DShaderReflectionConstantBuffer* pTable, u32 destination)
{
	//VERIFY(_desc);
	VERIFY(pTable);
	D3D_SHADER_BUFFER_DESC	TableDesc;
	CHK_DX(pTable->GetDesc(&TableDesc));

	//D3DXSHADER_CONSTANTTABLE* desc	= (D3DXSHADER_CONSTANTTABLE*) _desc;
	//D3DXSHADER_CONSTANTINFO* it		= (D3DXSHADER_CONSTANTINFO*) (LPBYTE(desc)+desc->ConstantInfo);
	//LPBYTE					 ptr	= LPBYTE(desc);
	//for (u32 dwCount = desc->Constants; dwCount; dwCount--,it++)
	for (u32 i = 0; i < TableDesc.Variables; ++i)
	{
		ID3DShaderReflectionVariable* pVar;
		D3D_SHADER_VARIABLE_DESC		VarDesc;
		ID3DShaderReflectionType* pType;
		D3D_SHADER_TYPE_DESC			TypeDesc;

		pVar = pTable->GetVariableByIndex(i);
		VERIFY(pVar);
		pVar->GetDesc(&VarDesc);
		pType = pVar->GetType();
		VERIFY(pType);
		pType->GetDesc(&TypeDesc);

		// Name
		//LPCSTR	name		=	LPCSTR(ptr+it->Name);
		LPCSTR	name = VarDesc.Name;

		// Type
		//u16		type		=	RC_float;
		u16		type = u16(-1);
		switch (TypeDesc.Type)
		{
		case D3D_SVT_FLOAT:
			type = RC_float;
			break;
		case D3D_SVT_BOOL:
			type = RC_bool;
			break;
		case D3D_SVT_INT:
			type = RC_int;
			break;
		default:
			fatal("R_constant_table::parse: unexpected shader variable type.");
		}

		// Rindex,Rcount
		//u16		r_index		=	it->RegisterIndex;
		//	Used as byte offset in constant buffer
		VERIFY(VarDesc.StartOffset < 0x10000);
		u16		r_index = u16(VarDesc.StartOffset);
		u16		r_type = u16(-1);

		// TypeInfo + class
		//D3DXSHADER_TYPEINFO*	T	= (D3DXSHADER_TYPEINFO*)(ptr+it->TypeInfo);
		BOOL bSkip = FALSE;
		//switch (T->Class)
		switch (TypeDesc.Class)
		{
		case D3D_SVC_SCALAR:
			r_type = RC_1x1;
			break;
		case D3D_SVC_VECTOR:
		{
			switch (TypeDesc.Columns)
			{
			case 4:
				r_type = RC_1x4;
				break;
			case 3:
				r_type = RC_1x3;
				break;
			case 2:
				r_type = RC_1x2;
				break;
			default:
				fatal("Vector: 1 components is scalar - there is special case for this!!!!!");
				break;
			}
		}
		break;
		case D3D_SVC_MATRIX_ROWS:
		{
			switch (TypeDesc.Columns)
			{
			case 4:
				switch (TypeDesc.Rows)
				{
				case 2:
					r_type = RC_2x4;
					break;
				case 3:
					r_type = RC_3x4;
					break;
					/*
					switch (it->RegisterCount)
					{
					case 2:	r_type	=	RC_2x4;	break;
					case 3: r_type	=	RC_3x4;	break;
					default:
					fatal		("MATRIX_ROWS: unsupported number of RegisterCount");
					break;
					}
					break;
					*/
				case 4:
					r_type = RC_4x4;
					//VERIFY(4 == it->RegisterCount); 
					break;
				default:
					fatal("MATRIX_ROWS: unsupported number of Rows");
					break;
				}
				break;
			default:
				fatal("MATRIX_ROWS: unsupported number of Columns");
				break;
			}
		}
		break;
		case D3D_SVC_MATRIX_COLUMNS:
			fatal("Pclass MATRIX_COLUMNS unsupported");
			break;
		case D3D_SVC_STRUCT:
			fatal("Pclass D3DXPC_STRUCT unsupported");
			break;
		case D3D_SVC_OBJECT:
		{
			//	TODO: DX10: 
			VERIFY(!"Implement shader object parsing.");
			/*
			switch (T->Type)
			{
			case D3DXPT_SAMPLER:
			case D3DXPT_SAMPLER1D:
			case D3DXPT_SAMPLER2D:
			case D3DXPT_SAMPLER3D:
			case D3DXPT_SAMPLERCUBE:
			{
			// ***Register sampler***
			// We have determined all valuable info, search if constant already created
			ref_constant	C		=	get	(name);
			if (!C)	{
			C					=	xr_new<R_constant>();//.g_constant_allocator.create();
			C->name				=	name;
			C->destination		=	RC_dest_sampler;
			C->type				=	RC_sampler;
			R_constant_load& L	=	C->samp;
			L.index				=	u16(r_index	+ ( (destination&1)? 0 : D3DVERTEXTEXTURESAMPLER0 ));
			L.cls				=	RC_sampler	;
			table.push_back		(C);
			} else {
			R_ASSERT			(C->destination	==	RC_dest_sampler);
			R_ASSERT			(C->type		==	RC_sampler);
			R_constant_load& L	=	C->samp;
			R_ASSERT			(L.index		==	r_index);
			R_ASSERT			(L.cls			==	RC_sampler);
			}
			}
			break;
			default:
			fatal		("Pclass D3DXPC_OBJECT - object isn't of 'sampler' type");
			break;
			}
			*/
		}
		bSkip = TRUE;
		break;
		default:
			bSkip = TRUE;
			break;
		}
		if (bSkip)			continue;

		// We have determined all valuable info, search if constant already created
		ref_constant	C = get(name);
		if (!C) {
			C = xr_new<R_constant>();//.g_constant_allocator.create();
			C->name = name;
			C->destination = destination;
			C->type = type;
			//R_constant_load& L	=	(destination&1)?C->ps:C->vs;
			R_constant_load& L = C->get_load(destination);/*((destination&RC_dest_pixel)
									? C->ps : (destination&RC_dest_vertex)
									? C->vs : C->gs);*/
			L.index = r_index;
			L.cls = r_type;
			table.push_back(C);
		}
		else {
			C->destination |= destination;
			VERIFY(C->type == type);
			//R_constant_load& L	=	(destination&1)?C->ps:C->vs;
			R_constant_load& L = C->get_load(destination);/*((destination&RC_dest_pixel)
									? C->ps : (destination&RC_dest_vertex)
									? C->vs : C->gs);*/
			L.index = r_index;
			L.cls = r_type;
		}
	}
	return TRUE;
}

BOOL R_constant_table::parseResources(ID3DShaderReflection* pReflection, int ResNum, u32 destination)
{
	for (int i = 0; i < ResNum; ++i)
	{
		D3D_SHADER_INPUT_BIND_DESC	ResDesc;
		pReflection->GetResourceBindingDesc(i, &ResDesc);

		u16	type = 0;

		switch (ResDesc.Type)
		{
		case D3D_SIT_TEXTURE:
			type = RC_dx10texture;
			break;
		case D3D_SIT_SAMPLER:
			type = RC_sampler;
			break;
		case D3D_SIT_UAV_RWTYPED:
			type = RC_dx11UAV;
			break;
		default:
			continue;
		}

		VERIFY(ResDesc.BindCount == 1);

		//u16	r_index = u16( ResDesc.BindPoint + ((destination&1)? 0 : CTexture::rstVertex) );

		u16	r_index = u16(-1);

		if (destination & RC_dest_pixel)
		{
			r_index = u16(ResDesc.BindPoint + CTexture::rstPixel);
		}
		else if (destination & RC_dest_vertex)
		{
			r_index = u16(ResDesc.BindPoint + CTexture::rstVertex);
		}
		else if (destination & RC_dest_geometry)
		{
			r_index = u16(ResDesc.BindPoint + CTexture::rstGeometry);
		}
		else if (destination & RC_dest_hull)
		{
			r_index = u16(ResDesc.BindPoint + CTexture::rstHull);
		}
		else if (destination & RC_dest_domain)
		{
			r_index = u16(ResDesc.BindPoint + CTexture::rstDomain);
		}
		else if (destination & RC_dest_compute)
		{
			r_index = u16(ResDesc.BindPoint + CTexture::rstCompute);
		}
		else
		{
			VERIFY(0);
		}

		ref_constant	C = get(ResDesc.Name);
		if (!C)
		{
			C = xr_new<R_constant>();//.g_constant_allocator.create();
			C->name = ResDesc.Name;
			C->destination = RC_dest_sampler;
			C->type = type;
			R_constant_load& L = C->samp;
			L.index = r_index;
			L.cls = type;
			table.push_back(C);
		}
		else
		{
			R_ASSERT(C->destination == RC_dest_sampler);
			R_ASSERT(C->type == type);
			R_constant_load& L = C->samp;
			R_ASSERT(L.index == r_index);
			R_ASSERT(L.cls == type);
		}

	}
	return TRUE;
}

IC u32 dest_to_shift_value(u32 destination)
{
	switch (destination & 0xFF)
	{
	case RC_dest_vertex:
		return RC_dest_vertex_cb_index_shift;
	case RC_dest_pixel:
		return RC_dest_pixel_cb_index_shift;
	case RC_dest_geometry:
		return RC_dest_geometry_cb_index_shift;
	case RC_dest_hull:
		return RC_dest_hull_cb_index_shift;
	case RC_dest_domain:
		return RC_dest_domain_cb_index_shift;
	case RC_dest_compute:
		return RC_dest_compute_cb_index_shift;

	default:
		FATAL("invalid enumeration for shader");
	}
	return 0;
}

IC u32 dest_to_cbuf_type(u32 destination)
{
	switch (destination & 0xFF)
	{
	case RC_dest_vertex:
		return CB_BufferVertexShader;
	case RC_dest_pixel:
		return CB_BufferPixelShader;
	case RC_dest_geometry:
		return CB_BufferGeometryShader;
	case RC_dest_hull:
		return CB_BufferHullShader;
	case RC_dest_domain:
		return CB_BufferDomainShader;
	case RC_dest_compute:
		return CB_BufferComputeShader;

	default:
		FATAL("invalid enumeration for shader");
	}
	return 0;
}

BOOL	R_constant_table::parse(void* _desc, u32 destination)
{
	ID3DShaderReflection* pReflection = (ID3DShaderReflection*)_desc;

	D3D_SHADER_DESC	ShaderDesc;
	pReflection->GetDesc(&ShaderDesc);

	if (ShaderDesc.ConstantBuffers)
	{
		m_CBTable.reserve(ShaderDesc.ConstantBuffers);
		//	Parse single constant table
		ID3DShaderReflectionConstantBuffer* pTable = 0;

		for (u16 iBuf = 0; iBuf < ShaderDesc.ConstantBuffers; ++iBuf)
		{
			pTable = pReflection->GetConstantBufferByIndex(iBuf);
			if (pTable)
			{
				//	Encode buffer index into destination
				u32	updatedDest = destination;
				updatedDest |= iBuf << dest_to_shift_value(destination);/*((destination&RC_dest_pixel)
					? RC_dest_pixel_cb_index_shift : (destination&RC_dest_vertex)
					? RC_dest_vertex_cb_index_shift : RC_dest_geometry_cb_index_shift);*/

					//	Encode bind dest (pixel/vertex buffer) and bind point index
				u32	uiBufferIndex = iBuf;
				uiBufferIndex |= dest_to_cbuf_type(destination);/*(destination&RC_dest_pixel)
					? CB_BufferPixelShader : (destination&RC_dest_vertex)
					? CB_BufferVertexShader : CB_BufferGeometryShader;*/

				parseConstants(pTable, updatedDest);
				ref_cbuffer	tempBuffer = dxRenderDeviceRender::Instance().Resources->_CreateConstantBuffer(pTable);
				m_CBTable.push_back(cb_table_record(uiBufferIndex, tempBuffer));
			}
		}
	}

	if (ShaderDesc.BoundResources)
	{
		parseResources(pReflection, ShaderDesc.BoundResources, destination);
	}

	std::sort(table.begin(), table.end(), p_sort);
	return		TRUE;
}