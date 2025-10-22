///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX - Calculation Components
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_calculations.h"
///////////////////////////////////////////////////////////////////////////////////

void EnvironmentCalculations::CalculateContinuousMultipliers(EnvironmentContext& context)
{
	// Continuous openness multiplier
	context.OpenSpaceMultiplier = EnvironmentContext::SmoothStep(context.Openness * 1.2f);

	// Continuous enclosedness multiplier
	context.EnclosedMultiplier = EnvironmentContext::GaussianBell(context.Enclosedness, 0.7f, 0.25f) *
								 EnvironmentContext::ExponentialDecay(context.Openness, 4.0f) * 2.0f;

	// Continuous complexity multiplier
	context.ComplexityMultiplier = EnvironmentContext::SmoothStep(context.GeometryAnalysis.fComplexity * 1.5f);

	// Apply continuous multipliers to parameters
	context.OpenReductionFactor = 1.0f - context.OpenSpaceMultiplier * 0.8f;
	context.EnclosedBoostFactor = 1.0f + context.EnclosedMultiplier * 0.6f;
	context.ComplexityBoostFactor = 1.0f + context.ComplexityMultiplier * 0.4f;
}

void EnvironmentCalculations::CalculateReflectivity(EnvironmentContext& context)
{
	// Enhanced material-aware reflectivity calculation
	float avg_material_reflectivity = 0.0f;
	u32 total_material_hits = 0;
	for (u32 i = 1; i <= 5; ++i)
	{
		if (context.MaterialHits[i] > 0)
		{
			float material_reflectivity = 1.0f - (i - 1) * 0.2f;
			clamp(material_reflectivity, 0.15f, 0.8f);
			avg_material_reflectivity += material_reflectivity * context.MaterialHits[i];
			total_material_hits += context.MaterialHits[i];
		}
	}
	if (total_material_hits > 0)
		avg_material_reflectivity /= total_material_hits;
	else
		avg_material_reflectivity = 0.1f;
	context.Reflectivity = 0.05f + 0.35f * context.Enclosedness * avg_material_reflectivity;
	clamp(context.Reflectivity, 0.02f, 0.45f);
}

void EnvironmentCalculations::CalculateGeometricProperties(EnvironmentContext& context)
{
	// Более точный расчет объема помещения
	std::vector<float> valid_distances;
	for (float dist : context.RaycastDistances)
	{
		if (dist < 95.0f)
		{ // Игнорируем очень далекие попадания
			valid_distances.push_back(dist);
		}
	}

	if (valid_distances.size() > 10)
	{
		// Расчет объема методом выпуклой оболочки (упрощенный)
		float avg_distance = 0.0f;
		for (float dist : valid_distances)
		{
			avg_distance += dist;
		}
		avg_distance /= valid_distances.size();

		// Более точная оценка объема для разных форм помещений
		if (context.GeometryAnalysis.fVerticality > 1.5f)
		{
			// Высокие помещения - цилиндрическая модель
			context.RoomVolume = PI * avg_distance * avg_distance * context.GeometryAnalysis.fAvgCeilingHeight;
		}
		else if (context.EnvironmentVariance < 0.3f)
		{
			// Регулярные помещения - сферическая модель
			context.RoomVolume = (4.0f / 3.0f) * PI * avg_distance * avg_distance * avg_distance;
		}
		else
		{
			// Сложные помещения - кубическая модель
			context.RoomVolume = 8.0f * avg_distance * avg_distance * avg_distance;
		}
	}
	else
	{
		// Открытое пространство
		context.RoomVolume = 50000.0f;
	}

	// Ограничение объема для расчетов
	context.RoomVolume = _min(context.RoomVolume, 100000.0f);
	context.NormalizedVolume = context.RoomVolume / 1000.0f;

	// Улучшенный расчет открытости/закрытости
	float hit_ratio = float(context.TotalHits) / DETAILED_DIRECTIONS_COUNT;
	float avg_distance = context.TotalDistance / DETAILED_DIRECTIONS_COUNT;

	context.Openness = (1.0f - hit_ratio) * (avg_distance / 50.0f);
	context.Enclosedness = hit_ratio * (1.0f - (avg_distance / 100.0f));

	// Балансировка
	float sum = context.Openness + context.Enclosedness;
	if (sum > 0)
	{
		context.Openness /= sum;
		context.Enclosedness /= sum;
	}

	clamp(context.Openness, 0.0f, 1.0f);
	clamp(context.Enclosedness, 0.0f, 1.0f);
}

