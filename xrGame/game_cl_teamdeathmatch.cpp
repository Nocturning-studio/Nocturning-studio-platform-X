#include "stdafx.h"
#include "game_cl_teamdeathmatch.h"
#include "xrMessages.h"
#include "hudmanager.h"
#include "level.h"
#include "UIGameTDM.h"
#include "xr_level_controller.h"
#include "clsid_game.h"
#include "map_manager.h"
#include "map_location.h"
#include "actor.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UISkinSelector.h"
#include "ui/UIPDAWnd.h"
#include "ui/UIMapDesc.h"
#include "game_base_menu_events.h"
#include "ui/TeamInfo.h"
#include "string_table.h"

//.#define	TEAM0_MENU		"teamdeathmatch_team0"
//.#define	TEAM1_MENU		"teamdeathmatch_team1"
//.#define	TEAM2_MENU		"teamdeathmatch_team2"

#define TDM_MESSAGE_MENUS "tdm_messages_menu"

#include "game_cl_teamdeathmatch_snd_messages.h"

const shared_str game_cl_TeamDeathmatch::GetTeamMenu(s16 team)
{
	switch (team)
	{
	case 0:
		return "teamdeathmatch_team0";
		break;
	case 1:
		return "teamdeathmatch_team1";
		break;
	case 2:
		return "teamdeathmatch_team2";
		break;
	default:
		NODEFAULT;
	};
#ifdef DEBUG
	return NULL;
#endif // DEBUG
}

game_cl_TeamDeathmatch::game_cl_TeamDeathmatch()
{
	PresetItemsTeam1.clear();
	PresetItemsTeam2.clear();

	m_bTeamSelected = FALSE;
	m_game_ui = NULL;

	m_bShowPlayersNames = false;
	m_bFriendlyIndicators = false;
	m_bFriendlyNames = false;

	LoadSndMessages();
}
void game_cl_TeamDeathmatch::Init()
{
	//	pInventoryMenu	= xr_new<CUIInventoryWnd>();
	//	pPdaMenu = xr_new<CUIPdaWnd>();
	//	pMapDesc = xr_new<CUIMapDesc>();
	//-----------------------------------------------------------
	LoadTeamData(GetTeamMenu(1));
	LoadTeamData(GetTeamMenu(2));
}

game_cl_TeamDeathmatch::~game_cl_TeamDeathmatch()
{
	PresetItemsTeam1.clear();
	PresetItemsTeam2.clear();

	xr_delete(pCurBuyMenu);
	xr_delete(pCurSkinMenu);

	//	xr_delete(pInventoryMenu);
}

void game_cl_TeamDeathmatch::net_import_state(NET_Packet& P)
{
	bool teamsEqual = (!teams.empty()) ? (teams[0].score == teams[1].score) : false;
	inherited::net_import_state(P);
	m_bFriendlyIndicators = !!P.r_u8();
	m_bFriendlyNames = !!P.r_u8();
	if (!teams.empty())
	{
		if (teamsEqual)
		{
			if (teams[0].score != teams[1].score)
			{
				if (Level().CurrentViewEntity())
				{
					if (teams[0].score > teams[1].score)
						PlaySndMessage(ID_TEAM1_LEAD);
					else
						PlaySndMessage(ID_TEAM2_LEAD);
				}
			}
		}
		else
		{
			if (teams[0].score == teams[1].score)
				if (Level().CurrentViewEntity())
					PlaySndMessage(ID_TEAMS_EQUAL);
		}
	};
}
void game_cl_TeamDeathmatch::TranslateGameMessage(u32 msg, NET_Packet& P)
{
	CStringTable st;
	string512 Text;
	//	LPSTR	Color_Teams[3]	= {"%c[255,255,255,255]", "%c[255,64,255,64]", "%c[255,64,64,255]"};
	char Color_Main[] = "%c[255,192,192,192]";
	//	LPSTR	TeamsNames[3]	= {"Zero Team", "Team Green", "Team Blue"};

	switch (msg)
	{
	case GAME_EVENT_PLAYER_JOIN_TEAM: // tdm
	{
		string64 PlayerName;
		P.r_stringZ(PlayerName);
		u16 Team;
		P.r_u16(Team);

		sprintf_s(Text, "%s%s %s%s %s%s",
				  "", // no color
				  PlayerName, Color_Main, *st.translate("mp_joined"), CTeamInfo::GetTeam_color_tag(int(Team)),
				  CTeamInfo::GetTeam_name(int(Team)));
		CommonMessageOut(Text);
		//---------------------------------------
		Msg("%s %s %s", PlayerName, *st.translate("mp_joined"), CTeamInfo::GetTeam_name(int(Team)));
	}
	break;

	case PLAYER_CHANGE_TEAM: // tdm
	{
		u16 PlayerID, OldTeam, NewTeam;
		P.r_u16(PlayerID);
		P.r_u16(OldTeam);
		P.r_u16(NewTeam);

		game_PlayerState* pPlayer = GetPlayerByGameID(PlayerID);
		if (!pPlayer)
			break;

		sprintf_s(Text, "%s%s %s%s %s%s", CTeamInfo::GetTeam_color_tag(int(OldTeam)), pPlayer->name, Color_Main,
				  *st.translate("mp_switched_to"), CTeamInfo::GetTeam_color_tag(int(NewTeam)),
				  CTeamInfo::GetTeam_name(int(NewTeam)));
		CommonMessageOut(Text);
		//---------------------------------------
		Msg("%s *s %s", pPlayer->name, *st.translate("mp_switched_to"), CTeamInfo::GetTeam_name(int(NewTeam)));
	}
	break;

	default:
		inherited::TranslateGameMessage(msg, P);
	};
}

