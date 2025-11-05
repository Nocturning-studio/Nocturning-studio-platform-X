#include "stdafx.h"
#include "ConstantBuffer.h"

#include "BufferUtils.h"
#include "dxRenderDeviceRender.h"

dx10ConstantBuffer::~dx10ConstantBuffer()
{
	DEV->_DeleteConstantBuffer(this);
//	Flush();
	_RELEASE(m_pBuffer);
	xr_free(m_pBufferData);
}

dx10ConstantBuffer::dx10ConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable)
	: m_bChanged(true)
{
	D3D_SHADER_BUFFER_DESC sbuffer_desc;

	CHK_DX(pTable->GetDesc(&sbuffer_desc));

	m_strBufferName._set(sbuffer_desc.Name);
	m_eBufferType = sbuffer_desc.Type;
	m_uiBufferSize = sbuffer_desc.Size;

	//	Fill member list with variable descriptions
	m_MembersList.resize(sbuffer_desc.Variables);
	m_MembersNames.resize(sbuffer_desc.Variables);
	for (u32 i=0; i< sbuffer_desc.Variables; ++i)
	{
		ID3DShaderReflectionVariable* pVar;
		ID3DShaderReflectionType*		pType;

		D3D_SHADER_VARIABLE_DESC		var_desc;

		pVar = pTable->GetVariableByIndex(i);
		VERIFY(pVar);
		pType = pVar->GetType();
		VERIFY(pType);
		pType->GetDesc(&m_MembersList[i]);
		//	Buffers with the same layout can contain totally different members
		CHK_DX(pVar->GetDesc(&var_desc));
		m_MembersNames[i] = var_desc.Name;
	}

	m_uiMembersCRC = crc32( &m_MembersList[0], sbuffer_desc.Variables*sizeof(m_MembersList[0]));

	D3D_BUFFER_DESC cbuffer_desc;
	cbuffer_desc.ByteWidth = sbuffer_desc.Size;
	cbuffer_desc.Usage = D3D_USAGE_DYNAMIC;
	cbuffer_desc.BindFlags = D3D_BIND_CONSTANT_BUFFER;
	cbuffer_desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
	cbuffer_desc.MiscFlags = 0;

	R_CHK(HW.pDevice->CreateBuffer(&cbuffer_desc, 0, &m_pBuffer));
	VERIFY(m_pBuffer);
	m_pBufferData = xr_malloc(cbuffer_desc.ByteWidth);
	VERIFY(m_pBufferData);
}

bool dx10ConstantBuffer::Similar(dx10ConstantBuffer &_in)
{
	if ( m_strBufferName._get() != _in.m_strBufferName._get() )
		return false;

	if ( m_eBufferType != _in.m_eBufferType )
		return false;

	if ( m_uiMembersCRC != _in.m_uiMembersCRC )
		return false;

	if ( m_MembersList.size() != _in.m_MembersList.size() )
		return false;

	if ( memcmp(&m_MembersList[0], &_in.m_MembersList[0], m_MembersList.size()*sizeof(m_MembersList[0])) )
		return false;

	VERIFY(m_MembersNames.size() == _in.m_MembersNames.size());

	int iMemberNum = m_MembersNames.size();
	for ( int i=0; i<iMemberNum; ++i)
	{
		if (m_MembersNames[i].c_str()!=_in.m_MembersNames[i].c_str())
			return false;
	}

	return true;
}

void dx10ConstantBuffer::Flush()
{
	if (m_bChanged)
	{
		void	*pData;
		D3D_MAPPED_SUBRESOURCE	pSubRes;
		CHK_DX(HW.pContext->Map(m_pBuffer, 0, D3D_MAP_WRITE_DISCARD, 0, &pSubRes));
		pData = pSubRes.pData;

		VERIFY(pData);
		VERIFY(m_pBufferData);
		CopyMemory(pData, m_pBufferData, m_uiBufferSize);

		HW.pContext->Unmap(m_pBuffer, 0);

		m_bChanged = false;
	}
}