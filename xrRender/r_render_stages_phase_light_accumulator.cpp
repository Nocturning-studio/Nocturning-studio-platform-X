#include "stdafx.h"
#include "r_rendertarget.h"

void CRenderTarget::phase_accumulator()
{
	OPTICK_EVENT("CRenderTarget::phase_accumulator");

	// Targets
	if (dwAccumulatorClearMark == Device.dwFrame)
	{
		set_Render_Target_Surface(rt_Light_Accumulator, rt_Shadow_Accumulator, NULL, NULL);
		set_Depth_Buffer(HW.pBaseZB);
	}
	else
	{
		// initial setup
		dwAccumulatorClearMark = Device.dwFrame;

		// clear
		set_Render_Target_Surface(rt_Light_Accumulator, rt_Shadow_Accumulator, NULL, NULL);
		set_Depth_Buffer(HW.pBaseZB);
		dwLightMarkerID = 5;					// start from 5, increment in 2 units
		u32 clr4clear = color_rgba(0, 0, 0, 0); // 0x00
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, clr4clear, 1.0f, 0L));
	}
}

