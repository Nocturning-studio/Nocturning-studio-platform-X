///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_thread.cpp
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "Sound_environment_thread.h"
#include "Sound_environment_common.h"

// Включаем необходимые хедеры движка для RayPick
#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "igame_level.h"
#include "xr_area.h"
#include "xr_object.h"
#include "xr_collide_defs.h"
#endif

// Массив направлений должен быть виден здесь
extern const Fvector3 DetailedSphereDirections[];

CPathTracingThread::CPathTracingThread() : m_pThread(nullptr)
{
	m_bStop = false;
	m_bWorkPending = false;
	m_bResultReady = false;
	m_bIsCalculating = false;
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
	Msg("[SoundEnv] Async Path Tracing thread stopped.");
}

void CPathTracingThread::RequestUpdate(const Fvector& cameraPos)
{
	// Если поток уже занят вычислениями, мы просто обновляем целевую позицию,
	// но не сбрасываем текущий расчет (или можно игнорировать, если занят).
	// В данном случае - просто обновляем таргет.

	std::lock_guard<std::mutex> lock(m_Mutex);
	m_TargetPos = cameraPos;
	m_bWorkPending = true;
	m_cv.notify_one(); // Будим поток
}

bool CPathTracingThread::GetResult(EnvironmentContext& out_context)
{
	if (!m_bResultReady)
		return false;

	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_bResultReady)
	{
		// Копируем данные из потока в игровой контекст
		// ВАЖНО: EAXData считать здесь не надо, передаем только "сырые" данные лучей
		out_context.RaycastDistances = m_ResultContext.RaycastDistances;
		out_context.TotalHits = m_ResultContext.TotalHits;
		out_context.TotalDistance = m_ResultContext.TotalDistance;

		for (int i = 0; i < 6; ++i)
		{
			out_context.MaterialHits[i] = m_ResultContext.MaterialHits[i];
			out_context.MaterialDistances[i] = m_ResultContext.MaterialDistances[i];
		}

		// Копируем позицию, для которой это было посчитано
		out_context.CameraPosition = m_ResultContext.CameraPosition;

		m_bResultReady = false;
		return true;
	}
	return false;
}

void CPathTracingThread::ThreadProc()
{
	while (true)
	{
		Fvector currentPos;

		{
			// Ждем задачу
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_cv.wait(lock, [this] { return m_bWorkPending || m_bStop; });

			if (m_bStop)
				break;

			// Забираем задачу
			currentPos = m_TargetPos;
			m_bWorkPending = false;
			m_bIsCalculating = true;
		}

		// === ТЯЖЕЛЫЕ ВЫЧИСЛЕНИЯ (ВНЕ LOCK'а) ===

		// Создаем локальный контекст для расчетов
		EnvironmentContext localCtx;
		localCtx.Reset();
		localCtx.CameraPosition = currentPos;

		// Выполняем трассировку
		PerformAsyncTrace(localCtx);

		// === ЗАПИСЬ РЕЗУЛЬТАТА ===
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			// Перемещаем результаты в член класса
			m_ResultContext = localCtx;
			m_bResultReady = true;
			m_bIsCalculating = false;
		}
	}
}

float CPathTracingThread::PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type)
{
	// Эту функцию мы просто копируем или переносим из Sound_environment.cpp
	// ВАЖНО: g_pGameLevel->ObjectSpace.RayPick для STATIC геометрии безопасен.
	// Нельзя делать RayPick для DYNAMIC объектов без блокировки всего движка.

	collide::rq_result hit;

	// В потоке мы не можем брать CurrentViewEntity для игнора, передаем NULL.
	// RayPick по статике (rqtStatic) игнорирует динамические объекты и так.
	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, hit, NULL);

	if (bHit && hit.range > 0.1f)
	{
		// Получение треугольника для нормали тоже безопасно для статики (CDB read-only)
		Fvector hit_normal;
		CDB::TRI* tri = g_pGameLevel->ObjectSpace.GetStaticTris() + hit.element;
		Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
		hit_normal.mknormal(verts[tri->verts[0]], verts[tri->verts[1]], verts[tri->verts[2]]);

		float cos_angle = _abs(dir.dotproduct(hit_normal));

		// Логика материалов (та же самая)
		if (hit.range < 2.0f)
		{
			if (cos_angle > 0.9f)
				material_type = MATERIAL_STONE;
			else if (cos_angle > 0.7f)
				material_type = MATERIAL_METAL;
			else
				material_type = MATERIAL_WOOD;
		}
		else if (hit.range < 10.0f)
		{
			if (cos_angle > 0.8f)
				material_type = MATERIAL_STONE;
			else if (cos_angle > 0.5f)
				material_type = MATERIAL_WOOD;
			else
				material_type = MATERIAL_SOFT;
		}
		else if (hit.range < 50.0f)
		{
			if (cos_angle > 0.6f)
				material_type = MATERIAL_STONE;
			else
				material_type = MATERIAL_WOOD;
		}
		else
		{
			if (hit.range < max_dist * 0.9f)
				material_type = MATERIAL_STONE;
			else
				material_type = MATERIAL_GLASS; // Sky
		}
		return hit.range;
	}

	material_type = MATERIAL_AIR;
	return max_dist;
}

// Константы для трассировки
static const int MAX_BOUNCES = 4;		  // Глубина рекурсии
static const float RAY_ENERGY_MIN = 0.1f; // Если энергия ниже, прекращаем луч

// Вспомогательная структура для возврата полных данных о попадании
struct STraceHitResult
{
	bool bHit;
	float dist;
	u32 material;
	Fvector position;
	Fvector normal;
};

