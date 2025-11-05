#ifndef	dx10BufferUtils_included
#define	dx10BufferUtils_included
#pragma once

#define BufferUtils dx10BufferUtils

namespace dx10BufferUtils
{
ENGINE_API HRESULT  CreateVertexBuffer(ID3D11Buffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
ENGINE_API HRESULT  CreateIndexBuffer(ID3D11Buffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
ENGINE_API HRESULT	 CreateConstantBuffer( ID3D11Buffer** ppBuffer, UINT DataSize);
ENGINE_API void	 ConvertVertexDeclaration( const xr_vector<D3DVERTEXELEMENT9> &declIn, xr_vector<D3D11_INPUT_ELEMENT_DESC> &declOut);
};

#endif	//	dx10BufferUtils_included