#include "stdafx.h"
#pragma hdrstop

#include "sh_atomic.h"
#include "ResourceManager.h"

// Atomic
SVS::~SVS()
{
	_RELEASE(sh);
	Device.Resources->DestroyShader<SVS>(this);
}
SPS::~SPS()
{
	_RELEASE(sh);
	Device.Resources->DestroyShader<SPS>(this);
}
SState::~SState()
{
	_RELEASE(state);
	Device.Resources->_DeleteState(this);
}
SDeclaration::~SDeclaration()
{
	_RELEASE(dcl);
	Device.Resources->_DeleteDecl(this);
}