// Расширенная функция Raycast, которая возвращает нормаль и позицию
// ВАЖНО: Это всё еще thread-safe для статической геометрии (rqtStatic)
STraceHitResult CastRayInternal(Fvector start, Fvector dir, float range)
{
	STraceHitResult res;
	res.bHit = false;
	res.dist = range;
	res.material = MATERIAL_AIR;
	res.normal.set(0, 1, 0); // Default up

	collide::rq_result RQ;
	// Трассируем только СТАТИКУ (стены), игнорируем динамику для скорости и стабильности потока
	BOOL hit = g_pGameLevel->ObjectSpace.RayPick(start, dir, range, collide::rqtStatic, RQ, NULL);

	if (hit && RQ.range > 0.05f) // Игнорируем слишком близкие (self-intersection)
	{
		res.bHit = true;
		res.dist = RQ.range;

		// Вычисляем позицию удара
		res.position.mad(start, dir, RQ.range);

		// === ПОЛУЧЕНИЕ НОРМАЛИ (ГЕОМЕТРИЯ) ===
		CDB::TRI* tri = g_pGameLevel->ObjectSpace.GetStaticTris() + RQ.element;
		Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
		// Считаем нормаль треугольника
		res.normal.mknormal(verts[tri->verts[0]], verts[tri->verts[1]], verts[tri->verts[2]]);

		// === ОПРЕДЕЛЕНИЕ МАТЕРИАЛА ===
		// Используем угол падения для примитивной эвристики материала
		// (В идеале тут нужно читать GameMtl из шейдера треугольника, но это сложнее)
		float cos_angle = _abs(dir.dotproduct(res.normal));

		if (RQ.range < 5.0f)
		{
			if (cos_angle > 0.8f)
				res.material = MATERIAL_STONE;
			else if (cos_angle > 0.5f)
				res.material = MATERIAL_WOOD;
			else
				res.material = MATERIAL_SOFT;
		}
		else
		{
			res.material = MATERIAL_STONE; // Далекие стены обычно бетон/камень
		}
	}

	return res;
}

void CPathTracingThread::PerformAsyncTrace(EnvironmentContext& ctx)
{
	// Сбрасываем счетчики
	ctx.TotalHits = 0;
	ctx.TotalDistance = 0;

	// Коэффициенты поглощения для материалов (сколько энергии ОСТАЕТСЯ)
	// Stone=0.7 (отражает), Soft=0.2 (поглощает)
	const float MatReflectionCoeff[] = {1.0f, 0.7f, 0.8f, 0.5f, 0.2f, 0.1f};

	// Проходим по всем первичным направлениям
	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		if (m_bStop)
			return; // Быстрый выход

		Fvector rayPos = ctx.CameraPosition;
		Fvector rayDir = DetailedSphereDirections[i];
		float currentEnergy = 1.0f;
		float rayTotalDist = 0.0f;

		// === РЕКУРСИВНЫЙ ОТСКОК (ITERATIVE LOOP) ===
		for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce)
		{
			// Стреляем лучом
			// Для первого отскока (bounce 0) range большой, для вторичных - поменьше
			float max_range = (bounce == 0) ? SMART_RAY_DISTANCE : (SMART_RAY_DISTANCE * 0.5f);

			STraceHitResult hit = CastRayInternal(rayPos, rayDir, max_range);

			// Добавляем дистанцию к общей длине пути этого луча
			rayTotalDist += hit.dist;

			if (hit.bHit)
			{
				// Если это ПЕРВЫЙ удар - это прямой замер габаритов комнаты
				if (bounce == 0)
				{
					ctx.RaycastDistances.push_back(hit.dist);
					ctx.TotalHits++; // Считаем попадание в геометрию
				}

				// Записываем материал (с весом энергии)
				if (hit.material < 6)
				{
					// Чем больше bounce, тем меньше вклад в статистику материалов
					// (но мы все равно учитываем, что звук "нашел" дерево)
					ctx.MaterialHits[hit.material]++;
					ctx.MaterialDistances[hit.material] += hit.dist;
				}

				// === РАСЧЕТ ОТРАЖЕНИЯ ===
				// R = I - 2(N.I)N
				float dot = rayDir.dotproduct(hit.normal);
				Fvector reflectDir;
				reflectDir.mad(rayDir, hit.normal, -2.0f * dot);
				reflectDir.normalize_safe();

				// Сдвигаем старт чуть вперед по нормали, чтобы не зацепить сам треугольник (Bias)
				rayPos.mad(hit.position, hit.normal, 0.05f);
				rayDir = reflectDir;

				// Уменьшаем энергию луча в зависимости от материала
				float reflFactor = MatReflectionCoeff[hit.material];
				currentEnergy *= reflFactor;

				// Если энергии слишком мало - луч затух
				if (currentEnergy < RAY_ENERGY_MIN)
					break;
			}
			else
			{
				// Луч улетел в небо (Sky)
				if (bounce == 0)
				{
					// Записываем как длинный луч
					ctx.RaycastDistances.push_back(max_range);
				}
				break; // Дальше отражаться не от чего
			}
		}

		// Добавляем суммарную длину пути луча (с учетом отскоков) в общую статистику
		// Это позволяет оценить реальный "объем" звукового поля, а не просто расстояние до стен
		ctx.TotalDistance += rayTotalDist;
	}

	// Нормализуем TotalDistance для совместимости со старым кодом
	// (так как теперь TotalDistance включает вторичные лучи и будет огромным)
	// Или можно использовать это новое значение для расчета "сложности" (Complexity) помещения.
}
