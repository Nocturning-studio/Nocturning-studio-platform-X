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
#pragma once

#include <process.h>

namespace Threading
{

using ThreadHandle = HANDLE;
using ThreadId = u32;
using EntryFuncType = void (*)(void*);

struct SThreadStartupInfo
{
	pcstr threadName;
	EntryFuncType entryFunc;
	void* argList;
};

//////////////////////////////////////////////////////////////

XRCORE_API ThreadId GetCurrThreadId();

XRCORE_API ThreadHandle GetCurrentThreadHandle();

XRCORE_API void SetThreadName(ThreadHandle threadHandle, pcstr name);

XRCORE_API bool SpawnThread(EntryFuncType entry, pcstr name, u32 stack, void* arglist);

XRCORE_API void WaitThread(ThreadHandle& threadHandle);

XRCORE_API void CloseThreadHandle(ThreadHandle& threadHandle);

} // namespace Threading