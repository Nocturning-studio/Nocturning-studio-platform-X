#include "stdafx.h"

void CRenderTarget::phase_wallmarks		()
{
	// Targets
	if (RImplementation.o.aa_mode == AA_MSAA)
	{
		u_setrt(rt_Color_ms);
		u_setzb(rt_MSAA_depth);
	}
	else
	{
		u_setrt(rt_Color);
		if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
		else						u_setzb(HW.pBaseDepthReadWriteDSV);
	}

	// Stencil	- draw only where stencil >= 0x1
	RCache.set_Stencil					(TRUE, D3D_COMPARISON_LESS_EQUAL,0x01,0xff,0x00);
	RCache.set_CullMode					(D3D_CULL_BACK);
	RCache.set_ColorWriteEnable			(D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE);
}
