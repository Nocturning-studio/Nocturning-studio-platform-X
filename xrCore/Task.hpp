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
//#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_ARM)
constexpr size_t RECOMMENDED_TASK_SIZE = 64; // bytes
//#elif defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_ARM64) || defined(XR_ARCHITECTURE_E2K)
//constexpr size_t RECOMMENDED_TASK_SIZE = 128; // bytes
//#else
//#error Determine your platform requirements
//#endif

#include "Noncopyable.hpp"

class XRCORE_API Task final : Noncopyable
{
	friend class TaskManager;
	friend class TaskAllocator;

  public:
	using TaskFunc = fastdelegate::FastDelegate<void(Task&, void*)>;
	using OnFinishFunc = fastdelegate::FastDelegate<void(const Task&, void*)>;

  private:
	// ordered from biggest to smallest
	TaskFunc m_task_func;
	OnFinishFunc m_on_finish_callback;
	pcstr m_name;
	Task* m_parent;
	std::atomic_int16_t m_jobs; // at least 1 (task itself), zero means task is done.

	static constexpr size_t all_size =
		sizeof(m_task_func) + sizeof(m_on_finish_callback) + sizeof(m_name) + sizeof(m_parent) + sizeof(m_jobs);
	u8 m_data[RECOMMENDED_TASK_SIZE - all_size];

  private:
	// Used by TaskAllocator as Task initial state
	Task();

	// Will just execute
	Task(pcstr name, const TaskFunc& task, void* data, size_t dataSize, Task* parent = nullptr);
	// Will execute and call back
	Task(pcstr name, const TaskFunc& task, const OnFinishFunc& onFinishCallback, void* data, size_t dataSize,
		 Task* parent = nullptr);

  public:
	Task* GetParent() const
	{
		return m_parent;
	}

	auto GetJobsCount() const
	{
		return m_jobs.load(std::memory_order_relaxed);
	}

	bool HasChildren() const
	{
		return GetJobsCount() > 1;
	}

	bool IsFinished() const
	{
		return 0 == m_jobs.load(std::memory_order_relaxed);
	}

  private:
	// Called by TaskManager
	void Execute();
	void Finish();
};