void EnvironmentCalculations::CalculateEchoParameters(EnvironmentContext& context)
{
	context.RoomVolume = context.GeometryAnalysis.fApproxVolume;

	// Защита от слишком маленьких объемов, но без чрезмерного ограничения
	if (context.RoomVolume < 5.0f)
	{
		context.RoomVolume = 5.0f;
	}

	context.NormalizedVolume = context.RoomVolume / 1000.0f;
	clamp(context.NormalizedVolume, 0.005f, 50.0f);

	// ПРАВИЛЬНАЯ МОДЕЛЬ: в помещениях эхо сильнее, на открытых пространствах - слабее
	float optimal_echo_volume = 25.0f; // Увеличено для лучшего баланса

	// ИНВЕРСИЯ: чем меньше объем, тем сильнее эхо (для помещений)
	float volume_factor = 1.0f - EnvironmentContext::SmoothStep(context.NormalizedVolume / 30.0f);

	// Базовый расчет силы эха - СИЛЬНЕЕ в помещениях
	float base_echo_strength = 0.3f + 0.5f * volume_factor; // Увеличены коэффициенты

	// Усиление в замкнутых пространствах
	float enclosed_boost = 1.0f + context.Enclosedness * 0.8f;

	// Ослабление в открытых пространствах
	float openness_reduction = 1.0f - context.Openness * 0.6f;

	// Комплексность увеличивает эхо
	float complexity_boost = 1.0f + context.GeometryAnalysis.fComplexity * 0.4f;

	context.VolumeFactor = base_echo_strength * enclosed_boost * openness_reduction * complexity_boost;

	// ОГРАНИЧЕНИЯ: максимум увеличен для помещений
	clamp(context.VolumeFactor, 0.1f, 1.2f); // Увеличен максимум

	// Расчет задержки эха - КОРОЧЕ в помещениях, ДЛИННЕЕ на открытых пространствах
	float base_echo_delay = 0.005f + 0.2f * EnvironmentContext::LogisticGrowth(context.NormalizedVolume, 0.15f, 10.0f);

	// Корректировка: в помещениях уменьшаем задержку, на открытых пространствах увеличиваем
	float delay_enclosed_correction = 0.6f + 0.4f * (1.0f - context.Enclosedness);
	context.EchoDelay = base_echo_delay * delay_enclosed_correction;

	clamp(context.EchoDelay, 0.003f, 0.3f);
}

void EnvironmentCalculations::CalculateReverbParameters(EnvironmentContext& context)
{
	float normalized_volume_for_reverb = context.RoomVolume / 1000.0f;
	clamp(normalized_volume_for_reverb, 0.005f, 50.0f);

	// ПРАВИЛЬНАЯ МОДЕЛЬ: реверберация сильнее в помещениях
	float volume_reverb_component =
		0.4f + 0.8f * (1.0f - EnvironmentContext::SmoothStep(normalized_volume_for_reverb / 40.0f));

	// Компоненты реверберации - УСИЛЕНЫ для помещений
	float reflectivity_component = 0.7f + 0.3f * context.Reflectivity;
	float complexity_component = 1.0f + context.GeometryAnalysis.fComplexity * 0.5f;
	float openness_component = 1.0f - context.Openness * 0.7f; // Сильнее уменьшение на открытых пространствах
	float enclosed_component = 1.0f + context.Enclosedness * 0.6f; // Усиление в помещениях
	float path_tracing_component = 1.0f + context.PathTracingResult.iNumBounces * 0.1f;

	// Комбинированное время реверберации
	context.BaseReverb = volume_reverb_component * reflectivity_component * complexity_component * openness_component *
						 enclosed_component * path_tracing_component;

	// Ограничения с учетом усиления для помещений
	clamp(context.BaseReverb, 0.3f, 12.0f);
}

