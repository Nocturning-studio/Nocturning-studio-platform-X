// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#pragma warning(disable:4995)
#include "..\xrEngine\stdafx.h"
#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)
#pragma warning(disable:4714)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4237)

#include "..\xrEngine\resourcemanager.h"
#include "..\xrEngine\vis_common.h"
#include "..\xrEngine\render.h"
#include "..\xrEngine\_d3d_extensions.h"
#include "..\xrEngine\igame_level.h"
#include "..\xrEngine\blenders\blender.h"
#include "..\xrEngine\blenders\blender_clsid.h"
#include "..\xrEngine\psystem.h"
#include "xrRender_console.h"
#include "render.h"
#include "r_color_converting.h"

IC	void	jitter(CBlender_Compile& C)
{
	//C.r_Sampler	("jitter0",	JITTER(0), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	//C.r_Sampler	("jitter1",	JITTER(1), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	//C.r_Sampler	("jitter2",	JITTER(2), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	//C.r_Sampler	("jitter3",	JITTER(3), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	C.r_Sampler("s_blue_noise", "noise\\blue_noise_texture", true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	C.r_Sampler("s_perlin_noise", "noise\\perlin_noise_texture", true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
}

IC	void	gbuffer(CBlender_Compile& C)
{
	C.r_Sampler_rtf("s_gbuffer_1", r_RT_GBuffer_1);
	C.r_Sampler_rtf("s_gbuffer_2", r_RT_GBuffer_2);
	C.r_Sampler_rtf("s_gbuffer_3", r_RT_GBuffer_3);
}
