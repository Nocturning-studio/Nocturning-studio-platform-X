#pragma once

namespace BufferUtils
{
	HRESULT CreateBuffer(ID3DBuffer** ppBuffer, const void* pData, UINT DataSize, D3D_BIND_FLAG Bind);
};
