#include "StdAfx.h"
#include "UIStatsIcon.h"
#include "UITextureMaster.h"
#include "UIInventoryUtilities.h"

CUIStatsIcon::TEX_INFO CUIStatsIcon::m_tex_info[MAX_DEF_TEX][2];

CUIStatsIcon::CUIStatsIcon()
{
	SetStretchTexture(true);
	InitTexInfo();
	m_bAvailableTexture = true;
}

using namespace InventoryUtilities;

void CUIStatsIcon::InitTexInfo()
{
	if (m_tex_info[RANK_0][0].sh)
		return;
	// ranks
	string128 rank_tex;
	for (int i = RANK_0; i <= RANK_4; i++)
	{
		sprintf_s(rank_tex, "ui_hud_status_green_0%d", i + 1);
		m_tex_info[i][0].sh.create("hud\\default", CUITextureMaster::GetTextureFileName(rank_tex));
		m_tex_info[i][0].rect = CUITextureMaster::GetTextureRect(rank_tex);

		sprintf_s(rank_tex, "ui_hud_status_blue_0%d", i + 1);
		m_tex_info[i][1].sh.create("hud\\default", CUITextureMaster::GetTextureFileName(rank_tex));
		m_tex_info[i][1].rect = CUITextureMaster::GetTextureRect(rank_tex);
	}

	// artefact
	LPCSTR artefact_name = pSettings->r_string("artefacthunt_gamedata", "artefact");
	float fGridWidth = pSettings->r_float(artefact_name, "inv_grid_width");
	float fGridHeight = pSettings->r_float(artefact_name, "inv_grid_height");
	float fXPos = pSettings->r_float(artefact_name, "inv_grid_x");
	float fYPos = pSettings->r_float(artefact_name, "inv_grid_y");

	m_tex_info[ARTEFACT][0].sh = GetEquipmentIconsShader();
	m_tex_info[ARTEFACT][0].rect.set(fXPos * INV_GRID_WIDTH, fYPos * INV_GRID_HEIGHT,
									 fXPos * INV_GRID_WIDTH + fGridWidth * INV_GRID_WIDTH,
									 fYPos * INV_GRID_HEIGHT + fGridHeight * INV_GRID_HEIGHT);

	//	m_tex_info[ARTEFACT][0].rect.set( 200, 400, 50, 50);

	// death
	m_tex_info[DEATH][0].sh.create("hud\\default", "ui\\ui_mp_icon_kill");
	m_tex_info[DEATH][0].rect.x1 = 32;
	m_tex_info[DEATH][0].rect.y1 = 202;
	m_tex_info[DEATH][0].rect.x2 = m_tex_info[DEATH][0].rect.x1 + 30;
	m_tex_info[DEATH][0].rect.y2 = m_tex_info[DEATH][0].rect.y1 + 30;
}

void CUIStatsIcon::FreeTexInfo()
{
	// ranks
	for (int i = RANK_0; i <= RANK_4; i++)
	{
		m_tex_info[i][0].sh.destroy();
		m_tex_info[i][1].sh.destroy();
	}
	m_tex_info[ARTEFACT][0].sh.destroy();
	m_tex_info[DEATH][0].sh.destroy();
}

void CUIStatsIcon::SetText(LPCSTR str)
{
	if (0 == str[0])
	{
		SetVisible(false);
		return;
	}
	else
		SetVisible(true);

	if (strstr(str, "status"))
	{
		int team = 1;
		if (strstr(str, "green"))
			team = 0;

		int rank = atoi(strstr(str, "0")) - 1;

		GetStaticItem()->SetShader(m_tex_info[rank][team].sh);
		SetOriginalRect(m_tex_info[rank][team].rect);
	}
	else if (0 == xr_strcmp(str, "death"))
	{
		GetStaticItem()->SetShader(m_tex_info[DEATH][0].sh);
		SetOriginalRect(m_tex_info[DEATH][0].rect);
	}
	else if (0 == xr_strcmp(str, "artefact"))
	{
		GetStaticItem()->SetShader(m_tex_info[ARTEFACT][0].sh);
		SetOriginalRect(m_tex_info[ARTEFACT][0].rect);
	}
	else
	{
		InitTexture(str);
	}
}
