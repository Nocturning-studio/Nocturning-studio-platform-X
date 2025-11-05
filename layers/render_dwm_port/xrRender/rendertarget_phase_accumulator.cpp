#include "stdafx.h"

void	CRenderTarget::phase_accumulator()
{
	if (RImplementation.o.cspecular)
		u_setrt(rt_Accumulator, rt_Accumulator_1);
	else
		u_setrt(rt_Accumulator);

	if (dwAccumulatorClearMark != Device.dwFrame)
	{
		dwAccumulatorClearMark = Device.dwFrame;

		reset_light_marker();

		if (RImplementation.o.cspecular)
			RCache.clear_RenderTargetView(rt_Accumulator_1->pRT, rgba_black);

		RCache.clear_RenderTargetView(rt_Accumulator->pRT, rgba_black);
		RCache.set_Stencil(TRUE, D3D_COMPARISON_LESS_EQUAL, 0x01, 0xff, 0x00);
		RCache.set_CullMode(D3D_CULL_NONE);
		RCache.set_ColorWriteEnable();
	}

	if(RImplementation.o.aa_mode == AA_MSAA)	u_setzb(rt_MSAA_depth);
	else if (RImplementation.o.ssaa)			u_setzb(rt_SSAA_depth);
	else										u_setzb(HW.pBaseDepthReadWriteDSV);

	//	Restore viewport after shadow map rendering
	RImplementation.Target->enable_SSAA();
	RImplementation.rmNormal();
}

void	CRenderTarget::phase_vol_accumulator()
{
	u_setrt(rt_Generic_2);


	if(RImplementation.o.aa_mode == AA_MSAA)	u_setzb(rt_MSAA_depth);
	else if (RImplementation.o.ssaa)			u_setzb(rt_SSAA_depth);
	else										u_setzb(HW.pBaseDepthReadWriteDSV);

	RCache.set_Stencil(FALSE);
	RCache.set_CullMode(D3D_CULL_NONE);
	RCache.set_ColorWriteEnable();
}

void	CRenderTarget::phase_rsm_accumulator()
{
	u_setrt(rt_RSM);
	if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
	else						u_setzb(HW.pBaseDepthReadWriteDSV);

	//RCache.clear_CurrentRenderTargetView(rgba_black);

	RCache.set_Stencil(FALSE);
	RCache.set_CullMode(D3D_CULL_NONE);
	RCache.set_ColorWriteEnable();
}