/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx_.h"
#include "BugslayerUtil.h"

// Windows 7 and above are always NT-based systems
BOOL __stdcall IsNT(void)
{
	return TRUE;
}