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
#include <atomic>

#include "Noncopyable.hpp"

#include "xrCore.h"

#ifdef CONFIG_PROFILE_LOCKS
typedef void (*add_profile_portion_callback)(pcstr id, const u64& time);
void XRCORE_API set_add_profile_portion(add_profile_portion_callback callback);

#define MUTEX_PROFILE_PREFIX_ID #mutexes /
#define MUTEX_PROFILE_ID(a) MACRO_TO_STRING(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID, a))
#endif // CONFIG_PROFILE_LOCKS

class XRCORE_API Lock
{
	struct LockImpl* impl{};

  public:
#ifdef CONFIG_PROFILE_LOCKS
	Lock(const char* id);
#else
	Lock();
#endif
	~Lock();

	Lock(Lock& other) = delete;
	Lock& operator=(Lock& other) = delete;

	Lock(Lock&& other) noexcept(false);
	Lock& operator=(Lock&& other) noexcept(false);

#ifdef CONFIG_PROFILE_LOCKS
	void Enter();
#else
	void Enter();
#endif

	bool TryEnter();

	void Leave();

	bool IsLocked() const
	{
		return !!lockCounter;
	}

  private:
	std::atomic_int lockCounter;
#ifdef CONFIG_PROFILE_LOCKS
	const char* id;
#endif
};