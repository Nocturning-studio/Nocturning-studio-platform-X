// Engine.cpp: implementation of the CEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Engine.h"

CEngine Engine;
xrDispatchTable PSGP;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEngine::CEngine()
{
}

CEngine::~CEngine()
{
}

extern void msCreate(LPCSTR name);

void CEngine::Initialize(void)
{
	Msg("Initializing Engine...");

	// Bind PSGP
	LPCSTR xrCPUPipeDllName = "xrCPU_Pipe.dll";
	Msg("Loading DLL: %s", xrCPUPipeDllName);
	hPSGP = LoadLibrary(xrCPUPipeDllName);
	if (!hPSGP)
		make_string("Can't load xrCPU_Pipe.dll, please reinstall application");

	xrBinder* bindCPU = (xrBinder*)GetProcAddress(hPSGP, "xrBind_PSGP");
	R_ASSERT(bindCPU);
	bindCPU(&PSGP, true);

	// Other stuff
	Msg("Initializing Engine Sheduler...");
	Engine.Sheduler.Initialize();
	//
#ifdef DEBUG
	msCreate("game");
#endif
}

void CEngine::Destroy()
{
	Engine.Sheduler.Destroy();
#ifdef DEBUG_MEMORY_MANAGER
	extern void dbg_dump_leaks_prepare();
	if (Memory.debug_mode)
		dbg_dump_leaks_prepare();
#endif // DEBUG_MEMORY_MANAGER
	Engine.External.Destroy();



	if (hPSGP)
	{
		FreeLibrary(hPSGP);
		hPSGP = 0;
		ZeroMemory(&PSGP, sizeof(PSGP));
	}
}
