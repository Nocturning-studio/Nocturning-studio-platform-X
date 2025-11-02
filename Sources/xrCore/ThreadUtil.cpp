/*
	Copyright (c) 2014-2021 OpenXRay
	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
		http://www.apache.org/licenses/LICENSE-2.0
	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#include "stdafx.h"
#include "ThreadUtil.h"
#include <Windows.h>

namespace Threading
{

ThreadId GetCurrThreadId()
{
	return GetCurrentThreadId();
}

ThreadHandle GetCurrentThreadHandle()
{
	return GetCurrentThread();
}

void SetThreadName(ThreadHandle threadHandle, pcstr name)
{
	const DWORD MSVC_EXCEPTION = 0x406D1388;
	DWORD threadId = threadHandle != NULL ? GetThreadId(threadHandle) : DWORD(-1);

	struct SThreadNameInfo
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	};

	SThreadNameInfo info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = threadId;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MSVC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

u32 __stdcall ThreadEntry(void* params)
{
	SThreadStartupInfo* args = (SThreadStartupInfo*)params;
	SetThreadName(NULL, args->threadName);
	EntryFuncType entry = args->entryFunc;
	void* arglist = args->argList;
	xr_delete(args);
	_initialize_cpu_thread();

	// call
	entry(arglist);

	return 0;
}

bool SpawnThread(EntryFuncType entry, pcstr name, u32 stack, void* arglist)
{
	OPTICK_THREAD(name);
	OPTICK_FRAME(name);
	Msg("Spawning thread: %s", name);

	Debug._initialize(false);

	SThreadStartupInfo* info = xr_new<SThreadStartupInfo>();
	info->threadName = name;
	info->entryFunc = entry;
	info->argList = arglist;
	ThreadHandle threadHandle = (ThreadHandle)_beginthreadex(NULL, stack, ThreadEntry, info, CREATE_SUSPENDED, NULL);

	if (!threadHandle)
	{
		xr_string errMsg = Debug.error2string(GetLastError());
		Msg("SpawnThread: can't create thread '%s'. Error Msg: '%s'", name, errMsg.c_str());
		return false;
	}

	ResumeThread(threadHandle);
	return true;
}

void WaitThread(ThreadHandle& threadHandle)
{
	WaitForSingleObject(threadHandle, INFINITE);
}

void CloseThreadHandle(ThreadHandle& threadHandle)
{
	if (threadHandle)
	{
		CloseHandle(threadHandle);
		threadHandle = nullptr;
	}
}
} // namespace Threading