///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_thread.h
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Sound_environment_context.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class CPathTracingThread
{
  public:
	CPathTracingThread();
	~CPathTracingThread();

	void Start();
	void Stop();
	void RequestUpdate(const Fvector& cameraPos);
	bool GetResult(EnvironmentContext& out_context);
	bool IsCalculating() const
	{
		return m_bIsCalculating;
	}

  private:
	void ThreadProc();

  private:
	std::thread* m_pThread;
	std::mutex m_Mutex;
	std::condition_variable m_cv;

	std::atomic<bool> m_bStop;
	std::atomic<bool> m_bWorkPending;
	std::atomic<bool> m_bResultReady;
	std::atomic<bool> m_bIsCalculating;

	Fvector m_TargetPos;
	Fvector m_LastProcessedPos;

	EnvironmentContext m_ResultContext;
	EnvironmentContext m_AccumContext;
};