void EnvironmentCalculations::CalculateReflectionParameters(EnvironmentContext& context)
{
	// ПЕРЕСМОТРЕННЫЙ РАСЧЕТ: отражения сильнее в помещениях
	float base_reflection_intensity = context.PathTracingResult.fTotalEnergy * (0.4f + 0.6f * context.Reflectivity);

	// Ключевое изменение: ИНВЕРСИЯ зависимости от объема
	// В помещениях (малый объем) - сильные отражения, на открытых пространствах (большой объем) - слабые
	float volume_correction = 1.0f - EnvironmentContext::SmoothStep(context.NormalizedVolume / 50.0f);
	volume_correction = 0.3f + 0.7f * volume_correction; // Минимальный уровень 0.3

	// Усиление в замкнутых пространствах
	float enclosed_boost = 1.0f + context.Enclosedness * 0.8f;

	// Ослабление на открытых пространствах
	float openness_reduction = 1.0f - context.Openness * 0.8f;

	// Применение множителей
	float reflection_energy = base_reflection_intensity * 0.8f * volume_correction * enclosed_boost *
							  openness_reduction * context.ComplexityBoostFactor;

	float primary_reflections = base_reflection_intensity * 0.5f * volume_correction * enclosed_boost *
								openness_reduction * context.ComplexityBoostFactor;

	float secondary_reflections = base_reflection_intensity * 0.3f * volume_correction * enclosed_boost *
								  openness_reduction * context.ComplexityBoostFactor;

	// Расчет задержки отражений - КОРОЧЕ в помещениях
	float base_reflection_delay =
		0.002f + 0.035f * EnvironmentContext::LogisticGrowth(context.NormalizedVolume, 0.2f, 15.0f);

	// Корректировка: в помещениях уменьшаем задержку
	float reflection_delay = base_reflection_delay * (0.5f + 0.5f * (1.0f - context.Enclosedness));

	// ОГРАНИЧЕНИЯ: увеличены максимальные значения для помещений
	clamp(reflection_energy, 0.01f, 0.8f);
	clamp(primary_reflections, 0.005f, 0.6f);
	clamp(secondary_reflections, 0.003f, 0.4f);
	clamp(reflection_delay, 0.001f, 0.05f);

	// Сохранение параметров
	context.ReflectionEnergy = reflection_energy;
	context.PrimaryReflections = primary_reflections;
	context.SecondaryReflections = secondary_reflections;
	context.ReflectionDelay = reflection_delay;
	context.SurfaceComplexity =
		context.PathTracingResult.fEnergyDistribution * (0.6f + 0.4f * context.GeometryAnalysis.fComplexity);
	clamp(context.SurfaceComplexity, 0.0f, 0.8f);

	Msg("[Reflection Analysis] Energy=%.4f, Primary=%.4f, Secondary=%.4f", reflection_energy, primary_reflections,
		secondary_reflections);
	Msg("[Reflection Analysis] Delay=%.4fs, Complexity=%.3f, Volume=%.1f", reflection_delay, context.SurfaceComplexity,
		context.NormalizedVolume);
}

