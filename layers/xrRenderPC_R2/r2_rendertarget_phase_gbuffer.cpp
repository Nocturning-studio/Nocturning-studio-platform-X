////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modified: 13.10.2023
// Modifier: Deathman
// Nocturning studio for NS Project X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::enable_anisotropy_filtering()
{
	for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, ps_r__tf_Anisotropic));
}

void CRenderTarget::disable_anisotropy_filtering()
{
	for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
		CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1));
}

void CRenderTarget::clear_gbuffer()
{
	u_setrt(rt_GBuffer_Position, rt_GBuffer_Normal, rt_GBuffer_Albedo, HW.pBaseZB);
	CHK_DX(HW.pDevice->Clear(0L, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x0, 1.0f, 0L));
}

void CRenderTarget::create_gbuffer()
{
	enable_anisotropy_filtering();

	u_setrt(rt_GBuffer_Position, rt_GBuffer_Normal, rt_GBuffer_Albedo, HW.pBaseZB);

	// Stencil - write 0x1 at pixel pos
	RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
					   D3DSTENCILOP_KEEP);

	// Misc	- draw only front-faces
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE));
	RCache.set_CullMode(CULL_CCW);
	RCache.set_ColorWriteEnable();
}
