////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modified: 13.10.2023
// Modifier: Deathman, Mihan-323
// Nocturning studio for NS Project X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::clear_gbuffer()
{
	OPTICK_EVENT("CRenderTarget::clear_gbuffer");

	if (ps_r_shading_flags.test(RFLAG_FLAT_SHADING))
		u_setrt(rt_GBuffer_1, rt_GBuffer_2, rt_GBuffer_3, NULL, HW.pBaseZB);
	else
		u_setrt(rt_GBuffer_1, rt_GBuffer_2, rt_GBuffer_3, rt_GBuffer_4, HW.pBaseZB);

	CHK_DX(HW.pDevice->Clear(0L, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x0, 1.0f, 0L));
}

void CRenderTarget::create_gbuffer()
{
	OPTICK_EVENT("CRenderTarget::create_gbuffer");

	if (ps_r_shading_flags.test(RFLAG_FLAT_SHADING))
		u_setrt(rt_GBuffer_1, rt_GBuffer_2, rt_GBuffer_3, NULL, HW.pBaseZB);
	else
		u_setrt(rt_GBuffer_1, rt_GBuffer_2, rt_GBuffer_3, rt_GBuffer_4, HW.pBaseZB);

	// Stencil - write 0x1 at pixel pos
	RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
					   D3DSTENCILOP_KEEP);

	// Misc	- draw only front-faces
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE));

	//Set backface culling
	RCache.set_CullMode(CULL_CCW);

	RCache.set_ColorWriteEnable();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
