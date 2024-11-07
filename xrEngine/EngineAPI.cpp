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
extern u32 renderer_value; // con cmd

void CEngineAPI::Initialize(void)
{
	Msg("Initializing Engine API...");
	//////////////////////////////////////////////////////////////////////////
	// render
	LPCSTR r2_name = "xrRender_R2.dll";

	Msg("Initializing Renderer...");

	// try to initialize R2
	Log("Loading DLL:", r2_name);
	hRender = LoadLibrary(r2_name);

	if (0 == hRender)
		Msg("Loading failed - incompatible hardware.");

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
		R_ASSERT(pCreate);

		pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");
		R_ASSERT(pDestroy);
	}

	//////////////////////////////////////////////////////////////////////////
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
	pCreate = 0;
	pDestroy = 0;
	Engine.Event._destroy();
	XRC.r_clear_compact();
}
