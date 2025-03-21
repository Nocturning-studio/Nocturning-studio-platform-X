////////////////////////////////////////////////////////////////////////////////
// Created: 16.03.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#include "r_render_pipeline.h"
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
		//RenderDebug();
	}

	Device.Statistic->RenderCALC.End();
}
////////////////////////////////////////////////////////////////////////////////
