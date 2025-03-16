#include "stdafx.h"

void CRenderTarget::phase_smap_direct(light* L, u32 sub_phase)
{
	// Targets
	set_Render_Target_Surface(rt_smap_surf);
	set_Depth_Buffer(rt_smap_depth->pRT);

	// optimized clear
	D3DRECT R;
	R.x1 = L->X.D.minX;
	R.x2 = L->X.D.maxX;
	R.y1 = L->X.D.minY;
	R.y2 = L->X.D.maxY;
	CHK_DX(HW.pDevice->Clear(1L, &R, D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0L));

	// Stencil - disable
	RenderBackend.set_Stencil(FALSE);

	// Misc	- draw only front/back-faces
	//if (SE_SUN_NEAR == sub_phase || SE_SUN_MIDDLE == sub_phase)
		RenderBackend.set_CullMode(CULL_CCW);
	//else
	//	RenderBackend.set_CullMode(CULL_CW);

	RenderBackend.set_ColorWriteEnable(FALSE);
}

void CRenderTarget::phase_smap_direct_tsh(light* L, u32 sub_phase)
{
	VERIFY(RenderImplementation.o.Tshadows);
	u32 _clr = 0xffffffff; // color_rgba(127,127,12,12);
	RenderBackend.set_ColorWriteEnable();
	CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, _clr, 1.0f, 0L));
}