CUIGameCustom* game_cl_TeamDeathmatch::createGameUI()
{
	game_cl_mp::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_TEAMDEATHMATCH;
	m_game_ui = smart_cast<CUIGameTDM*>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();

	//-----------------------------------------------------------
	//	pInventoryMenu = xr_new<CUIInventoryWnd>();
	//-----------------------------------------------------------
	//	pPdaMenu = xr_new<CUIPdaWnd>();
	//-----------------------------------------------------------
	//	pMapDesc = xr_new<CUIMapDesc>();
	//-----------------------------------------------------------
	LoadMessagesMenu(TDM_MESSAGE_MENUS);
	//-----------------------------------------------------------
	return m_game_ui;
}

void game_cl_TeamDeathmatch::GetMapEntities(xr_vector<SZoneMapEntityData>& dst)
{
	SZoneMapEntityData D;
	u32 color_self_team = 0xff00ff00;
	D.color = color_self_team;

	s16 local_team = local_player->team;

	PLAYERS_MAP_IT it = players.begin();
	for (; it != players.end(); ++it)
	{
		if (local_team == it->second->team)
		{
			u16 id = it->second->GameID;
			CObject* pObject = Level().Objects.net_Find(id);
			if (!pObject)
				continue;
			if (!pObject || pObject->CLS_ID != CLSID_OBJECT_ACTOR)
				continue;

			VERIFY(pObject);
			D.pos = pObject->Position();
			dst.push_back(D);
		}
	}
}

void game_cl_TeamDeathmatch::OnMapInfoAccept()
{
	if (CanCallTeamSelectMenu())
		StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
};

