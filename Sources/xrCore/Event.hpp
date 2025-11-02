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

class XRCORE_API Event
{
  private:
	void* handle;

  public:
	Event();
	~Event();

	// Reset the event to the unsignalled state.
	void Reset();
	// Set the event to the signalled state.
	void Set();
	// Wait indefinitely for the object to become signalled.
	void Wait() const;
	// Wait, with a time limit, for the object to become signalled.
	bool Wait(u32 millisecondsTimeout) const;

	void* GetHandle() const
	{
		return handle;
	}
};