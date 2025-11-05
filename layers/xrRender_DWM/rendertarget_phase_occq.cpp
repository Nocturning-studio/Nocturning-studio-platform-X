#include "stdafx.h"

void	CRenderTarget::phase_occq	()
{
	u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT);
	u_setzb(HW.pBaseDepthReadWriteDSV);

	RCache.set_Shader			( s_occq	);
	RCache.set_CullMode			(D3D_CULL_BACK);
	RCache.set_Stencil			(TRUE, D3D_COMPARISON_LESS_EQUAL,0x01,0xff,0x00);
	RCache.set_ColorWriteEnable	(FALSE		);
}