void game_cl_TeamDeathmatch::OnTeamMenuBack()
{
	if (local_player->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
	{
		StartStopMenu(m_game_ui->m_pMapDesc, true);
		//        StartStopMenu(pMapDesc, true);
	}
};

void game_cl_TeamDeathmatch::OnTeamMenu_Cancel()
{
	StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
	if (!m_bTeamSelected && !m_bSpectatorSelected)
	{
		if (CanCallTeamSelectMenu() && !m_game_ui->m_pUITeamSelectWnd->IsShown())
		{
			StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
			return;
		}
	}
	m_bMenuCalledFromReady = FALSE;
};

void game_cl_TeamDeathmatch::OnSkinMenuBack()
{
	if (CanCallTeamSelectMenu())
		StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
};

void game_cl_TeamDeathmatch::OnSpectatorSelect()
{
	m_bTeamSelected = FALSE;
	inherited::OnSpectatorSelect();
}

void game_cl_TeamDeathmatch::OnTeamSelect(int Team)
{
	bool NeedToSendTeamSelect = true;
	if (Team != -1)
	{
		if (Team + 1 == local_player->team && m_bSkinSelected)
			NeedToSendTeamSelect = false;
		else
		{
			NeedToSendTeamSelect = true;
		}
	}

	if (NeedToSendTeamSelect)
	{
		CObject* l_pObj = Level().CurrentEntity();

		CGameObject* l_pPlayer = smart_cast<CGameObject*>(l_pObj);
		if (!l_pPlayer)
			return;

		NET_Packet P;
		l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
		P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
		P.w_u8(PLAYER_CHANGE_TEAM);

		P.w_s16(s16(Team + 1));
		// P.w_u32			(0);
		l_pPlayer->u_EventSend(P);
		//-----------------------------------------------------------------
		m_bSkinSelected = FALSE;
	};
	//-----------------------------------------------------------------
	m_bTeamSelected = TRUE;
	//---------------------------
	//	if (m_bMenuCalledFromReady)
	//	{
	//		OnKeyboardPress(kJUMP);
	//	}
};
//-----------------------------------------------------------------
void game_cl_TeamDeathmatch::SetCurrentBuyMenu()
{
	if (!local_player)
		return;
	if (!local_player->team || local_player->skin == -1)
		return;
	if (!pCurBuyMenu)
	{
		if (local_player->team == 1)
		{
			pCurBuyMenu = InitBuyMenu(GetBaseCostSect(), 1);
			LoadTeamDefaultPresetItems(GetTeamMenu(1), pCurBuyMenu, &PresetItemsTeam1);
			pCurPresetItems = &PresetItemsTeam1;
		}
		else
		{
			pCurBuyMenu = InitBuyMenu(GetBaseCostSect(), 2);
			LoadTeamDefaultPresetItems(GetTeamMenu(2), pCurBuyMenu, &PresetItemsTeam2);
			pCurPresetItems = &PresetItemsTeam2;
		};
		LoadDefItemsForRank(pCurBuyMenu);
	};

	//	if (pCurBuyMenu) pCurBuyMenu->SetSkin(local_player->skin);
	if (!pCurBuyMenu)
		return;

	//-----------------------------------
	if (m_cl_dwWarmUp_Time != 0)
		pCurBuyMenu->IgnoreMoneyAndRank(true);
	else
		pCurBuyMenu->IgnoreMoneyAndRank(false);
	//-----------------------------------
};

void game_cl_TeamDeathmatch::SetCurrentSkinMenu()
{
	s16 new_team;
	if (!local_player)
		return;
	if (local_player->team == 1)
	{
		new_team = 1;
	}
	else
	{
		new_team = 2;
	}
	if (pCurSkinMenu && pCurSkinMenu->GetTeam() == new_team)
		return;

	if (pCurSkinMenu && new_team != pCurSkinMenu->GetTeam())
		if (pCurSkinMenu->IsShown())
			StartStopMenu(pCurSkinMenu, true);

	xr_delete(pCurSkinMenu);
	pCurSkinMenu = InitSkinMenu(new_team);
};

bool game_cl_TeamDeathmatch::CanBeReady()
{
	if (!local_player)
		return false;

	m_bMenuCalledFromReady = TRUE;

	if (!m_bTeamSelected)
	{
		m_bMenuCalledFromReady = FALSE;
		if (CanCallTeamSelectMenu())
			StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
		return false;
	}

	return inherited::CanBeReady();
};

char* game_cl_TeamDeathmatch::getTeamSection(int Team)
{
	switch (Team)
	{
	case 1: {
		return "teamdeathmatch_team1";
	}
	break;
	case 2: {
		return "teamdeathmatch_team2";
	}
	break;
	default:
		return NULL;
	};
};

#include "string_table.h"
#include "ui/teaminfo.h"

void game_cl_TeamDeathmatch::shedule_Update(u32 dt)
{
	CStringTable st;
	string512 msg;

	if (!m_game_ui && HUD().GetUI())
		m_game_ui = smart_cast<CUIGameTDM*>(HUD().GetUI()->UIGame());
	inherited::shedule_Update(dt);

	if (!m_game_ui)
		return;
	//---------------------------------------------------------
	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown() && !CanCallTeamSelectMenu())
		StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
	//---------------------------------------------------------

	switch (m_phase)
	{
	case GAME_PHASE_TEAM1_SCORES: {
		sprintf_s(msg, /*team %s wins*/ *st.translate("mp_team_wins"), CTeamInfo::GetTeam_name(1));
		m_game_ui->SetRoundResultCaption(msg);

		SetScore();
	}
	break;
	case GAME_PHASE_TEAM2_SCORES: {
		sprintf_s(msg, /*team %s wins*/ *st.translate("mp_team_wins"), CTeamInfo::GetTeam_name(2));
		m_game_ui->SetRoundResultCaption(msg);

		SetScore();
	}
	break;
	case GAME_PHASE_INPROGRESS: {
		if (local_player && !local_player->IsSkip())
		{
			if (Level().CurrentEntity() && Level().CurrentEntity()->CLS_ID == CLSID_SPECTATOR)
			{
				if (!(pCurBuyMenu && pCurBuyMenu->IsShown()) && !(pCurSkinMenu && pCurSkinMenu->IsShown()) &&
					!(m_game_ui->m_pMapDesc && m_game_ui->m_pMapDesc->IsShown()) &&
					(HUD().GetUI() && HUD().GetUI()->GameIndicatorsShown()))
				{
					if (!m_bTeamSelected)
						m_game_ui->SetPressJumpMsgCaption("mp_press_jump2select_team");
				};
			};
			SetScore();
		};
	}
	break;
	default: {
	}
	break;
	};
}

