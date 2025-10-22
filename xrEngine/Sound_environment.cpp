///////////////////////////////////////////////////////////////////////////////////
// Created: 10.12.2024
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "igame_level.h"
#include "igame_persistent.h"
#include "xr_area.h"
#include "xr_object.h"
#include "xr_collide_defs.h"
#endif

#include "Sound_environment.h"

///////////////////////////////////////////////////////////////////////////////////

// Статические константы для физической модели
const float CSoundEnvironment::CPhysicalAcousticModel::SOUND_SPEED = 340.0f;
const float CSoundEnvironment::CPhysicalAcousticModel::AIR_ABSORPTION_COEF = 0.001f;

CSoundEnvironment::CSoundEnvironment()
{
	ZeroMemory(&m_CurrentData, sizeof(m_CurrentData));
	ZeroMemory(&m_PrevData, sizeof(m_PrevData));
	m_LastUpdatedFrame = 0;
	m_CurrentData.Reset();
}

CSoundEnvironment::~CSoundEnvironment()
{
}

///////////////////////////////////////////////////////////////////////////////////

float CSoundEnvironment::PerformSmartRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type)
{
	collide::rq_result hit;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, hit, ViewEntity);

	if (bHit && hit.range > 0.1f)
	{
		// Определяем тип материала по расстоянию (упрощенно)
		if (hit.range < 5.0f)
			material_type = 1; // Близкие поверхности - высокое отражение
		else if (hit.range < 15.0f)
			material_type = 2; // Средние расстояния
		else
			material_type = 3; // Далёкие поверхности

		return hit.range;
	}

	// Если луч не попал - возвращаем максимальное расстояние
	material_type = 0; // Воздух
	return max_dist;
}

float CSoundEnvironment::CPhysicalAcousticModel::CalculateEchoStrength(float volume, float reflectivity, float openness)
{
	// Нелинейная зависимость эха от объема помещения
	// В маленьких помещениях эхо быстро затухает, в больших - накапливается

	float normalizedVolume = volume / 1000.0f; // Нормализуем к 1000 м³
	clamp(normalizedVolume, 0.1f, 10.0f);

	// Логарифмическая зависимость - эхо растет медленнее объема
	float logVolume = log10f(normalizedVolume + 1.0f);

	// Пороговая функция - эхо значительно усиливается только после определенного объема
	float thresholdVolume = 2.0f; // 2000 м³
	float volumeFactor = 0.0f;

	if (normalizedVolume < thresholdVolume)
	{
		// Маленькие помещения - слабое эхо
		volumeFactor = normalizedVolume / thresholdVolume * 0.3f;
	}
	else
	{
		// Большие помещения - сильное эхо (но с насыщением)
		volumeFactor = 0.3f + (1.0f - 0.3f) * (1.0f - expf(-(normalizedVolume - thresholdVolume) / 2.0f));
	}

	// Учитываем отражательную способность
	float echoStrength = volumeFactor * reflectivity;

	// Дополнительное затухание для очень больших объемов (открытые пространства)
	if (normalizedVolume > 5.0f)
	{
		echoStrength *= 0.1f; // Сильное затухание в открытых пространствах
	}

	// Учитываем открытость пространства
	echoStrength *= (1.0f - openness * 0.5f);

	clamp(echoStrength, 0.0f, 1.0f);
	return echoStrength;
}

float CSoundEnvironment::CPhysicalAcousticModel::CalculateReverbTime(float volume, float absorption)
{
	// Упрощенная формула Сабине: T60 = 0.161 * V / A
	if (absorption < 0.01f)
		absorption = 0.01f;
	float reverbTime = (0.161f * volume) / absorption;

	// Ограничиваем разумными пределами
	clamp(reverbTime, 0.1f, 8.0f);
	return reverbTime;
}

float CSoundEnvironment::CPhysicalAcousticModel::CalculateCriticalDistance(float volume)
{
	// Критическое расстояние зависит от объема помещения
	float criticalDist = 2.0f + (volume / 500.0f);
	clamp(criticalDist, 2.0f, 20.0f);
	return criticalDist;
}

///////////////////////////////////////////////////////////////////////////////////

