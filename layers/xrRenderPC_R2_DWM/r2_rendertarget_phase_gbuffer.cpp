////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modified: 13.10.2023
// Modifier: Deathman, Mihan-323
// Nocturning studio for NS Project X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRenderTarget::enable_anisotropy_filtering()
{
#pragma message(Reminder("enable_aniso"))
	//for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
	//	CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, ps_r_tf_Anisotropic));
}

void CRenderTarget::disable_anisotropy_filtering()
{
#pragma message(Reminder("Fix disable_aniso"))
	//for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
	//	CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1));
}

void CRenderTarget::clear_gbuffer()
{
	// TODO: FIX ME
	// может вызвать очень неприятную ошибку!!!
	//u_setrt(rt_GBuffer_Albedo, rt_GBuffer_Normal, rt_GBuffer_Position, rt_ZB->pRT);
	//if (RImplementation.o.gbuffer_opt_mode <= 1)
	//	u_setrt(rt_GBuffer_Albedo, rt_GBuffer_Normal, rt_GBuffer_Position, rt_ZB->pRT);
	//else
	//	u_setrt(rt_GBuffer_Albedo, rt_GBuffer_Normal, 0, rt_ZB->pRT);
	//CHK_DX(HW.pDevice->Clear(0L, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x0, 1.0f, 0L));
	ClearZB(rt_ZB->pZRT, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL);
}

void CRenderTarget::create_gbuffer()
{
	if (RImplementation.o.gbuffer_opt_mode <= 1)
		// u_setrt(rt_GBuffer_Albedo, rt_GBuffer_Normal, rt_GBuffer_Position, rt_ZB->pRT);
		SetRT(rt_GBuffer_Albedo, rt_GBuffer_Normal, rt_GBuffer_Position, 0, rt_ZB->pZRT, false, false);
	else
		//u_setrt(rt_GBuffer_Albedo, rt_GBuffer_Normal, 0, rt_ZB->pRT);
		SetRT(rt_GBuffer_Albedo, rt_GBuffer_Normal, 0, 0, rt_ZB->pZRT, false, false);

	// Stencil - write 0x1 at pixel pos
	RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
					   D3DSTENCILOP_KEEP);

	// Misc	- draw only front-faces
	//CHK_DX(HW.pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE));

	//Set backface culling
	RCache.set_CullMode(CULL_CCW);

	RCache.set_ColorWriteEnable();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
