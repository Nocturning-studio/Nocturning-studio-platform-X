#include "stdafx.h"
#pragma hdrstop
#include "Sound_environment.h"
#include "Sound_environment_calculations.h"

// Интерполяция (SmoothEAXData) остается здесь или в common

CSoundEnvironment::CSoundEnvironment() : m_LastUpdatedFrame(0)
{
	ZeroMemory(&m_CurrentData, sizeof(m_CurrentData));
	ZeroMemory(&m_PrevData, sizeof(m_PrevData));
	m_CurrentData.Reset();
	m_Context.Reset();
}

CSoundEnvironment::~CSoundEnvironment()
{
	// Обязательно стопаем поток при удалении
	m_Thread.Stop();
}

void CSoundEnvironment::OnLevelLoad()
{
	m_Thread.Start();
}

void CSoundEnvironment::OnLevelUnload()
{
	// Критично остановить поток ДО выгрузки геометрии уровня
	m_Thread.Stop();
}

// Вспомогательная функция для линейной интерполяции
template <typename T> T EAXLerp(T current, T target, float step)
{
	// Защита от перелета (overshoot)
	float diff = (float)(target - current);
	if (_abs(diff) < 0.01f)
		return target; // Близко? Ставим сразу
	return (T)(current + diff * step);
}

void SmoothEAXData(SEAXEnvironmentData& current, const SEAXEnvironmentData& target, float speed_factor)
{
	// speed_factor: 0.0 (нет изменений) ... 1.0 (мгновенно)
	// Рекомендуемое значение: 0.1f - 0.2f (для плавности за 5-10 кадров)

	// 1. Громкость (Room, Refl, Reverb) - интерполируем как Long
	current.lRoom = EAXLerp(current.lRoom, target.lRoom, speed_factor);
	current.lRoomHF = EAXLerp(current.lRoomHF, target.lRoomHF, speed_factor);
	current.lReflections = EAXLerp(current.lReflections, target.lReflections, speed_factor);
	current.lReverb = EAXLerp(current.lReverb, target.lReverb, speed_factor);

	// 2. Временные параметры (Decay, Delay) - интерполируем как Float
	current.flDecayTime = EAXLerp(current.flDecayTime, target.flDecayTime, speed_factor);
	current.flDecayHFRatio = EAXLerp(current.flDecayHFRatio, target.flDecayHFRatio, speed_factor);
	current.flReflectionsDelay = EAXLerp(current.flReflectionsDelay, target.flReflectionsDelay, speed_factor);
	current.flReverbDelay = EAXLerp(current.flReverbDelay, target.flReverbDelay, speed_factor);

	// 3. Параметры среды
	current.flEnvironmentSize = EAXLerp(current.flEnvironmentSize, target.flEnvironmentSize, speed_factor);
	current.flEnvironmentDiffusion =
		EAXLerp(current.flEnvironmentDiffusion, target.flEnvironmentDiffusion, speed_factor);
	current.flAirAbsorptionHF = EAXLerp(current.flAirAbsorptionHF, target.flAirAbsorptionHF, speed_factor);

	// 4. Флаги и прочее
	current.dwFlags = target.dwFlags;						  // Флаги меняем мгновенно
	current.flRoomRolloffFactor = target.flRoomRolloffFactor; // Rolloff тоже лучше сразу
}

void CSoundEnvironment::ProcessNewData()
{
	// 1. Данные лучей уже в m_Context (заполнил поток)

	// 2. Выполняем быструю математику (доли миллисекунды) в главном потоке
	EnvironmentCalculations::CalculateEnvironmentalProperties(m_Context);
	EnvironmentCalculations::CalculateEAXParameters(m_Context);

	// 3. Валидация и Финализация
	if (!EnvironmentCalculations::ValidatePhysicalContext(m_Context))
	{
		m_Context.EAXData.Reset();
	}

	m_Context.EAXData.dwFrameStamp = Device.dwFrame;
	m_Context.EAXData.bDataValid = true;

	// 4. Интерполяция (сглаживание)
	// Если это первый запуск - сразу ставим, иначе плавно
	if (m_CurrentData.lRoom <= -9000)
		m_CurrentData.CopyFrom(m_Context.EAXData);
	else
		SmoothEAXData(m_CurrentData, m_Context.EAXData, 0.5f);

	m_PrevData.CopyFrom(m_CurrentData);

	// 5. Отправка в OpenAL
	if (::Sound)
		::Sound->commit_eax(&m_CurrentData);
}

void CSoundEnvironment::Update()
{
	if (!psSoundFlags.test(ss_EAX))
		return;

	// 1. ПРОВЕРКА: Готов ли результат от потока?
	// GetResult вернет true только если поток закончил работу
	if (m_Thread.GetResult(m_Context))
	{
		// Ура, свежие данные лучей! Обрабатываем.
		ProcessNewData();
		m_LastUpdatedFrame = Device.dwFrame;
	}

	// 2. ЗАПРОС: Нужно ли запустить новый расчет?
	// Если поток свободен И прошло время с последнего обновления
	if (!m_Thread.IsCalculating() && Device.dwFrame > (m_LastUpdatedFrame + UPDATE_INTERVAL))
	{
		// Просто кидаем задачу в поток и забываем.
		// Это не блокирует игру.
		m_Thread.RequestUpdate(Device.vCameraPosition);
	}
}
