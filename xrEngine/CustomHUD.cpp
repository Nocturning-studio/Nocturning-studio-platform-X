#include "stdafx.h"
#include "CustomHUD.h"

Flags32 psHUD_Flags = {HUD_CROSSHAIR_RT | HUD_WEAPON_RT | HUD_CROSSHAIR_DYNAMIC | HUD_CROSSHAIR_RT2 | HUD_DRAW_RT |
					   HUD_DRAW_MINIMAP | HUD_DRAW_HEALTH_BAR | HUD_DRAW_MOTION_BAR};

ENGINE_API CCustomHUD* g_hud = NULL;

CCustomHUD::CCustomHUD()
{
	g_hud = this;
}

CCustomHUD::~CCustomHUD()
{
	g_hud = NULL;
}
