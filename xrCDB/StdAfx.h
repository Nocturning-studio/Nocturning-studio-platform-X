// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef stdafxH
#define stdafxH
#pragma once

#include "../xrCore/xrCore.h"

#define ENGINE_API
#include "opcode.h"

#include <optick/optick.h>
#ifdef ENABLE_PROFILING
#pragma comment(lib, "OptickCore.lib")
#endif

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // stdafxH
