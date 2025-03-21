#include "stdafx.h"

void CRender::set_light_accumulator()
{
	OPTICK_EVENT("CRenderTarget::set_light_accumulator");

	if (dwAccumulatorClearMark == Device.dwFrame)
	{
		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Light_Accumulator,
												RenderTarget->rt_Shadow_Accumulator);

		RenderBackend.set_Depth_Buffer(HW.pBaseZB);
	}
	else
	{
		dwAccumulatorClearMark = Device.dwFrame;

		RenderBackend.set_Render_Target_Surface(RenderTarget->rt_Light_Accumulator,
												RenderTarget->rt_Shadow_Accumulator);

		RenderBackend.set_Depth_Buffer(HW.pBaseZB);
		dwLightMarkerID = 5; // start from 5, increment in 2 units
		CHK_DX(HW.pDevice->Clear(0L, NULL, D3DCLEAR_TARGET, color_rgba(0, 0, 0, 0), 1.0f, 0L));
	}
}

