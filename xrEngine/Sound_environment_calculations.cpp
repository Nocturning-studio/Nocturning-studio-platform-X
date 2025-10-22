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

void EnvironmentCalculations::CalculateGeometricProperties(EnvironmentContext& context)
{
	// Calculate basic environment parameters
	context.EnvironmentRadius = context.TotalDistance / DETAILED_DIRECTIONS_COUNT;
	context.EnvironmentDensity = float(context.TotalHits) / DETAILED_DIRECTIONS_COUNT;

	// Enhanced openness calculation
	float basic_openness = float(context.FarHits + context.MediumHits * 0.3f) / DETAILED_DIRECTIONS_COUNT;
	float path_tracing_openness = 1.0f - (context.PathTracingResult.fTotalEnergy * 3.0f);
	float geometry_openness = 1.0f - (context.GeometryAnalysis.fComplexity * 0.15f + context.EnvironmentDensity * 0.7f);

	context.Openness = (basic_openness * 0.5f + path_tracing_openness * 0.3f + geometry_openness * 0.2f);
	clamp(context.Openness, 0.0f, 1.0f);

	// Enhanced enclosedness calculation
	float basic_enclosedness = float(context.CloseHits + context.MediumHits * 0.5f) / DETAILED_DIRECTIONS_COUNT;
	float path_tracing_enclosedness = context.PathTracingResult.fTotalEnergy * 1.5f;
	float geometry_enclosedness = context.GeometryAnalysis.fComplexity * 0.4f + context.EnvironmentDensity * 0.6f;

	context.Enclosedness =
		(basic_enclosedness * 0.5f + path_tracing_enclosedness * 0.25f + geometry_enclosedness * 0.25f);
	clamp(context.Enclosedness, 0.0f, 1.0f);

	// Environment variance and uniformity
	float Variance = 0.0f;
	float AvgDistance = context.TotalDistance / DETAILED_DIRECTIONS_COUNT;
	for (u32 i = 0; i < context.RaycastDistances.size(); ++i)
	{
		float Diff = context.RaycastDistances[i] - AvgDistance;
		Variance += Diff * Diff;
	}

	context.EnvironmentVariance =
		sqrtf(Variance / context.RaycastDistances.size()) / (context.EnvironmentRadius + 0.1f);
	clamp(context.EnvironmentVariance, 0.0f, 1.0f);
	context.Uniformity = 1.0f - context.EnvironmentVariance;
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

void EnvironmentCalculations::CalculateEchoParameters(EnvironmentContext& context)
{
	context.RoomVolume = context.GeometryAnalysis.fApproxVolume;
	context.NormalizedVolume = context.RoomVolume / 1000.0f;
	clamp(context.NormalizedVolume, 0.1f, 50.0f);

	// Физически корректная модель эха
	float optimal_echo_volume = 20.0f;
	float volume_ratio = context.NormalizedVolume / optimal_echo_volume;

	float echo_gaussian = expf(-(volume_ratio - 1.0f) * (volume_ratio - 1.0f) / 0.5f);
	float small_room_echo_base = 0.2f;
	float echo_peak_boost = 0.6f;
	float large_space_echo_reduction =
		1.0f - EnvironmentContext::SmoothStep((context.NormalizedVolume - 30.0f) / 20.0f) * 0.4f;

	context.VolumeFactor = small_room_echo_base + echo_gaussian * echo_peak_boost;
	context.VolumeFactor *= large_space_echo_reduction;

	float echo_openness_correction = 1.0f - context.Openness * 0.5f;
	float echo_complexity_boost = 1.0f + context.GeometryAnalysis.fComplexity * 0.3f;

	context.VolumeFactor *= echo_openness_correction * echo_complexity_boost;
	clamp(context.VolumeFactor, 0.1f, 0.8f);

	// Расчет задержки эха
	float base_echo_delay = 0.01f + 0.14f * EnvironmentContext::LogisticGrowth(context.NormalizedVolume, 0.15f, 10.0f);
	float enclosed_delay_correction = 0.7f + 0.3f * (1.0f - context.Enclosedness);
	context.EchoDelay = base_echo_delay * enclosed_delay_correction;
	clamp(context.EchoDelay, 0.01f, 0.25f);
}

void EnvironmentCalculations::CalculateReverbParameters(EnvironmentContext& context)
{
	float normalized_volume_for_reverb = context.RoomVolume / 1000.0f;
	clamp(normalized_volume_for_reverb, 0.1f, 50.0f);

	// Базовое время реверберации на основе объема
	float volume_reverb_component =
		0.3f + 0.7f * EnvironmentContext::LogisticGrowth(normalized_volume_for_reverb, 0.25f, 8.0f);

	// Корректировка для очень больших пространств
	if (normalized_volume_for_reverb > 25.0f)
	{
		float large_space_reverb_reduction = 1.0f - (normalized_volume_for_reverb - 25.0f) / 25.0f * 0.3f;
		clamp(large_space_reverb_reduction, 0.7f, 1.0f);
		volume_reverb_component *= large_space_reverb_reduction;
	}

	// Компоненты реверберации
	float reflectivity_component = 0.6f + 0.4f * context.Reflectivity;
	float complexity_component = 1.0f + context.GeometryAnalysis.fComplexity * 0.3f;
	float openness_component = 1.0f - context.Openness * 0.5f;
	float path_tracing_component = 1.0f + context.PathTracingResult.iNumBounces * 0.08f;

	// Комбинированное время реверберации
	context.BaseReverb = volume_reverb_component * reflectivity_component * complexity_component * openness_component *
						 path_tracing_component;
}

void EnvironmentCalculations::CalculateReflectionParameters(EnvironmentContext& context)
{
	// Расчет интенсивности отражений на основе path tracing и отражательной способности
	float base_reflection_intensity = context.PathTracingResult.fTotalEnergy * (0.5f + 0.5f * context.Reflectivity);

	// Применение непрерывных множителей к параметрам отражений
	float reflection_energy = base_reflection_intensity * 0.7f * context.OpenReductionFactor *
							  context.EnclosedBoostFactor * context.ComplexityBoostFactor;

	float primary_reflections = base_reflection_intensity * 0.5f * context.OpenReductionFactor *
								context.EnclosedBoostFactor * context.ComplexityBoostFactor;

	float secondary_reflections = base_reflection_intensity * 0.2f * context.OpenReductionFactor *
								  context.EnclosedBoostFactor * context.ComplexityBoostFactor;

	// Непрерывный расчет задержки отражений на основе path tracing и геометрии
	float base_reflection_delay =
		0.005f + 0.025f * EnvironmentContext::LogisticGrowth(context.NormalizedVolume, 0.3f, 8.0f);

	float reflection_delay =
		base_reflection_delay * (0.6f + 0.4f * context.Enclosedness) * (1.0f + context.ComplexityMultiplier * 0.3f);

	// Расчет сложности поверхности на основе распределения энергии и геометрии
	float surface_complexity = context.PathTracingResult.fEnergyDistribution *
							   (0.6f + 0.4f * context.GeometryAnalysis.fComplexity) * context.ComplexityBoostFactor;

	// Ограничение параметров отражений
	clamp(reflection_energy, 0.005f, 0.4f);
	clamp(primary_reflections, 0.0f, 0.5f);
	clamp(secondary_reflections, 0.0f, 0.2f);
	clamp(reflection_delay, 0.003f, 0.03f);
	clamp(surface_complexity, 0.0f, 0.7f);

	// Сохранение расчетных параметров в контекст для возможного использования в отладке
	// или будущих расширениях системы
	context.ReflectionEnergy = reflection_energy;
	context.PrimaryReflections = primary_reflections;
	context.SecondaryReflections = secondary_reflections;
	context.ReflectionDelay = reflection_delay;
	context.SurfaceComplexity = surface_complexity;

	// Дополнительная корректировка на основе пространственного распределения
	float spatial_variance_effect = 1.0f + context.EnvironmentVariance * 0.4f;
	float uniformity_effect = 1.0f - context.Uniformity * 0.2f;

	// Финальная корректировка энергии отражений
	reflection_energy *= spatial_variance_effect * uniformity_effect;
	clamp(reflection_energy, 0.005f, 0.45f);

	// Применение дополнительных физических корректировок
	// В очень открытых пространствах уменьшаем энергию отражений
	float open_space_reflection_reduction = EnvironmentContext::ExponentialDecay(context.Openness, 3.0f);
	reflection_energy *= open_space_reflection_reduction;

	// В сложных геометриях увеличиваем вторичные отражения
	float complex_geometry_boost = 1.0f + context.GeometryAnalysis.fComplexity * 0.5f;
	secondary_reflections *= complex_geometry_boost;
	clamp(secondary_reflections, 0.0f, 0.25f);

	// Отладочный вывод
	Msg("[Reflection Analysis] Energy=%.4f, Primary=%.4f, Secondary=%.4f", reflection_energy, primary_reflections,
		secondary_reflections);
	Msg("[Reflection Analysis] Delay=%.4fs, Complexity=%.3f", reflection_delay, surface_complexity);
}

void EnvironmentCalculations::MapToEAXParameters(EnvironmentContext& context)
{
	// Room parameters - непрерывные функции
	context.EAXData.lRoom = (long)EnvironmentContext::InterpolateLinear(-3000, -200, context.Enclosedness);
	context.EAXData.lRoomHF = (long)EnvironmentContext::InterpolateLinear(-1500, -50, context.Openness);
	context.EAXData.flRoomRolloffFactor = EnvironmentContext::InterpolateLinear(0.05f, 1.2f, context.Enclosedness);

	// Decay parameters
	context.EAXData.flDecayTime =
		context.BaseReverb * context.EnclosedBoostFactor * (1.0f - context.OpenSpaceMultiplier * 0.2f);
	clamp(context.EAXData.flDecayTime, 0.2f, 3.2f);
	context.EAXData.flDecayHFRatio = EnvironmentContext::InterpolateLinear(0.15f, 0.85f, context.Openness);

	// Reflections parameters
	float reflectionStrength = context.VolumeFactor * context.Reflectivity * context.PathTracingResult.fTotalEnergy;
	clamp(reflectionStrength, 0.0f, 1.0f);

	// НЕПРЕРЫВНОЕ отображение отражений на основе размера окружения
	float adjustedRadius = context.EnvironmentRadius * (0.8f + context.Openness * 0.4f);
	clamp(adjustedRadius, 1.0f, 100.0f);

	// Непрерывная функция для отражений вместо условий
	float normalizedRadius = adjustedRadius / 100.0f;

	// Гауссовы колокола для разных диапазонов размеров
	float smallRoomReflections = EnvironmentContext::GaussianBell(normalizedRadius, 0.05f, 0.1f);
	float mediumRoomReflections = EnvironmentContext::GaussianBell(normalizedRadius, 0.15f, 0.15f);
	float largeRoomReflections = EnvironmentContext::GaussianBell(normalizedRadius, 0.3f, 0.2f);
	float hugeRoomReflections = EnvironmentContext::GaussianBell(normalizedRadius, 0.6f, 0.25f);

	// Нормализация весов
	float totalWeight =
		smallRoomReflections + mediumRoomReflections + largeRoomReflections + hugeRoomReflections + 0.001f;

	// Взвешенная сумма диапазонов отражений
	float minReflections = (smallRoomReflections * -500 + mediumRoomReflections * -800 + largeRoomReflections * -1500 +
							hugeRoomReflections * -2000) /
						   totalWeight;
	float maxReflections = (smallRoomReflections * 500 + mediumRoomReflections * 300 + largeRoomReflections * 100 +
							hugeRoomReflections * -500) /
						   totalWeight;

	context.EAXData.lReflections = (long)(minReflections + (maxReflections - minReflections) * reflectionStrength);

	context.EAXData.flReflectionsDelay = context.EchoDelay;
	clamp(context.EAXData.flReflectionsDelay, 0.005f, 0.25f);

	// Reverb parameters
	float reverbIntensity = context.VolumeFactor * context.Enclosedness;
	clamp(reverbIntensity, 0.0f, 1.0f);
	context.EAXData.lReverb = (long)EnvironmentContext::InterpolateLinear(-1500, 600, reverbIntensity);
	context.EAXData.flReverbDelay = context.EchoDelay * 1.5f;
	clamp(context.EAXData.flReverbDelay, 0.01f, 0.35f);

	// Environment parameters
	context.EAXData.flEnvironmentSize = adjustedRadius;
	clamp(context.EAXData.flEnvironmentSize, 1.0f, 100.0f);
	context.EAXData.flEnvironmentDiffusion =
		EnvironmentContext::InterpolateLinear(0.1f, 0.95f, 1.0f - context.Openness);
	context.EAXData.flAirAbsorptionHF =
		EnvironmentContext::InterpolateLinear(-1.0f, -12.0f, context.EAXData.flAirAbsorptionHF);

	// НЕПРЕРЫВНАЯ коррекция открытых пространств вместо условий
	float openSpaceEffect = EnvironmentContext::SmoothStep(context.Openness * 1.5f - 0.6f); // Начинается с 0.4f плавно

	// Непрерывное уменьшение эффектов в открытых пространствах
	float reflectionReduction = EnvironmentContext::ExponentialDecay(openSpaceEffect, 2.0f);
	float reverbReduction = EnvironmentContext::ExponentialDecay(openSpaceEffect, 1.5f);
	float decayReduction = 1.0f - openSpaceEffect * 0.5f;
	float delayIncrease = 1.0f + openSpaceEffect * 0.3f;

	context.EAXData.lReflections = (long)(context.EAXData.lReflections * reflectionReduction);
	context.EAXData.lReverb = (long)(context.EAXData.lReverb * reverbReduction);
	context.EAXData.flDecayTime *= decayReduction;
	context.EAXData.flReflectionsDelay *= delayIncrease;
	context.EAXData.flEnvironmentDiffusion =
		EnvironmentContext::InterpolateLinear(context.EAXData.flEnvironmentDiffusion, 0.98f, openSpaceEffect);

	// Final validation
	context.EAXData.lRoom = _max(context.EAXData.lRoom, -4000);
	context.EAXData.lRoomHF = _max(context.EAXData.lRoomHF, -2000);
	context.EAXData.lReflections = _max(context.EAXData.lReflections, -2500);
	context.EAXData.lReverb = _min(_max(context.EAXData.lReverb, -2000), 800);
}
