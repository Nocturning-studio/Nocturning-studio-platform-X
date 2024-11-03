#include "stdafx.h"
#include "r2_rendertarget.h"

void CRenderTarget::phase_accumulator()
{
	// Targets
	if (dwAccumulatorClearMark == Device.dwFrame)
	{
		u_setrt(rt_Light_Accumulator, NULL, NULL, rt_ZB->pRT);
	}
	else
	{
		// initial setup
		dwAccumulatorClearMark = Device.dwFrame;

		// clear
		u_setrt(rt_Light_Accumulator, NULL, NULL, rt_ZB->pRT);
		dwLightMarkerID = 5;					// start from 5, increment in 2 units
		u32 clr4clear = color_rgba(0, 0, 0, 0); // 0x00
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, clr4clear, 1.0f, 0L));
	}
}

void CRenderTarget::phase_vol_accumulator()
{
	if (!GetActiveVolumetric())
	{
		SetActiveVolumetric(true);

		u_setrt(rt_Generic_2, NULL, NULL, rt_ZB->pRT);
		u32 clr4clearVol = color_rgba(0, 0, 0, 0); // 0x00
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, clr4clearVol, 1.0f, 0L));
	}
	else
	{
		u_setrt(rt_Generic_2, NULL, NULL, rt_ZB->pRT);
	}

	RCache.set_Stencil(FALSE);
	RCache.set_CullMode(CULL_NONE);
	RCache.set_ColorWriteEnable();
}
