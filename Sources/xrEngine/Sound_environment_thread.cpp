///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_thread.cpp
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
#include "Sound_environment_thread.h"
#include "Sound_environment_pathtracing.h"

extern const u32 DETAILED_DIRECTIONS_COUNT;

CPathTracingThread::CPathTracingThread() : m_pThread(nullptr)
{
	m_bStop = false;
	m_bWorkPending = false;
	m_bResultReady = false;
	m_bIsCalculating = false;
	m_LastProcessedPos.set(0, 0, 0);
}

CPathTracingThread::~CPathTracingThread()
{
	Stop();
}

void CPathTracingThread::Start()
{
	if (m_pThread)
		return;
	m_bStop = false;
	m_AccumContext.ResetAccumulation();
	m_pThread = new std::thread(&CPathTracingThread::ThreadProc, this);
	Msg("[SoundEnv] Async Path Tracing thread started.");
}

void CPathTracingThread::Stop()
{
	if (!m_pThread)
		return;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_bStop = true;
	}
	m_cv.notify_one();
	if (m_pThread->joinable())
		m_pThread->join();
	delete m_pThread;
	m_pThread = nullptr;
}

void CPathTracingThread::RequestUpdate(const Fvector& cameraPos)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_TargetPos = cameraPos;
	m_bWorkPending = true;
	m_cv.notify_one();
}

bool CPathTracingThread::GetResult(EnvironmentContext& out_context)
{
	if (!m_bResultReady)
		return false;
	std::lock_guard<std::mutex> lock(m_Mutex);

	if (m_bResultReady)
	{
		out_context.TotalHits = m_ResultContext.TotalHits;
		out_context.TotalDistance = m_ResultContext.TotalDistance;
		out_context.CameraPosition = m_ResultContext.CameraPosition;
		for (int i = 0; i < 6; ++i)
			out_context.MaterialHits[i] = m_ResultContext.MaterialHits[i];

		out_context.RaycastDistances.clear();
		out_context.RaycastDistances.push_back(out_context.TotalDistance /
											   _max(1.0f, (float)DETAILED_DIRECTIONS_COUNT));

		m_bResultReady = false;
		return true;
	}
	return false;
}

void CPathTracingThread::ThreadProc()
{
	CPathTracingSystem Tracer;

	while (true)
	{
		Fvector CurrentPos;
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_cv.wait(lock, [this] { return m_bWorkPending || m_bStop; });
			if (m_bStop)
				break;
			CurrentPos = m_TargetPos;
			m_bWorkPending = false;
			m_bIsCalculating = true;
		}

		float dist_moved = m_LastProcessedPos.distance_to(CurrentPos);

		// —брос аккумул€ции при движении
		if (dist_moved > 0.5f)
		{
			m_AccumContext.ResetAccumulation();
			m_LastProcessedPos = CurrentPos;
		}

		m_AccumContext.CameraPosition = CurrentPos;

		Tracer.PerformPathTracingAnalysis(m_AccumContext);
		m_AccumContext.Accum_FrameCount++;

		// ”среднение
		EnvironmentContext TempResult;
		float div = (float)m_AccumContext.Accum_FrameCount;

		if (div > 0.0f)
		{
			TempResult.TotalDistance = m_AccumContext.Accum_TotalDistance / div;
			TempResult.TotalHits = m_AccumContext.Accum_TotalHits / div;
			for (int i = 0; i < 6; ++i)
				TempResult.MaterialHits[i] = (u32)((float)m_AccumContext.Accum_MaterialHits[i] / div);

			TempResult.CameraPosition = CurrentPos;
		}

		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_ResultContext = TempResult;
			m_bResultReady = true;
			m_bIsCalculating = false;
		}

		if (m_AccumContext.Accum_FrameCount > 20)
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}