void EnvironmentCalculations::MapToEAXParameters(EnvironmentContext& context)
{
	// Ключевое изменение: ИНВЕРСИЯ параметров EAX
	// В помещениях - менее отрицательные/положительные значения, на открытых пространствах - более отрицательные

	// Room parameters - ЗАМКНУТЫЕ помещения имеют менее отрицательные значения
	float room_intensity = context.Enclosedness * (1.0f - context.Openness * 0.3f);
	context.EAXData.lRoom =
		(long)EnvironmentContext::InterpolateLinear(-1500, -50, room_intensity); // Менее отрицательные значения

	context.EAXData.lRoomHF = (long)EnvironmentContext::InterpolateLinear(-800, -10, context.Openness);
	context.EAXData.flRoomRolloffFactor = 0.05f + 0.95f * context.Enclosedness;

	// Decay parameters - ДЛИННЕЕ в помещениях
	float decay_time = context.BaseReverb * (0.8f + 0.4f * context.Enclosedness);
	clamp(decay_time, 0.4f, 15.0f); // Увеличен минимум
	context.EAXData.flDecayTime = decay_time;

	context.EAXData.flDecayHFRatio = 0.1f + 0.8f * context.Openness;

	// Reflections parameters - СИЛЬНЕЕ в помещениях
	float reflectionStrength = context.VolumeFactor * context.Reflectivity;

	// ИНВЕРСИЯ: в помещениях менее отрицательные значения отражений
	float reflections_min =
		EnvironmentContext::InterpolateLinear(-800, -2000, context.Openness); // В помещениях: -800, на открытых: -2000
	float reflections_max =
		EnvironmentContext::InterpolateLinear(300, -800, context.Openness); // В помещениях: 300, на открытых: -800

	context.EAXData.lReflections = (long)(reflections_min + (reflections_max - reflections_min) * reflectionStrength);

	context.EAXData.flReflectionsDelay = context.EchoDelay;
	clamp(context.EAXData.flReflectionsDelay, 0.002f, 0.2f);

	// Reverb parameters - СИЛЬНЕЕ в помещениях
	float reverbIntensity = context.VolumeFactor * context.Enclosedness;

	// ИНВЕРСИЯ: в помещениях менее отрицательные значения реверберации
	float reverb_min =
		EnvironmentContext::InterpolateLinear(-800, -2000, context.Openness); // В помещениях: -800, на открытых: -2000
	float reverb_max =
		EnvironmentContext::InterpolateLinear(500, -500, context.Openness); // В помещениях: 500, на открытых: -500

	context.EAXData.lReverb = (long)(reverb_min + (reverb_max - reverb_min) * reverbIntensity);
	context.EAXData.flReverbDelay = context.EchoDelay * 1.3f;
	clamp(context.EAXData.flReverbDelay, 0.003f, 0.25f);

	// Environment parameters
	float environment_size = 5.0f + 95.0f * (1.0f - context.Enclosedness);
	context.EAXData.flEnvironmentSize = environment_size;
	clamp(context.EAXData.flEnvironmentSize, 5.0f, 100.0f);

	context.EAXData.flEnvironmentDiffusion = 0.1f + 0.8f * context.Openness;
	context.EAXData.flAirAbsorptionHF = -1.0f - 10.0f * context.Openness;

	// Финальная корректировка для очень открытых пространств
	if (context.Openness > 0.8f)
	{
		float open_reduction = 1.0f - (context.Openness - 0.8f) * 5.0f;
		clamp(open_reduction, 0.1f, 1.0f);

		context.EAXData.lReflections = (long)(context.EAXData.lReflections * open_reduction);
		context.EAXData.lReverb = (long)(context.EAXData.lReverb * open_reduction);
	}

	// Мягкая валидация параметров
	context.EAXData.lRoom = _max(context.EAXData.lRoom, -3000);
	context.EAXData.lRoomHF = _max(context.EAXData.lRoomHF, -1500);
	context.EAXData.lReflections = _min(_max(context.EAXData.lReflections, -3000), 1000);
	context.EAXData.lReverb = _min(_max(context.EAXData.lReverb, -2500), 1000);

	// Отладочный вывод
	Msg("[EAX Final] Room=%d, Reflections=%d, Reverb=%d, Decay=%.2fs", context.EAXData.lRoom,
		context.EAXData.lReflections, context.EAXData.lReverb, context.EAXData.flDecayTime);
	Msg("[EAX Final] Openness=%.2f, Enclosedness=%.2f, Volume=%.1f", context.Openness, context.Enclosedness,
		context.NormalizedVolume);
}