float CSoundEnvironment::PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type,
												bool bCollectInfo)
{
	collide::rq_result hit;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, hit, ViewEntity);

	if (bHit && hit.range > 0.1f)
	{
		// Улучшенное определение материала на основе расстояния и угла
		float distance_factor = hit.range / max_dist;
		float angle_factor = 1.0f - fabs(dir.y); // Горизонтальные поверхности более отражающие

		if (hit.range < 3.0f)
			material_type = 1; // Близкие поверхности - высокое отражение
		else if (hit.range < 10.0f)
			material_type = 2; // Средние расстояния
		else if (hit.range < 25.0f)
			material_type = 3; // Далёкие поверхности
		else
			material_type = 4; // Очень далекие поверхности

		return hit.range;
	}

	material_type = 0; // Воздух/небо
	return max_dist;
}

void CSoundEnvironment::AnalyzeSpatialDistribution(Fvector center, std::vector<float>& distances,
												   SEAXEnvironmentData& result)
{
	// Анализ пространственного распределения
	float min_dist = 10000.0f, max_dist = 0.0f;
	float sum = 0.0f, sum_sq = 0.0f;

	for (u32 i = 0; i < distances.size(); ++i)
	{
		float dist = distances[i];
		min_dist = _min(min_dist, dist);
		max_dist = _max(max_dist, dist);
		sum += dist;
		sum_sq += dist * dist;
	}

	float mean = sum / distances.size();
	float variance = (sum_sq / distances.size()) - (mean * mean);
	float std_dev = sqrtf(variance);

	// Классификация типа пространства на основе распределения
	float coherence = 1.0f - (std_dev / mean);
	clamp(coherence, 0.0f, 1.0f);

	// Определение типа геометрии
	if (max_dist - min_dist < 5.0f && coherence > 0.8f)
	{
		// Регулярное помещение (комната)
		result.fRoomSize = mean / 30.0f;
	}
	else if (std_dev > mean * 0.5f)
	{
		// Сложная геометрия (коридоры, неправильная форма)
		result.fRoomSize = (mean + std_dev) / 40.0f;
	}
	else
	{
		// Открытое пространство
		result.fRoomSize = mean / 100.0f;
	}

	clamp(result.fRoomSize, 0.05f, 1.0f);
}

