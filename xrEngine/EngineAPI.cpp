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

ENGINE_API bool is_enough_address_space_available()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
}

CEngineAPI::~CEngineAPI()
{
#ifdef FEATURE_LEVELS_DEBUG
	if (feature_level_token)
	{
		for (int i = 0; feature_level_token[i].name; i++)
		{
			xr_free(feature_level_token[i].name);
		}
		xr_free(feature_level_token);
		feature_level_token = NULL;
	}
#endif
}

#ifdef FEATURE_LEVELS_DEBUG
extern xr_token* feature_level_token;
#endif

extern u32 renderer_value; // con cmd

void CEngineAPI::Initialize(void)
{
	Msg("Initializing Renderer...");

#ifdef FEATURE_LEVELS_DEBUG
	{
		u32 size = 1;

		switch (support.level)
		{
		case D3D_FEATURE_LEVEL_10_0:
			size = 3;
			break;
		case D3D_FEATURE_LEVEL_10_1:
			size = 4;
			break;
		case D3D_FEATURE_LEVEL_11_0:
			size = 5;
			break;
		case D3D_FEATURE_LEVEL_11_1:
			size = 6;
			break;
		case D3D_FEATURE_LEVEL_12_0:
			size = 7;
			break;
		case D3D_FEATURE_LEVEL_12_1:
			size = 8;
			break;
		}

		feature_level_token = xr_alloc<xr_token>(size);

		xr_token feature_level_token_full[] = {
			{"opt_auto", 0},
			{"opt_10_0", D3D_FEATURE_LEVEL_10_0},
			{"opt_10_1", D3D_FEATURE_LEVEL_10_1},
			{"opt_11_0", D3D_FEATURE_LEVEL_11_0},
			{"opt_11_1", D3D_FEATURE_LEVEL_11_1},
			{"opt_12_0", D3D_FEATURE_LEVEL_12_0},
			{"opt_12_1", D3D_FEATURE_LEVEL_12_1},
		};

		Msg("* Feature level options: ");

		for (u32 i = 0; i < size - 1; i++)
		{
			feature_level_token[i].id = feature_level_token_full[i].id;
			feature_level_token[i].name = xr_strdup(feature_level_token_full[i].name);

			Msg("* \t%s", feature_level_token[i].name);
		}

		feature_level_token[size - 1].id = -1;
		feature_level_token[size - 1].name = NULL;
	}
#endif

	switch (renderer_value)
	{
	case RenderCreationParams::R_R2:
		Log("Loading DLL:", "xrRender_R2.dll");
		hRender = LoadLibrary("xrRender_R2.dll");
		break;
	case RenderCreationParams::R_R4:
		Log("Loading DLL:", "xrRender_R4.dll");
		hRender = LoadLibrary("xrRender_R4.dll");
		break;
	case RenderCreationParams::R_R1:
		Log("Loading DLL:", "xrRender_R1.dll");
		hRender = LoadLibrary("xrRender_R1.dll");
		break;
	}

	if (0 == hRender)
	{
		Msg("Loading failed - incompatible hardware.");
	}

	// game
	{
		Msg("Initializing Game API...");

		LPCSTR g_name = "xrGame.dll";
		Msg("Loading DLL: %s", g_name);
		hGame = LoadLibrary(g_name);

		if (0 == hGame)
			R_CHK(GetLastError());

		R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");

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
