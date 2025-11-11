#include "stdafx.h"
#pragma hdrstop

#include "sh_atomic.h"
#include "ResourceManager.h"

// Atomic
// SVS::~SVS								()			{	_RELEASE(vs);		dxRenderDeviceRender::Instance().Resources->_DeleteVS			(this);
// } SPS::~SPS								()			{	_RELEASE(ps);
// dxRenderDeviceRender::Instance().Resources->_DeletePS			(this);	}
// SState::~SState							()			{	_RELEASE(state);	dxRenderDeviceRender::Instance().Resources->_DeleteState		(this);
// } SDeclaration::~SDeclaration				()			{	_RELEASE(dcl);
// dxRenderDeviceRender::Instance().Resources->_DeleteDecl		(this);	}

///////////////////////////////////////////////////////////////////////
//	SVS
SVS::SVS() : sh(0)
//	,signature(0)
{
	;
}

SVS::~SVS()
{
	//_RELEASE(signature);
	//	Now it is release automatically
	_RELEASE(sh);
	Device.Resources->DestroyShader(this);
}

///////////////////////////////////////////////////////////////////////
//	SPS
SPS::~SPS()
{
	_RELEASE(sh);
	Device.Resources->DestroyShader(this);
}

///////////////////////////////////////////////////////////////////////
//	SGS
SGS::~SGS()
{
	_RELEASE(sh);
	//Device.Resources->_DeleteGS(this);
	Device.Resources->DestroyShader(this);
}

SHS::~SHS()
{
	_RELEASE(sh);
	//Device.Resources->_DeleteHS(this);
	Device.Resources->DestroyShader(this);
}
SDS::~SDS()
{
	_RELEASE(sh);
	//Device.Resources->_DeleteDS(this);
	Device.Resources->DestroyShader(this);
}
SCS::~SCS()
{
	_RELEASE(sh);
	//Device.Resources->_DeleteCS(this);
	Device.Resources->DestroyShader(this);
}

///////////////////////////////////////////////////////////////////////
//	SInputSignature
SInputSignature::SInputSignature(ID3DBlob* pBlob)
{
	VERIFY(pBlob);
	signature = pBlob;
	signature->AddRef();
};
SInputSignature::~SInputSignature()
{
	_RELEASE(signature);
	Device.Resources->_DeleteInputSignature(this);
}

///////////////////////////////////////////////////////////////////////
//	SState
SState::~SState()
{
	_RELEASE(state);
	Device.Resources->_DeleteState(this);
}

///////////////////////////////////////////////////////////////////////
//	SDeclaration
SDeclaration::~SDeclaration()
{
	Device.Resources->_DeleteDecl(this);
	xr_map<ID3DBlob*, ID3D11InputLayout*>::iterator iLayout;
	iLayout = vs_to_layout.begin();
	for (; iLayout != vs_to_layout.end(); ++iLayout)
	{
		//	Release vertex layout
		_RELEASE(iLayout->second);
	}
}
