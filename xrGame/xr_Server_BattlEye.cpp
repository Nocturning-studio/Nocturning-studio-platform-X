#include "stdafx.h"

#include "xr_Server_BattlEye.h"

#include "xrMessages.h"
#include "Level.h"
#include "xrServer.h"
#include "game_cl_base.h"

#include "../xrGameSpy/xrGameSpy_MainDefs.h"
#include "xrGameSpyServer.h"
#include "gamespy/GameSpy_Full.h"

#ifdef BATTLEYE

extern int g_be_message_out;
ENGINE_API bool g_dedicated_server;

BattlEyeServer::BattlEyeServer(xrServer* Server)
{
	m_module = NULL;
	pfnRun = NULL;
	pfnCommand = NULL;
	pfnAddPlayer = NULL;
	pfnRemovePlayer = NULL;
	pfnNewPacket = NULL;
	Init = NULL;
	m_succefull = false;

	if (!Level().battleye_system.InitDir())
	{
		return;
	}
	m_pServer = Server;

	m_module = LoadLibrary(Level().battleye_system.GetServerPath()); //=
	if (!m_module)
	{
		Msg("! Error LoadLibrary %s", BATTLEYE_SERVER_DLL);
		return;
	}
	//	string_path		path_dll;
	//	GetModuleFileName( m_module, path_dll, sizeof(path_dll) );
	//	Level().battleye_system.SetServerPath( path_dll );

	Init = (InitSrv_t)(GetProcAddress(m_module, "Init")); //=
	if (!Init)
	{
		Msg("! Error GetProcAddress <Init> from %s", BATTLEYE_SERVER_DLL);
		if (!FreeLibrary(m_module))
		{
			Msg("! Error FreeLibrary for %s", BATTLEYE_SERVER_DLL);
		}
		m_module = NULL;
		return;
	}

	string64 game_version;
	strcpy_s(game_version, GAME_VERSION);

	m_succefull = Init(game_version,
					   //		Level().battleye_system.auto_update,
					   &PrintMessage, &SendPacket, &KickPlayer, &pfnExit, &pfnRun, &pfnCommand, &pfnAddPlayer,
					   &pfnRemovePlayer, &pfnNewPacket);

	if (!m_succefull)
	{
		Msg("! Error initialization of %s (function Init return false)", BATTLEYE_SERVER_DLL);
		if (!FreeLibrary(m_module))
		{
			Msg("! Error FreeLibrary for %s", BATTLEYE_SERVER_DLL);
		}
		m_module = NULL;
		pfnRun = NULL;
		pfnCommand = NULL;
		pfnAddPlayer = NULL;
		pfnRemovePlayer = NULL;
		pfnNewPacket = NULL;
		return;
	}
}

void BattlEyeServer::AddConnectedPlayers() // if net_Ready
{
	Level().Server->clients_Lock();
	u32 cnt = Level().Server->game->get_players_count();
	for (u32 it = 0; it < cnt; ++it)
	{
		xrClientData* CL = (xrClientData*)Level().Server->client_Get(it);
		if (CL->net_Ready)
		{
			AddConnected_OnePlayer(CL);
		}
	}
	Level().Server->clients_Unlock();
}

void BattlEyeServer::AddConnected_OnePlayer(xrClientData* CL)
{
	if (g_dedicated_server && (CL->ID.value() == Level().Server->GetServerClient()->ID.value()))
	{
		return;
	}

	if (CL->m_guid[0] == 0)
	{
		AddPlayer(CL->ID.value(), (char*)CL->ps->getName(), 0, 0);
	}
	else
	{
		AddPlayer(CL->ID.value(), (char*)CL->ps->getName(), CL->m_guid, xr_strlen(CL->m_guid) + 1);
	}
}

void BattlEyeServer::PrintMessage(char* message)
{
	// if( g_be_message_out )
	{
		string512 text;
		sprintf_s(text, sizeof(text), "BattlEye Server: %s", message);
		Msg("%s", text);

		if (g_be_message_out) //==2
		{
			if (Level().game)
			{
				Level().game->CommonMessageOut(text);
			}
		}
	}
}

