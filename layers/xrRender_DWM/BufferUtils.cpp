#include "stdafx.h"
#include "BufferUtils.h"

namespace BufferUtils
{
	HRESULT	CreateBuffer(ID3DBuffer** ppBuffer, const void* pData, UINT DataSize, D3D_BIND_FLAG Bind)
	{
		D3D_BUFFER_DESC desc;
		desc.ByteWidth = DataSize;
		desc.Usage = D3D_USAGE_DEFAULT;
		desc.BindFlags = Bind;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D_SUBRESOURCE_DATA subData;
		subData.pSysMem = pData;

		HRESULT res = HW.pDevice->CreateBuffer(&desc, &subData, ppBuffer);

		return res;
	}
};