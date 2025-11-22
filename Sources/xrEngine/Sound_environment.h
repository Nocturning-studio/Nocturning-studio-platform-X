///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment.h
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Sound_environment_common.h"
#include "Sound_environment_context.h"
#include "Sound_environment_thread.h"
#include "Sound_environment_calculations.h"

class ENGINE_API CSoundEnvironment
{
  private:
	SEAXEnvironmentData m_CurrentData;
	SEAXEnvironmentData m_PrevData;
	u32 m_LastUpdatedFrame;
	static const u32 UPDATE_INTERVAL = 3;

	CPathTracingThread m_Thread;
	EnvironmentContext m_Context;
	float m_LastEnclosedness;

	void ProcessNewData();

  public:
	CSoundEnvironment();
	~CSoundEnvironment();

	void Update();
	void OnLevelLoad();
	void OnLevelUnload();
};
