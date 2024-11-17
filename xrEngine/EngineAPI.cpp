// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "xrXRC.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy(void){};
CEngineAPI::CEngineAPI()
{
	hGame = 0;
	hRender = 0;
	hTuner = 0;
	pCreate = 0;
	pDestroy = 0;
	tune_pause = dummy;
	tune_resume = dummy;
}

CEngineAPI::~CEngineAPI()
{
}

void CEngineAPI::Initialize(void)
{
	Msg("Initializing Engine API...");

	// render
	Msg("Initializing Renderer...");
	LPCSTR render_name = "xrRender_PC.dll";
	Log("Loading DLL:", render_name);
	hRender = LoadLibrary(render_name);
	R_ASSERT2(hRender, "! Can't load renderer");

	// game
	{
		Msg("Initializing Game API...");

		LPCSTR g_name = "xrGame.dll";
		Msg("Loading DLL: %s", g_name);
		hGame = LoadLibrary(g_name);

		if (0 == hGame)
			R_CHK(GetLastError());

		R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");

		Msg("Initializing xrFactory...");
		pCreate = (Factory_Create*)GetProcAddress(hGame, "xrFactory_Create");
		R_ASSERT2(pCreate, "Error in xrFactory_Create");

		pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");
		R_ASSERT2(pDestroy, "Error in xrFactory_Destroy");
	}

	// vTune
	tune_enabled = FALSE;
	if (strstr(Core.Params, "-tune"))
	{
		LPCSTR g_name = "vTuneAPI.dll";
		Log("Loading DLL:", g_name);
		hTuner = LoadLibrary(g_name);
		if (0 == hTuner)
		{
			R_CHK(GetLastError());
			Msg("Intel vTune is not installed");
		}
		else
		{
			tune_enabled = TRUE;
			tune_pause = (VTPause*)GetProcAddress(hTuner, "VTPause");
			tune_resume = (VTResume*)GetProcAddress(hTuner, "VTResume");
		}
	}

	LPCSTR DiscordAPI_name = "DiscordAPI.dll";
	Log("Loading DLL:", DiscordAPI_name);
	hDiscordAPI = LoadLibrary(DiscordAPI_name);
	R_ASSERT2(DiscordAPI_name, "! Can't load discord api");
}

void CEngineAPI::Destroy(void)
{
	if (hGame)
	{
		FreeLibrary(hGame);
		hGame = 0;
	}
	if (hRender)
	{
		FreeLibrary(hRender);
		hRender = 0;
	}
	if (hDiscordAPI)
	{
		FreeLibrary(hDiscordAPI);
		hDiscordAPI = 0;
	}
	pCreate = 0;
	pDestroy = 0;
	Engine.Event._destroy();
	XRC.r_clear_compact();
}