void game_cl_TeamDeathmatch::SetScore()
{
	if (local_player)
	{
		s16 lt = local_player->team;
		if (lt >= 0)
		{
			if (m_game_ui)
				m_game_ui->SetScoreCaption(teams[0].score, teams[1].score);
		}
	}
};

// BOOL	g_bShowPlayerNames = FALSE;

bool game_cl_TeamDeathmatch::OnKeyboardPress(int key)
{
	if (kTEAM == key)
	{
		if (m_game_ui)
		{
			if (CanCallTeamSelectMenu())
			{
				StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
			};

			return true;
		}
	};

	return inherited::OnKeyboardPress(key);
}

bool game_cl_TeamDeathmatch::IsEnemy(game_PlayerState* ps)
{
	if (!local_player)
		return false;
	return local_player->team != ps->team;
};

bool game_cl_TeamDeathmatch::IsEnemy(CEntityAlive* ea1, CEntityAlive* ea2)
{
	return (ea1->g_Team() != ea2->g_Team());
};

#define PLAYER_NAME_COLOR 0xff40ff40

void game_cl_TeamDeathmatch::OnRender()
{
	OPTICK_EVENT("game_cl_TeamDeathmatch::OnRender");

	if (local_player)
	{
		cl_TeamStruct* pTS = &TeamList[ModifyTeam(local_player->team)];
		PLAYERS_MAP_IT it = players.begin();
		for (; it != players.end(); ++it)
		{
			game_PlayerState* ps = it->second;
			u16 id = ps->GameID;
			if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
				continue;
			CObject* pObject = Level().Objects.net_Find(id);
			if (!pObject)
				continue;
			if (!pObject || pObject->CLS_ID != CLSID_OBJECT_ACTOR)
				continue;
			if (IsEnemy(ps))
				continue;
			if (ps == local_player)
				continue;

			float dup = 0.0f;
			if (/*m_bFriendlyNames &&*/ m_bShowPlayersNames)
			{
				VERIFY(pObject);
				CActor* pActor = smart_cast<CActor*>(pObject);
				VERIFY(pActor);
				Fvector IPos = pTS->IndicatorPos;
				IPos.y -= pTS->Indicator_r2;
				pActor->RenderText(ps->getName(), IPos, &dup, PLAYER_NAME_COLOR);
			}
			if (m_bFriendlyIndicators)
			{
				VERIFY(pObject);
				CActor* pActor = smart_cast<CActor*>(pObject);
				VERIFY(pActor);
				Fvector IPos = pTS->IndicatorPos;
				IPos.y += dup;
				pActor->RenderIndicator(IPos, pTS->Indicator_r1, pTS->Indicator_r2, pTS->IndicatorShader);
			};
		}
	};
	inherited::OnRender();
}

BOOL game_cl_TeamDeathmatch::CanCallBuyMenu()
{
	if (!m_game_ui)
		return FALSE;
	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
		return FALSE;
	if (!m_bTeamSelected)
		return FALSE;
	if (!m_bSkinSelected)
		return FALSE;

	return inherited::CanCallBuyMenu();
};

BOOL game_cl_TeamDeathmatch::CanCallSkinMenu()
{
	if (!m_game_ui)
		return FALSE;
	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
		return FALSE;
	if (!m_bTeamSelected)
		return FALSE;

	return inherited::CanCallSkinMenu();
};

BOOL game_cl_TeamDeathmatch::CanCallInventoryMenu()
{
	if (!m_game_ui)
		return FALSE;
	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
		return FALSE;

	return inherited::CanCallInventoryMenu();
};

BOOL game_cl_TeamDeathmatch::CanCallTeamSelectMenu()
{
	if (Phase() != GAME_PHASE_INPROGRESS)
		return false;
	if (!local_player)
		return false;
	if (m_game_ui->m_pInventoryMenu && m_game_ui->m_pInventoryMenu->IsShown())
	{
		return FALSE;
	};
	if (pCurBuyMenu && pCurBuyMenu->IsShown())
	{
		return FALSE;
	};
	if (pCurSkinMenu && pCurSkinMenu->IsShown())
	{
		return FALSE;
	};

	m_game_ui->m_pUITeamSelectWnd->SetCurTeam(ModifyTeam(local_player->team));
	return TRUE;
};

#define FRIEND_LOCATION "mp_friend_location"

