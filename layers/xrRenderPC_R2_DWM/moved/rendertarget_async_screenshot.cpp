#include "stdafx.h"

void CRenderTarget::DoAsyncScreenshot()
{
	//	Igor: screenshot will not have postprocess applied.
	//	TODO: fox that later
	if (RImplementation.m_bMakeAsyncSS)
	{
		HRESULT hr;


		ID3DTexture2D*	pBuffer;
		hr = HW.m_pSwapChain->GetBuffer( 0, __uuidof( ID3DTexture2D ), (LPVOID*)&pBuffer );
		HW.pContext->CopyResource( t_ss_async, pBuffer );


		RImplementation.m_bMakeAsyncSS = false;
	}
}
