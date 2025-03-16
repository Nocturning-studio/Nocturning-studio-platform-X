#include "stdafx.h"

void CRenderTarget::phase_occq()
{
	OPTICK_EVENT("CRenderTarget::phase_occq");

	set_Render_Target_Surface(Device.dwWidth, Device.dwHeight, HW.pBaseRT);
	set_Depth_Buffer(HW.pBaseZB);
	RenderBackend.set_CullMode(CULL_CCW);
	RenderBackend.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RenderBackend.set_ColorWriteEnable(FALSE);
	RenderBackend.set_Shader(s_occq);
}
