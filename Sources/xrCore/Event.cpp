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
#include "Event.hpp"
#include <windows.h>

Event::Event()
{
	handle = (void*)CreateEvent(NULL, FALSE, FALSE, NULL);
}

Event::~Event()
{
	CloseHandle(handle);
}

void Event::Reset()
{
	ResetEvent(handle);
}

void Event::Set()
{
	SetEvent(handle);
}

void Event::Wait() const
{
	WaitForSingleObject(handle, INFINITE);
}

bool Event::Wait(u32 millisecondsTimeout) const
{
	return WaitForSingleObject(handle, millisecondsTimeout) != WAIT_TIMEOUT;
}