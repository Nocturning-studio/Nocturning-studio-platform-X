/////////////////////////////////////////////////////////////////////
//	Created: 17.11.2024
//	Authors: Maks0, morrazzzz, NS_Deathman
/////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "DiscordAPI.h"
/////////////////////////////////////////////////////////////////////
DISCORDAPI_API CDiscordAPI Discord;
/////////////////////////////////////////////////////////////////////
CDiscordAPI::~CDiscordAPI()
{
	delete m_DiscordCore;
	delete m_ActivityDiscord;
}

void CDiscordAPI::Init()
{
	Msg("Initialising DiscordAPI...");
	auto ResultSDK_ = discord::Core::Create(m_AppID, DiscordCreateFlags_NoRequireDiscord, &m_DiscordCore);

	if (!m_DiscordCore)
	{
		int ErrorCode = static_cast<int>(ResultSDK_);
		Msg("! DiscordAPI: error while initializing");
		Msg("! Error Code: [%d]", ErrorCode);
		Msg("! Error Assotiation:");
		AssotiateError(ErrorCode);
		return;
	}

	m_ActivityDiscord = new discord::Activity();
	m_ActivityDiscord->GetAssets().SetLargeImage("logo_main");
	m_ActivityDiscord->GetAssets().SetSmallImage("logo_main");

	m_ActivityDiscord->SetInstance(true);
	m_ActivityDiscord->SetType(discord::ActivityType::Playing);
	m_ActivityDiscord->GetTimestamps().SetStart(time(nullptr));

	m_NeedUpdateActivity = true;
}

void CDiscordAPI::AssotiateError(int ErrorCode)
{
	switch (ErrorCode)
	{
	case 0:
		Msg("- DiscordResult: Ok");
		break;
	case 1:
		Msg("! DiscordResult: Service Unavailable");
		break;
	case 2:
		Msg("! DiscordResult: Invalid Version");
		break;
	case 3:
		Msg("! DiscordResult: Lock Failed");
		break;
	case 4:
		Msg("! DiscordResult: Internal Error");
		break;
	case 5:
		Msg("! DiscordResult: Invalid Payload");
		break;
	case 6:
		Msg("! DiscordResult: Invalid Command");
		break;
	case 7:
		Msg("! DiscordResult: Invalid Permissions");
		break;
	case 8:
		Msg("! DiscordResult: Not Fetched");
		break;
	case 9:
		Msg("! DiscordResult: Not Found");
		break;
	case 10:
		Msg("! DiscordResult: Conflict");
		break;
	case 11:
		Msg("! DiscordResult: Invalid Secret");
		break;
	case 12:
		Msg("! DiscordResult: Invalid Join Secret");
		break;
	case 13:
		Msg("! DiscordResult: No Eligible Activity");
		break;
	case 14:
		Msg("! DiscordResult: Invalid Invite");
		break;
	case 15:
		Msg("! DiscordResult: Not Authenticated");
		break;
	case 16:
		Msg("! DiscordResult: Invalid Access Token");
		break;
	case 17:
		Msg("! DiscordResult: Application Mismatch");
		break;
	case 18:
		Msg("! DiscordResult: Invalid Data Url");
		break;
	case 19:
		Msg("! DiscordResult: Invalid Base 64");
		break;
	case 20:
		Msg("! DiscordResult: Not Filtered");
		break;
	case 21:
		Msg("! DiscordResult: Lobby Full");
		break;
	case 22:
		Msg("! DiscordResult: Invalid Lobby Secret");
		break;
	case 23:
		Msg("! DiscordResult: Invalid Filename");
		break;
	case 24:
		Msg("! DiscordResult: Invalid File Size");
		break;
	case 25:
		Msg("! DiscordResult: Invalid Entitlement");
		break;
	case 26:
		Msg("! DiscordResult: Not Installed");
		break;
	case 27:
		Msg("! DiscordResult: Not Running");
		break;
	case 28:
		Msg("! DiscordResult: Insufficient Buffer");
		break;
	case 29:
		Msg("! DiscordResult: Purchase Canceled");
		break;
	case 30:
		Msg("! DiscordResult: Invalid Guild");
		break;
	case 31:
		Msg("! DiscordResult: Invalid Event");
		break;
	case 32:
		Msg("! DiscordResult: Invalid Channel");
		break;
	case 33:
		Msg("! DiscordResult: Invalid Origin");
		break;
	case 34:
		Msg("! DiscordResult: Rate Limited");
		break;
	case 35:
		Msg("! DiscordResult: OAuth2 Error");
		break;
	case 36:
		Msg("! DiscordResult: Select Channel Timeout");
		break;
	case 37:
		Msg("! DiscordResult: Get Guild Timeout");
		break;
	case 38:
		Msg("! DiscordResult: Select Voice Force Required");
		break;
	case 39:
		Msg("! DiscordResult: Capture Shortcut Already Listening");
		break;
	case 40:
		Msg("! DiscordResult: Unauthorized For Achievement");
		break;
	case 41:
		Msg("! DiscordResult: Invalid Gift Codet");
		break;
	case 42:
		Msg("! DiscordResult: Purchase Error");
		break;
	case 43:
		Msg("! DiscordResult: Transaction Aborted");
		break;
	case 44:
		Msg("! DiscordResult: Drawing Init Failed");
		break;
    }
}

void CDiscordAPI::Update()
{
	if (!m_DiscordCore)
		return;

	if (m_NeedUpdateActivity)
		UpdateActivity();

	m_DiscordCore->RunCallbacks();
}

void CDiscordAPI::UpdateActivity()
{
	m_ActivityDiscord->SetState(ANSIToUTF8(m_PhaseDiscord).c_str());
	m_ActivityDiscord->SetDetails(ANSIToUTF8(m_StatusDiscord).c_str());

	m_DiscordCore->ActivityManager().UpdateActivity(*m_ActivityDiscord, [](discord::Result result)
	{
		if (result != discord::Result::Ok)
			Msg("! DiscordAPI: Invalid UpdateActivity");
	});

	m_NeedUpdateActivity = false;
}

void CDiscordAPI::SetPhase(const xr_string& phase)
{
	m_PhaseDiscord = phase;
	m_NeedUpdateActivity = true;
}

void CDiscordAPI::SetStatus(const xr_string& status)
{
	m_StatusDiscord = status;
	m_NeedUpdateActivity = true;
}
/////////////////////////////////////////////////////////////////////