void game_cl_TeamDeathmatch::UpdateMapLocations()
{
	inherited::UpdateMapLocations();
	if (local_player)
	{
		PLAYERS_MAP_IT it = players.begin();
		for (; it != players.end(); ++it)
		{
			game_PlayerState* ps = it->second;
			u16 id = ps->GameID;
			if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
			{
				Level().MapManager().RemoveMapLocation(FRIEND_LOCATION, id);
				continue;
			};

			CObject* pObject = Level().Objects.net_Find(id);
			if (!pObject || pObject->CLS_ID != CLSID_OBJECT_ACTOR)
				continue;
			if (IsEnemy(ps))
			{
				if (Level().MapManager().HasMapLocation(FRIEND_LOCATION, id))
				{
					Level().MapManager().RemoveMapLocation(FRIEND_LOCATION, id);
				};
				continue;
			};
			if (!Level().MapManager().HasMapLocation(FRIEND_LOCATION, id))
			{
				(Level().MapManager().AddMapLocation(FRIEND_LOCATION, id))->EnablePointer();
			}
		}
	};
};

void game_cl_TeamDeathmatch::LoadSndMessages()
{
	//	LoadSndMessage("dm_snd_messages", "you_won", ID_YOU_WON);
	LoadSndMessage("tdm_snd_messages", "team1_win", ID_TEAM1_WIN);
	LoadSndMessage("tdm_snd_messages", "team2_win", ID_TEAM2_WIN);
	LoadSndMessage("tdm_snd_messages", "teams_equal", ID_TEAMS_EQUAL);
	LoadSndMessage("tdm_snd_messages", "team1_lead", ID_TEAM1_LEAD);
	LoadSndMessage("tdm_snd_messages", "team2_lead", ID_TEAM2_LEAD);

	LoadSndMessage("tdm_snd_messages", "team1_rank1", ID_TEAM1_RANK_1);
	LoadSndMessage("tdm_snd_messages", "team1_rank2", ID_TEAM1_RANK_2);
	LoadSndMessage("tdm_snd_messages", "team1_rank3", ID_TEAM1_RANK_3);
	LoadSndMessage("tdm_snd_messages", "team1_rank4", ID_TEAM1_RANK_4);

	LoadSndMessage("tdm_snd_messages", "team2_rank1", ID_TEAM2_RANK_1);
	LoadSndMessage("tdm_snd_messages", "team2_rank2", ID_TEAM2_RANK_2);
	LoadSndMessage("tdm_snd_messages", "team2_rank3", ID_TEAM2_RANK_3);
	LoadSndMessage("tdm_snd_messages", "team2_rank4", ID_TEAM2_RANK_4);
};

void game_cl_TeamDeathmatch::OnSwitchPhase_InProgress()
{
	if (!m_bSkinSelected)
		m_bTeamSelected = FALSE;
};

void game_cl_TeamDeathmatch::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
	inherited::OnSwitchPhase(old_phase, new_phase);
	switch (new_phase)
	{
	case GAME_PHASE_TEAM1_SCORES: {
		if (Level().CurrentViewEntity())
			PlaySndMessage(ID_TEAM1_WIN);
	}
	break;
	case GAME_PHASE_TEAM2_SCORES: {
		if (Level().CurrentViewEntity())
			PlaySndMessage(ID_TEAM2_WIN);
	}
	break;
	default: {
	}
	break;
	};
}

void game_cl_TeamDeathmatch::OnTeamChanged()
{
	xr_delete(pCurBuyMenu);
	SetCurrentBuyMenu();
	inherited::OnTeamChanged();
};

void game_cl_TeamDeathmatch::PlayRankChangesSndMessage()
{
	if (local_player)
	{
		switch (local_player->rank)
		{
		case 0:
			break;
		default:
			if (local_player->team == 1)
				PlaySndMessage(ID_TEAM1_RANK_0 + local_player->rank);
			if (local_player->team == 2)
				PlaySndMessage(ID_TEAM2_RANK_0 + local_player->rank);
			break;
		}
	}
};

void game_cl_TeamDeathmatch::OnGameMenuRespond_ChangeTeam(NET_Packet& P)
{
	s16 OldTeam = local_player->team;
	local_player->team = u8(P.r_s16() & 0x00ff);
	if (OldTeam != local_player->team)
		OnTeamChanged();

	SetCurrentSkinMenu();
	if (pCurSkinMenu)
	{
		pCurSkinMenu->SetCurSkin(local_player->skin);
		if (CanCallSkinMenu())
			StartStopMenu(pCurSkinMenu, true);
	}
};
