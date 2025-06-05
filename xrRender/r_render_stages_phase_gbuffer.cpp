////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Modified: 13.10.2023
// Modifier: NSDeathman, Mihan-323
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRender::clear_gbuffer()
{
	OPTICK_EVENT("CRenderTarget::clear_gbuffer");

	if (!ps_r_shading_flags.test(RFLAG_ENABLE_PBR))
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_GBuffer_1, 
												RenderTarget->rt_GBuffer_2, 
												RenderTarget->rt_GBuffer_3);
	else
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_GBuffer_1, 
												RenderTarget->rt_GBuffer_2, 
												RenderTarget->rt_GBuffer_3, 
												RenderTarget->rt_GBuffer_4);

	RenderBackend.set_Depth_Buffer(HW.pBaseZB);

	CHK_DX(HW.pDevice->Clear(0L, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x0, 1.0f, 0L));
}

void CRender::set_gbuffer()
{
	OPTICK_EVENT("CRenderTarget::set_gbuffer");

	if (!ps_r_shading_flags.test(RFLAG_ENABLE_PBR))
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_GBuffer_1, 
												RenderTarget->rt_GBuffer_2, 
												RenderTarget->rt_GBuffer_3);
	else
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_GBuffer_1, 
												RenderTarget->rt_GBuffer_2, 
												RenderTarget->rt_GBuffer_3, 
												RenderTarget->rt_GBuffer_4);

	RenderBackend.set_Depth_Buffer(HW.pBaseZB);

	// Stencil - write 0x1 at pixel pos
	RenderBackend.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);

	// Misc	- draw only front-faces
	CHK_DX(HW.pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE));

	//Set backface culling
	RenderBackend.set_CullMode(CULL_CCW);

	RenderBackend.set_ColorWriteEnable();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