void BattlEyeServer::SendPacket(int player, void* packet, int len)
{
	NET_Packet P;
	P.w_begin(M_BATTLEYE);
	P.w_u32(len);
	P.w(packet, len);
	Level().Server->SendTo((u32)player, P, net_flags());
}

void BattlEyeServer::KickPlayer(int player, char* reason)
{
	Level().Server->clients_Lock();

	u32 cnt = Level().Server->game->get_players_count();
	u32 it = 0;
	for (; it < cnt; ++it)
	{
		xrClientData* l_pC = (xrClientData*)Level().Server->client_Get(it);
		if (l_pC->ID.value() == (u32)player)
		{
			if (Level().Server->GetServerClient() != l_pC)
			{
				Msg("- Disconnecting : %s ! Kicked by BattlEye Server. Reason: %s", l_pC->ps->getName(), reason);
				string512 reason2;
				strcpy_s(reason2, reason);

				Level().Server->DisconnectClient(l_pC, reason2);
				break;
			}
			else
			{
				//				Msg("BattlEye Server disconnect server's client");
				//*CStringTable().translate( ui_st_kicked_by_battleye ); //reason translate?
				string512 reason2;
				sprintf_s(reason2, sizeof(reason2),
						  "  Disconnecting : %s !  Server's Client kicked by BattlEye Server.  Reason: %s",
						  l_pC->ps->getName(), reason);
				Msg(reason2);

				NET_Packet P;
				P.w_begin(M_GAMEMESSAGE);
				P.w_u32(GAME_EVENT_SERVER_DIALOG_MESSAGE);
				P.w_stringZ(reason2);
				Level().Server->SendBroadcast(l_pC->ID, P); // to all, except self

				Level().OnSessionTerminate(reason2); // to self
				Engine.Event.Defer("KERNEL:disconnect");
				break;
			}
		}
	}
	if (it == cnt)
	{
		Msg("! No such player found : %i", player);
	}

	Level().Server->clients_Unlock();
}

bool BattlEyeServer::Run()
{
	R_ASSERT(m_module);
	R_ASSERT(pfnRun);
	return pfnRun();
}

void BattlEyeServer::Command(char* command)
{
	R_ASSERT(m_module);
	R_ASSERT(pfnCommand);
	pfnCommand(command);
}

void BattlEyeServer::AddPlayer(int player, char* name, void* guid, int guidlen)
{
	R_ASSERT(m_module);
	R_ASSERT(pfnAddPlayer);
	pfnAddPlayer(player, name, guid, guidlen);
}

void BattlEyeServer::RemovePlayer(int player)
{
	R_ASSERT(m_module);
	R_ASSERT(pfnRemovePlayer);
	pfnRemovePlayer(player);
}

void BattlEyeServer::NewPacket(int player, void* packet, int len)
{
	R_ASSERT(m_module);
	R_ASSERT(pfnNewPacket);
	pfnNewPacket(player, packet, len);
}

bool BattlEyeServer::IsLoaded()
{
	return m_module != NULL;
}

BattlEyeServer::~BattlEyeServer()
{
	ReleaseDLL();
}

void BattlEyeServer::ReleaseDLL()
{
	if (m_succefull)
	{
		if (!pfnExit())
		{
			Msg("! Error unloading data in %s", BATTLEYE_SERVER_DLL);
		}
	}
	if (m_module)
	{
		if (!FreeLibrary(m_module))
		{
			Msg("! Error FreeLibrary for %s", BATTLEYE_SERVER_DLL);
		}
	}
	m_module = NULL;
	pfnRun = NULL;
	pfnCommand = NULL;
	pfnAddPlayer = NULL;
	pfnRemovePlayer = NULL;
	pfnNewPacket = NULL;
}

#endif // BATTLEYE
