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

#include "xrCore.h"

#include <process.h>

#include <thread>

namespace Threading
{
enum class priority_class
{
	idle,
	below_normal,
	normal,
	above_normal,
	high,
	realtime,
};

enum class priority_level
{
	idle,
	lowest,
	below_normal,
	normal,
	above_normal,
	highest,
	time_critical,
};

XRCORE_API priority_level GetCurrentThreadPriorityLevel();
XRCORE_API priority_class GetCurrentProcessPriorityClass();

XRCORE_API void SetCurrentThreadPriorityLevel(priority_level prio);
XRCORE_API void SetCurrentProcessPriorityClass(priority_class cls);

XRCORE_API void SetCurrentThreadName(cpcstr name);

template <typename Invocable, typename... Args>
[[nodiscard]] std::thread RunThread(cpcstr name, Invocable&& invocable, Args&&... args)
{
	return std::move(std::thread{[name](Invocable&& invocable2, Args&&... args2) {
									 SetCurrentThreadName(name);
									 _initialize_cpu_thread();
									 std::invoke(std::move(invocable2), std::move(args2)...);
								 },
								 std::forward<Invocable>(invocable), std::forward<Args>(args)...});
}

//template <typename... Args> void SpawnThread(Args&&... args)
//{
//	RunThread(std::forward<Args>(args)...).detach();
//}

template <typename Invocable, typename... Args> 
void SpawnThread(Invocable&& invocable, cpcstr name, Args&&... args)
{
	RunThread(name, invocable, std::forward<Args>(args)...).detach();
}
} // namespace Threading