void CSoundEnvironment::CalculateEnhancedEnvironmentData()
{
	SEAXEnvironmentData newData;

	Fvector RayPosition = Device.vCameraPosition;

	// Path tracing анализ
	m_PathTracer.PerformPathTracingAnalysis(RayPosition, m_PathTracingResult);

	// Основной анализ геометрии
	m_GeometryAnalyzer.AnalyzeEnvironmentGeometry(RayPosition, m_CurrentGeometry);

	std::vector<float> distances;
	distances.reserve(DETAILED_DIRECTIONS_COUNT);

	u32 TotalHits = 0;
	u32 CloseHits = 0;
	u32 FarHits = 0;
	float TotalDistance = 0.0f;

	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		Fvector3 RayDirection = DetailedSphereDirections[i];
		u32 material_type = 0;

		float distance = PerformDetailedRaycast(RayPosition, RayDirection, SMART_RAY_DISTANCE, material_type, true);

		distances.push_back(distance);
		TotalDistance += distance;

		if (material_type > 0)
		{
			TotalHits++;
			if (distance < 3.0f) // Уменьшили порог для близких попаданий
				CloseHits++;
			if (distance > 15.0f) // Уменьшили порог для далеких попаданий
				FarHits++;
		}
	}

	// ОСНОВНЫЕ ПАРАМЕТРЫ
	newData.fEnvironmentRadius = TotalDistance / DETAILED_DIRECTIONS_COUNT;
	newData.fEnvironmentDensity = float(TotalHits) / DETAILED_DIRECTIONS_COUNT;

	// ПЕРЕСМОТРЕННАЯ ФОРМУЛА ОТКРЫТОСТИ
	float basic_openness = float(FarHits) / DETAILED_DIRECTIONS_COUNT;
	float path_tracing_openness = 1.0f - (m_PathTracingResult.fTotalEnergy * 5.0f); // Усилили влияние
	float geometry_openness = 1.0f - (m_CurrentGeometry.fComplexity * 0.2f + newData.fEnvironmentDensity * 0.8f);

	// Комбинируем с весами в пользу открытости
	newData.fOpenness = (basic_openness * 0.4f + path_tracing_openness * 0.4f + geometry_openness * 0.2f);
	clamp(newData.fOpenness, 0.0f, 1.0f);

	// ПЕРЕСМОТРЕННАЯ ФОРМУЛА ЗАКРЫТОСТИ
	float basic_enclosedness = float(CloseHits) / DETAILED_DIRECTIONS_COUNT;
	float path_tracing_enclosedness = m_PathTracingResult.fTotalEnergy * 2.0f; // Уменьшили влияние
	float geometry_enclosedness = m_CurrentGeometry.fComplexity * 0.3f + newData.fEnvironmentDensity * 0.7f;

	newData.fEnclosedness =
		(basic_enclosedness * 0.4f + path_tracing_enclosedness * 0.3f + geometry_enclosedness * 0.3f);
	clamp(newData.fEnclosedness, 0.0f, 1.0f);

	// УБЕЖДАЕМСЯ В КОРРЕКТНОСТИ ПРОТИВОПОЛОЖНЫХ ПАРАМЕТРОВ
	if (newData.fOpenness + newData.fEnclosedness > 1.0f)
	{
		float sum = newData.fOpenness + newData.fEnclosedness;
		newData.fOpenness /= sum;
		newData.fEnclosedness /= sum;
	}

	// СИЛЬНОЕ ПРЕОБЛАДАНИЕ ОТКРЫТОСТИ ПРИ БОЛЬШИХ РАДИУСАХ
	if (newData.fEnvironmentRadius > 50.0f)
	{
		float open_bias = (newData.fEnvironmentRadius - 50.0f) / 100.0f;
		clamp(open_bias, 0.0f, 0.8f);
		newData.fOpenness = newData.fOpenness * (1.0f - open_bias) + open_bias;
		newData.fEnclosedness *= (1.0f - open_bias);
	}

	newData.fFogDensity = g_pGamePersistent->Environment().CurrentEnv->fog_density;

	// Дисперсия
	float Variance = 0.0f;
	float AvgDistance = TotalDistance / DETAILED_DIRECTIONS_COUNT;
	for (u32 i = 0; i < distances.size(); ++i)
	{
		float Diff = distances[i] - AvgDistance;
		Variance += Diff * Diff;
	}

	newData.fEnvironmentVariance = sqrtf(Variance / distances.size()) / newData.fEnvironmentRadius;
	clamp(newData.fEnvironmentVariance, 0.0f, 1.0f);
	newData.fUniformity = 1.0f - newData.fEnvironmentVariance;

	// СИЛЬНО УМЕНЬШАЕМ ОТРАЖАТЕЛЬНУЮ СПОСОБНОСТЬ
	newData.fReflectivity = 0.05f + 0.3f * newData.fEnclosedness * m_PathTracingResult.fEnergyDistribution;
	clamp(newData.fReflectivity, 0.02f, 0.4f); // Уменьшили диапазон

	// ФИЗИЧЕСКИЕ РАСЧЕТЫ
	float adjustedRadius = newData.fEnvironmentRadius * (1.0f + newData.fOpenness);
	newData.sPhysicalData.fAdjustedRadius = adjustedRadius;
	newData.sPhysicalData.fRoomVolume = m_CurrentGeometry.fApproxVolume;

	// ПЕРЕСМОТРЕННАЯ ФОРМУЛА СИЛЫ ЭХА
	float volume_cubic_meters = m_CurrentGeometry.fApproxVolume;
	float normalized_volume = volume_cubic_meters / 1000.0f;
	clamp(normalized_volume, 0.1f, 50.0f);

	float volumeFactor = 0.0f;

	if (normalized_volume < 1.0f) // Уменьшили пороги
	{
		volumeFactor = normalized_volume * 0.3f; // Уменьшили коэффициент
	}
	else if (normalized_volume < 5.0f)
	{
		volumeFactor = 0.3f + (normalized_volume - 1.0f) / 4.0f * 0.3f;
	}
	else
	{
		volumeFactor = 0.6f - (normalized_volume - 5.0f) / 45.0f * 0.4f;
	}

	clamp(volumeFactor, 0.05f, 0.6f);

	// СИЛЬНО УМЕНЬШАЕМ СИЛУ ЭХА
	float path_tracing_echo = m_PathTracingResult.fTotalEnergy * (1.0f + m_PathTracingResult.iNumBounces * 0.1f);
	float opennessFactor = 1.0f - newData.fOpenness * 0.9f;
	newData.sPhysicalData.fEchoStrength =
		volumeFactor * newData.fReflectivity * opennessFactor * path_tracing_echo * 0.5f;
	clamp(newData.sPhysicalData.fEchoStrength, 0.005f, 0.3f); // Уменьшили диапазон

	// ПЕРЕСМОТРЕННАЯ ФОРМУЛА РЕВЕРБЕРАЦИИ
	float base_reverb = 0.0f;
	if (normalized_volume < 1.0f)
	{
		base_reverb = 0.3f + normalized_volume * 0.4f;
	}
	else if (normalized_volume < 5.0f)
	{
		base_reverb = 0.7f + (normalized_volume - 1.0f) / 4.0f * 0.8f;
	}
	else
	{
		base_reverb = 1.5f - (normalized_volume - 5.0f) / 45.0f * 0.8f;
	}

	// УМЕНЬШАЕМ ВЛИЯНИЕ PATH TRACING НА РЕВЕРБЕРАЦИЮ
	float path_tracing_reverb = 1.0f + m_PathTracingResult.iNumBounces * 0.15f;
	newData.sPhysicalData.fReverbTime = base_reverb * path_tracing_reverb * (0.4f + 0.6f * (1.0f - newData.fOpenness));
	clamp(newData.sPhysicalData.fReverbTime, 0.1f, 2.5f); // Уменьшили диапазон

	// Критическое расстояние
	newData.sPhysicalData.fCriticalDistance = 2.0f + m_CurrentGeometry.fAvgWallDistance / 4.0f;
	clamp(newData.sPhysicalData.fCriticalDistance, 2.0f, 12.0f);

	// СИЛЬНО УМЕНЬШАЕМ ЭНЕРГИЮ ОТРАЖЕНИЙ
	newData.sReflectionData.fReflectionEnergy = m_PathTracingResult.fTotalEnergy * 0.5f;
	newData.sReflectionData.fPrimaryReflections = m_PathTracingResult.fTotalEnergy * 0.3f;
	newData.sReflectionData.fSecondaryReflections = m_PathTracingResult.fTotalEnergy * 0.1f;
	newData.sReflectionData.fReflectionDelay = m_PathTracingResult.fTotalDelay;
	newData.sReflectionData.fSurfaceComplexity = m_PathTracingResult.fEnergyDistribution;

	// АГРЕССИВНАЯ КОРРЕКЦИЯ ДЛЯ ОТКРЫТЫХ ПРОСТРАНСТВ
	if (newData.fOpenness > 0.4f)
	{
		float open_factor = (newData.fOpenness - 0.4f) / 0.6f;

		// СИЛЬНОЕ УМЕНЬШЕНИЕ ЭФФЕКТОВ В ОТКРЫТЫХ ПРОСТРАНСТВАХ
		newData.sPhysicalData.fEchoStrength *= (1.0f - open_factor * 0.9f);
		newData.sPhysicalData.fReverbTime *= (1.0f - open_factor * 0.7f);
		newData.sReflectionData.fReflectionEnergy *= (1.0f - open_factor * 0.95f);
		newData.sReflectionData.fPrimaryReflections *= (1.0f - open_factor * 0.95f);
		newData.sReflectionData.fSecondaryReflections *= (1.0f - open_factor * 0.95f);
		newData.fReflectivity *= (1.0f - open_factor * 0.8f);
	}

	newData.bDataValid = true;
	newData.dwFrameStamp = Device.dwFrame;

	m_PrevData.CopyFrom(m_CurrentData);
	m_CurrentData.CopyFrom(newData);

	// Детальный отладочный вывод
	Msg("[PathTracing] Energy=%.4f, Bounces=%d", m_PathTracingResult.fTotalEnergy, m_PathTracingResult.iNumBounces);

	Msg("[EnvAnalysis] Radius=%.1f, Openness=%.3f, Enclosed=%.3f, Echo=%.4f, Reverb=%.2f", newData.fEnvironmentRadius,
		newData.fOpenness, newData.fEnclosedness, newData.sPhysicalData.fEchoStrength,
		newData.sPhysicalData.fReverbTime);

	if (::Sound)
	{
		::Sound->set_environment_data(&m_CurrentData);
		::Sound->set_need_update_environment(true);
	}
}

void __stdcall CSoundEnvironment::MT_CALC()
{
	CalculateEnhancedEnvironmentData();
}

bool CSoundEnvironment::NeedUpdate() const
{
	if (!psSoundFlags.test(ss_EAX))
		return false;

	if (Device.dwFrame < (m_LastUpdatedFrame + UPDATE_INTERVAL))
		return false;

	// Проверяем значительное перемещение камеры
	// float MoveDistance = Device.vCameraPosition.distance_to(m_PrevData.fEnvironmentRadius);
	// if (MoveDistance > 0.5f)
	return true;

	// return false;
}

void CSoundEnvironment::Update()
{
	if (NeedUpdate())
	{
		Msg("[SoundEnv] Starting balanced environment analysis...");
		Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CSoundEnvironment::MT_CALC));
		m_LastUpdatedFrame = Device.dwFrame;
	}
}
