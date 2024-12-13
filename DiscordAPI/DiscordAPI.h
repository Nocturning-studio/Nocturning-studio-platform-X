/////////////////////////////////////////////////////////////////////
//	Created: 17.11.2024
//	Authors: Maks0, morrazzzz, NS_Deathman
/////////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <DiscordGameSDK/discord.h>
#include <atomic>
/////////////////////////////////////////////////////////////////////
#ifdef DISCORDAPI_EXPORTS
#define DISCORDAPI_API __declspec(dllexport)
#else
#define DISCORDAPI_API __declspec(dllimport)
#endif
/////////////////////////////////////////////////////////////////////
namespace discord
{
	class Activity;
	class Core;
}
/////////////////////////////////////////////////////////////////////
class DISCORDAPI_API CDiscordAPI
{
  private:
	xr_string m_StatusDiscord;
	xr_string m_PhaseDiscord;

	std::atomic_bool m_NeedUpdateActivity;

	s64 m_AppID = 1307777829374656562;

	discord::Activity* m_ActivityDiscord = nullptr;
	discord::Core* m_DiscordCore = nullptr;

  public:
	void Init();
	void AssotiateError(int ErrorCode);
	void Update();
	void UpdateActivity();
	void SetPhase(const xr_string& phase);
	void SetStatus(const xr_string& status);
	void SetAppID(s64 appId)
	{
		m_AppID = appId;
	}

	CDiscordAPI() = default;
	~CDiscordAPI();
};
/////////////////////////////////////////////////////////////////////
extern DISCORDAPI_API CDiscordAPI DiscordAPI;
/////////////////////////////////////////////////////////////////////
