////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: NSDeathman
// Refactored code: Launcher realization
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////
extern "C"
{
	typedef int __cdecl LauncherFunc(int);
}
////////////////////////////////////////////////////////////////////////////////
HMODULE hLauncher = NULL;
LauncherFunc* pLauncher = NULL;
////////////////////////////////////////////////////////////////////////////////
void InitLauncher()
{
	if (hLauncher)
		return;
	hLauncher = LoadLibrary("xrLauncher.dll");
	if (0 == hLauncher)
		R_CHK(GetLastError());
	R_ASSERT2(hLauncher, "xrLauncher DLL raised exception during loading or there is no xrLauncher.dll at all");

	pLauncher = (LauncherFunc*)GetProcAddress(hLauncher, "RunXRLauncher");
	R_ASSERT2(pLauncher, "Cannot obtain RunXRLauncher function from xrLauncher.dll");
};

void FreeLauncher()
{
	if (hLauncher)
	{
		FreeLibrary(hLauncher);
		hLauncher = NULL;
		pLauncher = NULL;
	};
}

int doLauncher()
{
#pragma todo("NSDeathman to NSDeathman: Починить лаунчер и режим бенчмарка")
	/*
		execUserScript();
		InitLauncher();
		int res = pLauncher(0);
		FreeLauncher();
		if(res == 1) // do benchmark
			g_bBenchmark = true;

		if(g_bBenchmark){ //perform benchmark cycle
			doBenchmark();

			// InitLauncher	();
			// pLauncher	(2);	//show results
			// FreeLauncher	();

			Core._destroy			();
			return					(1);
		};
		if(res==8){//Quit
			Core._destroy			();
			return					(1);
		}
	*/
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
