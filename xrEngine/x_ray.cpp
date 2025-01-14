////////////////////////////////////////////////////////////////////////////////
// Authors: 
//			Oles (Oles Shishkovtsov)
//			AlexMX (Alexander Maksimchuk)
// 
// Engine class realization
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_input.h"
#include "xr_ioconsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "ispatial.h"
#include "Text_Console.h"
#include <process.h>
#include "../xrDiscordAPI/DiscordAPI.h"
#include "Application.h"
//////////////////////////////////////////////////////////////////////////
extern CRenderDevice Device;
//////////////////////////////////////////////////////////////////////////
ENGINE_API CApplication* pApp = NULL;
ENGINE_API CInifile* pGameIni = NULL;

ENGINE_API bool g_bBenchmark = false;
//////////////////////////////////////////////////////////////////////////
// -------------------------------------------
// startup point
CXRay::CXRay()
{
	m_bIntroState = TRUE;
	logoWindow = NULL;
}

void CXRay::InitEngine()
{
	InitSettings();

	Engine.Initialize();

	while (GetIntroState() == true)
		Sleep(100);

	Device.Initialize();

	InitInput();

	InitConsole();
}

void CXRay::InitSettings()
{
	Msg("Initializing Settings...");

	string_path fname;
	FS.update_path(fname, "$game_config$", "system.ltx");
	pSettings = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pSettings->sections().empty(),
				  make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	FS.update_path(fname, "$game_config$", "game.ltx");
	pGameIni = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(!pGameIni->sections().empty(),
				  make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

void CXRay::InitConsole()
{
	Msg("Initializing Console...");

#ifdef DEDICATED_SERVER
		Console = xr_new<CTextConsole>();
#else
		Console = xr_new<CConsole>();
#endif

	Console->Initialize();

	if (strstr(Core.Params, "-ltx "))
	{
		string64 c_name;
		(void)sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
		strcpy_s(Console->ConfigFile, c_name);
		Msg("Execute custom game settings file: %s", c_name);
	}
	else
	{
		strcpy_s(Console->ConfigFile, sizeof(Console->ConfigFile), "user_");
		strconcat(sizeof(Console->ConfigFile), Console->ConfigFile, Console->ConfigFile, Core.UserName);
		strconcat(sizeof(Console->ConfigFile), Console->ConfigFile, Console->ConfigFile, "_game_settings.ltx");
		Msg("Execute game settings file: %s", Console->ConfigFile);
	}
}

void CXRay::InitInput()
{
	BOOL bCaptureInput = !strstr(Core.Params, "-i");
	if (g_dedicated_server)
		bCaptureInput = FALSE;

	pInput = xr_new<CInput>(bCaptureInput);
}

void CXRay::destroyInput()
{
	xr_delete(pInput);
}

void CXRay::InitSound()
{
	Msg("Initializing Sound...");
	CSound_manager_interface::_create(u64(Device.m_hWnd));
}

void CXRay::destroySound()
{
	CSound_manager_interface::_destroy();
}

void CXRay::destroySettings()
{
	xr_delete(pSettings);
	xr_delete(pGameIni);
}

void CXRay::destroyConsole()
{
	Console->Destroy();
	xr_delete(Console);
}

void CXRay::destroyEngine()
{
	Device.Destroy();
	Engine.Destroy();
}

void CXRay::execUserScript()
{
	// Execute script

	Console->Execute("unbindall");
	Console->ExecuteScript(Console->ConfigFile);
}

void slowdownthread(void*)
{
	OPTICK_EVENT("X-Ray Slowdown thread");
	OPTICK_FRAME("X-Ray Slowdown thread");
	for (;;)
	{
		if (Device.Statistic->fFPS < 30)
			Sleep(1);
		if (Device.mt_bMustExit)
			return;
		if (0 == pSettings)
			return;
		if (0 == Console)
			return;
		if (0 == pInput)
			return;
		if (0 == pApp)
			return;
	}
}

void CheckPrivilegySlowdown()
{
#ifdef DEBUG
	if (strstr(Core.Params, "-slowdown"))
	{
		Threading::SpawnThreadthread_spawn(slowdownthread, "Debug Slowdown thread", 0, 0);
	}
	if (strstr(Core.Params, "-slowdown2x"))
	{
		Threading::SpawnThreadthread_spawn(slowdownthread, "Debug Slowdown thread 0", 0, 0);
		Threading::SpawnThreadthread_spawn(slowdownthread, "Debug Slowdown thread 1", 0, 0);
	}
#endif // DEBUG
}

void CXRay::Startup()
{
	execUserScript();
	InitSound();

	// ...command line for auto start
	{
		LPCSTR pStartup = strstr(Core.Params, "-start ");
		if (pStartup)
			Console->Execute(pStartup + 1);
	}
	{
		LPCSTR pStartup = strstr(Core.Params, "-load ");
		if (pStartup)
			Console->Execute(pStartup + 1);
	}
	if (strstr(Core.Params, "-load_last_save"))
	{
			Console->Execute("load_last_save");
	}
	if (strstr(Core.Params, "-load_last_quick_save"))
	{
		Console->Execute("load_last_quick_save");
	}

	// Initialize APP
	// #ifndef DEDICATED_SERVER
	ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);
	Device.Create();
	// #endif
	LALib.OnCreate();
	pApp = xr_new<CApplication>();
	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);
	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

	// Destroy LOGO
	DestroyWindow(logoWindow);
	logoWindow = NULL;

	// Main cycle
	Memory.mem_usage();
	Device.Run();

	// Destroy APP
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	DEL_INSTANCE(g_pGamePersistent);
	xr_delete(pApp);
	Engine.Event.Dump();

	// Destroying
	destroySound();
	destroyInput();

	if (!g_bBenchmark)
		destroySettings();

	LALib.OnDestroy();

	if (!g_bBenchmark)
		destroyConsole();
	else
		Console->Destroy();

	destroyEngine();
}

#include "xr_ioc_cmd.h"

typedef void DUMMY_STUFF(const void*, const u32&, void*);
XRCORE_API DUMMY_STUFF* g_temporary_stuff;

#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"

void CXRay::DecodeResources()
{
	g_temporary_stuff = &trivial_encryptor::decode;
}

