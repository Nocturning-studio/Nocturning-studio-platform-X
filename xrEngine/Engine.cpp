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
extern void __cdecl xrBind_PSGP(xrDispatchTable* T, DWORD dwFeatures);

void CEngine::Initialize(void)
{
	Msg("Initializing Engine...");

	// Bind PSGP
	xrBind_PSGP(&PSGP, true);

	// Other stuff
	Msg("Initializing Engine Sheduler...");
	Engine.Sheduler.Initialize();

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
}
