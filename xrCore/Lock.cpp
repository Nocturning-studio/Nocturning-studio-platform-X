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
#include "Lock.hpp"
#include <mutex>

struct LockImpl
{
	CRITICAL_SECTION cs;

	LockImpl()
	{
		InitializeCriticalSection(&cs);
	}
	~LockImpl()
	{
		DeleteCriticalSection(&cs);
	}

	ICF void Lock()
	{
		EnterCriticalSection(&cs);
	}
	ICF void Unlock()
	{
		LeaveCriticalSection(&cs);
	}
	ICF bool TryLock()
	{
		return !!TryEnterCriticalSection(&cs);
	}
};

#ifdef CONFIG_PROFILE_LOCKS
static add_profile_portion_callback add_profile_portion = 0;
void set_add_profile_portion(add_profile_portion_callback callback)
{
	add_profile_portion = callback;
}
struct profiler
{
	u64 m_time;
	pcstr m_timer_id;

	IC profiler::profiler(pcstr timer_id)
	{
		if (!add_profile_portion)
			return;

		m_timer_id = timer_id;
		m_time = CPU::QPC();
	}

	IC profiler::~profiler()
	{
		if (!add_profile_portion)
			return;

		u64 time = CPU::QPC();
		(*add_profile_portion)(m_timer_id, time - m_time);
	}
};

Lock::Lock(const char* id) : impl(xr_new<LockImpl>()), lockCounter(0), id(id)
{
}

void Lock::Enter()
{
#if 0  // def DEBUG
    static bool show_call_stack = false;
    if (show_call_stack)
        OutputDebugStackTrace("----------------------------------------------------");
#endif // DEBUG
	profiler temp(id);
	mutex.lock();
	isLocked = true;
}
#else
Lock::Lock() : impl(xr_new<LockImpl>()), lockCounter(0)
{
}

Lock::~Lock()
{
	xr_delete(impl);
}

Lock::Lock(Lock&& other) noexcept(false)
{
	xr_delete(impl);
	impl = other.impl;
	lockCounter.store(other.lockCounter.load(std::memory_order_acquire), std::memory_order_release);
	other.impl = xr_new<LockImpl>();
	other.lockCounter.store(0, std::memory_order_release);
}

Lock& Lock::operator=(Lock&& other) noexcept(false)
{
	xr_delete(impl);
	impl = other.impl;
	lockCounter.store(other.lockCounter.load(std::memory_order_acquire), std::memory_order_release);
	other.impl = xr_new<LockImpl>();
	other.lockCounter.store(0, std::memory_order_release);
	return *this;
}

void Lock::Enter()
{
	impl->Lock();
	lockCounter.fetch_add(1, std::memory_order_acq_rel);
}
#endif // CONFIG_PROFILE_LOCKS

bool Lock::TryEnter()
{
	const bool locked = impl->TryLock();
	if (locked)
		lockCounter.fetch_add(1, std::memory_order_acq_rel);
	return locked;
}

void Lock::Leave()
{
	impl->Unlock();
	lockCounter.fetch_sub(1, std::memory_order_acq_rel);
}

#ifdef DEBUG
extern void OutputDebugStackTrace(const char* header);
#endif