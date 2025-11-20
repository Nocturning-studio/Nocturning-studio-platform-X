/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#include "stdafx_.h"
#include "BugslayerUtil.h"

// The project internal header file.
#include "Internal.h"

// The documentation for this function is in BugslayerUtil.h.
BOOL __stdcall GetLoadedModules(DWORD dwPID, UINT uiCount, HMODULE* paModArray, LPDWORD pdwRealCount)
{
	// Basic parameter validation
	if (pdwRealCount == nullptr)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (uiCount > 0 && paModArray == nullptr)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// For Windows 7 and above, we can safely use ToolHelp API
	// Note: We assume the process has appropriate privileges
	return TLHELPGetLoadedModules(dwPID, uiCount, paModArray, pdwRealCount);
}

// Alternative modern implementation using EnumProcessModules
BOOL __stdcall GetLoadedModulesEx(DWORD dwPID, UINT uiCount, HMODULE* paModArray, LPDWORD pdwRealCount)
{
	if (pdwRealCount == nullptr)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);
	if (hProcess == nullptr)
	{
		// Try with less privileges
		hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPID);
		if (hProcess == nullptr)
		{
			return FALSE;
		}
	}

	BOOL bSuccess = FALSE;
	HMODULE* hMods = nullptr;
	DWORD cbNeeded = 0;

	// First call to get the size needed
	if (EnumProcessModules(hProcess, nullptr, 0, &cbNeeded))
	{
		DWORD moduleCount = cbNeeded / sizeof(HMODULE);
		*pdwRealCount = moduleCount;

		if (uiCount == 0 || paModArray == nullptr)
		{
			// Caller just wants the count
			bSuccess = TRUE;
		}
		else if (uiCount >= moduleCount)
		{
			// Allocate buffer for the modules
			hMods = new HMODULE[moduleCount];

			if (EnumProcessModules(hProcess, hMods, cbNeeded, &cbNeeded))
			{
				// Copy to caller's array
				for (DWORD i = 0; i < min(moduleCount, uiCount); i++)
				{
					paModArray[i] = hMods[i];
				}
				bSuccess = TRUE;
			}

			delete[] hMods;
		}
		else
		{
			// Buffer too small
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			bSuccess = FALSE;
		}
	}

	CloseHandle(hProcess);
	return bSuccess;
}

// Simplified version that just returns the count
DWORD __stdcall GetLoadedModulesCount(DWORD dwPID)
{
	DWORD count = 0;
	GetLoadedModules(dwPID, 0, nullptr, &count);
	return count;
}
