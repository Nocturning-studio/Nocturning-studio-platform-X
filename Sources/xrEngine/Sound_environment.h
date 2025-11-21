///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment.h
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_common.h"
#include "Sound_environment_context.h"
#include "Sound_environment_geometry.h"
#include "Sound_environment_pathtracing.h"
#include "Sound_environment_calculations.h"
#include "Sound_environment_thread.h"
///////////////////////////////////////////////////////////////////////////////////

/**
 * @class CSoundEnvironment
 * @brief Main sound environment processor that analyzes 3D acoustic properties
 */
class ENGINE_API CSoundEnvironment
{
  private:
	SEAXEnvironmentData m_CurrentData;
	SEAXEnvironmentData m_PrevData;
	u32 m_LastUpdatedFrame;

	// »нтервал обновлени€ запроса.
	// ћожно ставить маленьким, так как поток сам разрулит нагрузку.
	static const u32 UPDATE_INTERVAL = 5;

	// === NEW THREAD SYSTEM ===
	CPathTracingThread m_Thread;
	EnvironmentContext m_Context; // ’раним контекст здесь

	// ћетоды дл€ внутренней логики (уже без Raycast)
	void ProcessNewData();

  public:
	CSoundEnvironment();
	~CSoundEnvironment();

	void Update();

	// ћетоды управлени€ жизненным циклом
	void OnLevelLoad();
	void OnLevelUnload();
};
///////////////////////////////////////////////////////////////////////////////////
