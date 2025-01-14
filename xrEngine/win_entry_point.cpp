////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: Deathman
// Refactored code: Windows application entry point
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "build_identificator.h"
#include "resource.h"
#include "x_ray.h"
////////////////////////////////////////////////////////////////////////////////
ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
static HWND logoWindow = NULL;
ENGINE_API bool g_dedicated_server = false;

ENGINE_API CXRay* pXRay = NULL;
////////////////////////////////////////////////////////////////////////////////
static BOOL CALLBACK logoDlgProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		DestroyWindow(hw);
		break;
	case WM_COMMAND:
		if (LOWORD(wp) == IDCANCEL)
			DestroyWindow(hw);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int stack_overflow_exception_filter(int exception_code)
{
	if (exception_code == EXCEPTION_STACK_OVERFLOW)
	{
		// Do not call _resetstkoflw here, because
		// at this point, the stack is not yet unwound.
		// Instead, signal that the handler (the __except block)
		// is to be executed.
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

#define VIRT_ERROR_SIZE 256
#define VIRT_MESSAGE_SIZE 512

BOOL IsOutOfVirtualMemory()
{
	MEMORYSTATUSEX statex;
	DWORD dwPageFileInMB = 0;
	DWORD dwPhysMemInMB = 0;
	HINSTANCE hApp = 0;
	char pszError[VIRT_ERROR_SIZE];
	char pszMessage[VIRT_MESSAGE_SIZE];

	ZeroMemory(&statex, sizeof(MEMORYSTATUSEX));
	statex.dwLength = sizeof(MEMORYSTATUSEX);

	if (!GlobalMemoryStatusEx(&statex))
		return 0;

	dwPageFileInMB = (DWORD)(statex.ullTotalPageFile / (1024 * 1024));
	dwPhysMemInMB = (DWORD)(statex.ullTotalPhys / (1024 * 1024));

	// Довольно отфонарное условие
	if ((dwPhysMemInMB > 500) && ((dwPageFileInMB + dwPhysMemInMB) > 2500))
		return 0;

	hApp = GetModuleHandle(NULL);

	if (!LoadString(hApp, RC_VIRT_MEM_ERROR, pszError, VIRT_ERROR_SIZE))
		return 0;

	if (!LoadString(hApp, RC_VIRT_MEM_TEXT, pszMessage, VIRT_MESSAGE_SIZE))
		return 0;

	MessageBox(NULL, pszMessage, pszError, MB_OK | MB_ICONHAND);

	return 1;
}

int APIENTRY WinMain_impl(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
#ifndef DEDICATED_SERVER
	// Check for virtual memory
	if ((strstr(lpCmdLine, "--skipmemcheck") == NULL) && IsOutOfVirtualMemory())
		return 0;
#else  // DEDICATED_SERVER
	g_dedicated_server = true;
#endif // DEDICATED_SERVER

	pXRay = xr_new<CXRay>();

	// Title window
	pXRay->logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, logoDlgProc);
	SetWindowPos(pXRay->logoWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

	// AVI
	pXRay->SetIntroState(FALSE);

	g_sLaunchOnExit_app[0] = NULL;
	g_sLaunchOnExit_params[0] = NULL;

	LPCSTR fsgame_ltx_name = "-fsltx ";
	string_path fsgame = "";
	if (strstr(lpCmdLine, fsgame_ltx_name))
	{
		int sz = xr_strlen(fsgame_ltx_name);
		sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
	}

	pXRay->DecodeResources();

	compute_build_id();
	Core._initialize("X-Ray Engine", "xray_engine", NULL, TRUE, fsgame[0] ? fsgame : NULL);

	FPU::m24r();
	
	pXRay->InitEngine();

	#pragma todo(Deathman to Deathman: Реализовать бенчмарк)
	/*
	LPCSTR benchName = "-batch_benchmark ";
	if (strstr(lpCmdLine, benchName))
	{
		int sz = xr_strlen(benchName);
		string64 b_name;
		sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", b_name);
		doBenchmark(b_name);
		return 0;
	}
	*/

	#pragma todo(Deathman to Deathman : Реализовать лаунчер)
	/*
	if (strstr(lpCmdLine, "-launcher"))
	{
		int l_res = doLauncher();
		if (l_res != 0)
			return 0;
	};
	*/

	Engine.External.Initialize();

	pXRay->Startup();

	Core._destroy();

	char* _args[3];
	// check for need to execute something external
	if (xr_strlen(g_sLaunchOnExit_app))
	{
		string4096 ModuleFileName = "";
		GetModuleFileName(NULL, ModuleFileName, 4096);

		string4096 ModuleFilePath = "";
		char* ModuleName = NULL;
		GetFullPathName(ModuleFileName, 4096, ModuleFilePath, &ModuleName);
		ModuleName[0] = 0;
		strcat(ModuleFilePath, g_sLaunchOnExit_app);
		_args[0] = g_sLaunchOnExit_app;
		_args[1] = g_sLaunchOnExit_params;
		_args[2] = NULL;

		_spawnv(_P_NOWAIT, _args[0], _args);
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	__try
	{
#ifdef DEDICATED_SERVER
		Debug._initialize(true);
#else  // DEDICATED_SERVER
		Debug._initialize(false);
#endif // DEDICATED_SERVER

		WinMain_impl(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except (stack_overflow_exception_filter(GetExceptionCode()))
	{
		_resetstkoflw();
		FATAL("stack overflow");
	}

	return (0);
}