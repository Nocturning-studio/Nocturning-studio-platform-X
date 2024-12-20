// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef stdafxH
#define stdafxH
#pragma once
#include "xrCore.h"
#include "xrCore_platform.h"
#include "Threading.h"

#ifdef ENABLE_PROFILING
#include <optick/optick.h>
#pragma comment(lib, "OptickCore.lib")
#endif

#endif

#pragma hdrstop
