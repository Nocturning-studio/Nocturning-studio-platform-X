#include "stdafx.h"

// startup
void	CRenderTarget::phase_scene_prepare	()
{
	PIX_EVENT(phase_scene_prepare);

	if (RImplementation.o.ssaa && RImplementation.o.aa_mode != AA_MSAA)
	{
		RCache.clear_RenderTargetView(rt_Position->pRT, rgba_black);
		RCache.clear_RenderTargetView(rt_SSAA_color->pRT, rgba_black);
		RCache.clear_DepthStencilView(rt_SSAA_depth->pZRT);
	}
	else if (RImplementation.o.aa_mode == AA_MSAA)
	{
		RCache.clear_RenderTargetView(rt_Position->pRT, rgba_black);
		RCache.clear_RenderTargetView(rt_Color_ms->pRT, rgba_black);
		RCache.clear_DepthStencilView(rt_MSAA_depth->pZRT);
	}
	else
	{
		RCache.clear_RenderTargetView(rt_Position->pRT, rgba_black);
		RCache.clear_RenderTargetView(rt_Color->pRT, rgba_black);
		//RCache.clear_RenderTargetView(rt_Motion->pRT, rgba_black);
		RCache.clear_DepthStencilView(HW.pBaseDepthReadWriteDSV);
	}

	if (RImplementation.o.volumetriclight || RImplementation.o.sunshafts)
		RCache.clear_RenderTargetView(rt_Generic_2->pRT, rgba_black);

	if (RImplementation.o.sun_il || RImplementation.o.spot_il)
		RCache.clear_RenderTargetView(rt_RSM->pRT, rgba_black);
}

// begin
void	CRenderTarget::phase_scene_begin	()
{
	/*if (RImplementation.o.txaa || RImplementation.o.aa_mode == AA_TAA)
	{
		if (RImplementation.o.aa_mode == AA_MSAA)
		{
			u_setrt(rt_Position, rt_Color_ms, rt_Motion_ms);
			u_setzb(rt_MSAA_depth);
		}
		else
		{
			u_setrt(rt_Position, rt_Color, rt_Motion);
			u_setzb(depth);
		}
	}
	else*/
	{
		if (RImplementation.o.aa_mode == AA_MSAA)
		{
			u_setrt(rt_Position, rt_Color_ms);
			u_setzb(rt_MSAA_depth);
		}
		else
		{
			u_setrt(rt_Position, rt_Color);
			u_setzb(rt_SSAA_depth);
			if (RImplementation.o.ssaa)	u_setzb(rt_SSAA_depth);
			else						u_setzb(HW.pBaseDepthReadWriteDSV);
			enable_SSAA();
		}
	}

	// Stencil - write 0x1 at pixel pos
	RCache.set_Stencil					( TRUE, D3D_COMPARISON_ALWAYS,0x01,0xff,0x7f, D3D_STENCIL_OP_KEEP, D3D_STENCIL_OP_REPLACE, D3D_STENCIL_OP_KEEP);

	// Misc		- draw only front-faces
	//	TODO: DX10: siable two-sided stencil here
	//CHK_DX(HW.pDevice->SetRenderState	( D3DRS_TWOSIDEDSTENCILMODE,FALSE				));
	RCache.set_CullMode					(D3D_CULL_BACK);
	RCache.set_ColorWriteEnable			( );


	SSManager.SetMaxAnisotropy(r__tf_aniso);

}

// end
void	CRenderTarget::phase_scene_end		()
{

	SSManager.SetMaxAnisotropy(1);

}