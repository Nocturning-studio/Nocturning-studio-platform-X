#include "stdafx.h"

void	CRenderTarget::phase_smap_direct		(light* L)
{

	if (RImplementation.o.vsm)
	{
		u_setrt(RImplementation.o.vsm ? rt_vsm_depth : NULL);
		RCache.clear_CurrentRenderTargetView(rgba_black);
	}
	else
		u_setrt(NULL);

	u_setzb(rt_smap_depth);

	RCache.clear_CurrentDepthView();

	//	Prepare viewport for shadow map rendering
	RImplementation.rmNormal();

	// Stencil	- disable
	RCache.set_Stencil ( FALSE );

}

void	CRenderTarget::phase_smap_rain		(light* L)
{
	phase_smap_direct(L);

	D3D_VIEWPORT VP =
	{
		(float)L->X.D.minX,(float)L->X.D.minY,
		(float)(L->X.D.maxX - L->X.D.minX) ,
		(float)(L->X.D.maxY - L->X.D.minY) ,
		0,1
	};

	RCache.set_Viewport(&VP);
}

void	CRenderTarget::phase_smap_direct_tsh	(light* L)
{
	PIX_EVENT(accum_transparent_smap);
	VERIFY								(RImplementation.o.tshadows);
	//u32		_clr						= 0xffffffff;	//color_rgba(127,127,12,12);
	FLOAT ColorRGBA[4] = { 1.0f, 1.0f, 1.0f, 1.0f};
	RCache.set_ColorWriteEnable			();
	//	Prepare viewport for shadow map rendering
	RImplementation.rmNormal();
	RCache.clear_RenderTargetView(RCache.get_RT(0), ColorRGBA);
	//CHK_DX								(HW.pDevice->Clear( 0L, NULL, D3DCLEAR_TARGET,	_clr,	1.0f, 0L));

}
