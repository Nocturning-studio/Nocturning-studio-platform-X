////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\fbasicvisual.h"
#include "..\xrEngine\customhud.h"
#include "..\xrEngine\xr_object.h"
////////////////////////////////////////////////////////////////////////////////
void CRender::Render()
{
	OPTICK_EVENT("CRender::Render");

	Device.Statistic->RenderCALC.Begin();

	bool b_need_render_menu = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	if (b_need_render_menu)
	{
		RenderMenu();
	}
	else
	{
		if (!(g_pGameLevel && g_pGameLevel->pHUD))
			return;

		RenderScene();
	}

	Device.Statistic->RenderCALC.End();
}
////////////////////////////////////////////////////////////////////////////////
