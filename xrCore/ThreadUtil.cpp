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

namespace Threading
{
void SetThreadNameImpl(pcstr name)
{
	string256 fullName;
	strconcat(sizeof(fullName), fullName, (const char*)"X-Ray ", name);

	using SetThreadDescriptionProc = decltype(&SetThreadDescription);
	static auto kernelHandle = GetModuleHandleA("kernel32.dll");
	static auto setThreadDescription =
		reinterpret_cast<SetThreadDescriptionProc>(GetProcAddress(kernelHandle, "SetThreadDescription"));

	if (setThreadDescription)
	{
		wchar_t buf[256];
		mbstowcs(buf, fullName, 256);

		setThreadDescription(GetCurrentThread(), buf);
	}
	else
		__try
		{
			constexpr DWORD MSVC_EXCEPTION = 0x406D1388;

			struct SThreadNameInfo
			{
				DWORD dwType{0x1000};
				LPCSTR szName{};
				DWORD dwThreadID{DWORD(-1)};
				DWORD dwFlags{};
			};

			SThreadNameInfo info;
			info.szName = fullName;

			RaiseException(MSVC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_CONTINUE_EXECUTION)
		{
		}
}

void SetCurrentThreadName(cpcstr name)
{
	SetThreadNameImpl(name);
#ifdef TRACY_ENABLE
	tracy::SetThreadName(name);
#endif
}

priority_level GetCurrentThreadPriorityLevel()
{
	switch (GetThreadPriority(GetCurrentThread()))
	{
	case THREAD_PRIORITY_IDLE:
		return priority_level::idle;
	case THREAD_PRIORITY_LOWEST:
		return priority_level::lowest;
	case THREAD_PRIORITY_BELOW_NORMAL:
		return priority_level::below_normal;
	default:
	case THREAD_PRIORITY_NORMAL:
		return priority_level::normal;
	case THREAD_PRIORITY_ABOVE_NORMAL:
		return priority_level::above_normal;
	case THREAD_PRIORITY_HIGHEST:
		return priority_level::highest;
	case THREAD_PRIORITY_TIME_CRITICAL:
		return priority_level::time_critical;
	}
}

priority_class GetCurrentProcessPriorityClass()
{
	switch (GetPriorityClass(GetCurrentProcess()))
	{
	case IDLE_PRIORITY_CLASS:
		return priority_class::idle;
	case BELOW_NORMAL_PRIORITY_CLASS:
		return priority_class::below_normal;
	default:
	case NORMAL_PRIORITY_CLASS:
		return priority_class::normal;
	case ABOVE_NORMAL_PRIORITY_CLASS:
		return priority_class::above_normal;
	case HIGH_PRIORITY_CLASS:
		return priority_class::high;
	case REALTIME_PRIORITY_CLASS:
		return priority_class::realtime;
	}
}

void SetCurrentThreadPriorityLevel(priority_level prio)
{
	int nPriority;
	switch (prio)
	{
	case priority_level::idle:
		nPriority = THREAD_PRIORITY_IDLE;
		break;
	case priority_level::lowest:
		nPriority = THREAD_PRIORITY_LOWEST;
		break;
	case priority_level::below_normal:
		nPriority = THREAD_PRIORITY_BELOW_NORMAL;
		break;
	default:
	case priority_level::normal:
		nPriority = THREAD_PRIORITY_NORMAL;
		break;
	case priority_level::above_normal:
		nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
		break;
	case priority_level::highest:
		nPriority = THREAD_PRIORITY_HIGHEST;
		break;
	case priority_level::time_critical:
		nPriority = THREAD_PRIORITY_TIME_CRITICAL;
		break;
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
}

void SetCurrentProcessPriorityClass(priority_class cls)
{
	DWORD dwPriorityClass;
	switch (cls)
	{
	case priority_class::idle:
		dwPriorityClass = IDLE_PRIORITY_CLASS;
		break;
	case priority_class::below_normal:
		dwPriorityClass = BELOW_NORMAL_PRIORITY_CLASS;
		break;
	default:
	case priority_class::normal:
		dwPriorityClass = NORMAL_PRIORITY_CLASS;
		break;
	case priority_class::above_normal:
		dwPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
		break;
	case priority_class::high:
		dwPriorityClass = HIGH_PRIORITY_CLASS;
		break;
	case priority_class::realtime:
		dwPriorityClass = REALTIME_PRIORITY_CLASS;
		break;
	}
	SetPriorityClass(GetCurrentProcess(), dwPriorityClass);
}
} // namespace Threading