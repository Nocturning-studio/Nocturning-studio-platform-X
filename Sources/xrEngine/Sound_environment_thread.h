///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_thread.h
// Author: NSDeathman
// Asynchronous Path Tracing Worker
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

	// Запуск и остановка потока (вызывать при старте/конце уровня)
	void Start();
	void Stop();

	// Главный поток вызывает это, чтобы попросить обновление
	void RequestUpdate(const Fvector& cameraPos);

	// Главный поток вызывает это, чтобы проверить, готовы ли данные
	// Возвращает true, если новые данные были записаны в out_context
	bool GetResult(EnvironmentContext& out_context);

	bool IsCalculating() const
	{
		return m_bIsCalculating;
	}

  private:
	// Функция, которая крутится в потоке
	void ThreadProc();

	// Внутренняя логика трассировки (перенесена из Sound_environment)
	void PerformAsyncTrace(EnvironmentContext& ctx);
	float PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type);

  private:
	std::thread* m_pThread;
	std::mutex m_Mutex;
	std::condition_variable m_cv;

	// Флаги состояния
	std::atomic<bool> m_bStop;			// Пора убивать поток
	std::atomic<bool> m_bWorkPending;	// Есть новая задача
	std::atomic<bool> m_bResultReady;	// Результат готов
	std::atomic<bool> m_bIsCalculating; // Прямо сейчас считаем

	// Данные для обмена
	Fvector m_TargetPos;				// Входные данные (позиция)
	EnvironmentContext m_ResultContext; // Выходные данные (результат)
};
