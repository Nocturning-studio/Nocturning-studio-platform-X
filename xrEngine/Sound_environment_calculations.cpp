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
	// ����� ������ ������ ������ ���������
	std::vector<float> valid_distances;
	for (float dist : context.RaycastDistances)
	{
		if (dist < 95.0f)
		{ // ���������� ����� ������� ���������
			valid_distances.push_back(dist);
		}
	}

	if (valid_distances.size() > 10)
	{
		// ������ ������ ������� �������� �������� (����������)
		float avg_distance = 0.0f;
		for (float dist : valid_distances)
		{
			avg_distance += dist;
		}
		avg_distance /= valid_distances.size();

		// ����� ������ ������ ������ ��� ������ ���� ���������
		if (context.GeometryAnalysis.fVerticality > 1.5f)
		{
			// ������� ��������� - �������������� ������
			context.RoomVolume = PI * avg_distance * avg_distance * context.GeometryAnalysis.fAvgCeilingHeight;
		}
		else if (context.EnvironmentVariance < 0.3f)
		{
			// ���������� ��������� - ����������� ������
			context.RoomVolume = (4.0f / 3.0f) * PI * avg_distance * avg_distance * avg_distance;
		}
		else
		{
			// ������� ��������� - ���������� ������
			context.RoomVolume = 8.0f * avg_distance * avg_distance * avg_distance;
		}
	}
	else
	{
		// �������� ������������
		context.RoomVolume = 50000.0f;
	}

	// ����������� ������ ��� ��������
	context.RoomVolume = _min(context.RoomVolume, 100000.0f);
	context.NormalizedVolume = context.RoomVolume / 1000.0f;

	// ���������� ������ ����������/����������
	float hit_ratio = float(context.TotalHits) / DETAILED_DIRECTIONS_COUNT;
	float avg_distance = context.TotalDistance / DETAILED_DIRECTIONS_COUNT;

	context.Openness = (1.0f - hit_ratio) * (avg_distance / 50.0f);
	context.Enclosedness = hit_ratio * (1.0f - (avg_distance / 100.0f));

	// ������������
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

	// ������ �� ������� ��������� �������, �� ��� ����������� �����������
	if (context.RoomVolume < 5.0f)
	{
		context.RoomVolume = 5.0f;
	}

	context.NormalizedVolume = context.RoomVolume / 1000.0f;
	clamp(context.NormalizedVolume, 0.005f, 50.0f);

	// ���������� ������: � ���������� ��� �������, �� �������� ������������� - ������
	float optimal_echo_volume = 25.0f; // ��������� ��� ������� �������

	// ��������: ��� ������ �����, ��� ������� ��� (��� ���������)
	float volume_factor = 1.0f - EnvironmentContext::SmoothStep(context.NormalizedVolume / 30.0f);

	// ������� ������ ���� ��� - ������� � ����������
	float base_echo_strength = 0.3f + 0.5f * volume_factor; // ��������� ������������

	// �������� � ��������� �������������
	float enclosed_boost = 1.0f + context.Enclosedness * 0.8f;

	// ���������� � �������� �������������
	float openness_reduction = 1.0f - context.Openness * 0.6f;

	// ������������� ����������� ���
	float complexity_boost = 1.0f + context.GeometryAnalysis.fComplexity * 0.4f;

	context.VolumeFactor = base_echo_strength * enclosed_boost * openness_reduction * complexity_boost;

	// �����������: �������� �������� ��� ���������
	clamp(context.VolumeFactor, 0.1f, 1.2f); // �������� ��������

	// ������ �������� ��� - ������ � ����������, ������� �� �������� �������������
	float base_echo_delay = 0.005f + 0.2f * EnvironmentContext::LogisticGrowth(context.NormalizedVolume, 0.15f, 10.0f);

	// �������������: � ���������� ��������� ��������, �� �������� ������������� �����������
	float delay_enclosed_correction = 0.6f + 0.4f * (1.0f - context.Enclosedness);
	context.EchoDelay = base_echo_delay * delay_enclosed_correction;

	clamp(context.EchoDelay, 0.003f, 0.3f);
}

void EnvironmentCalculations::CalculateReverbParameters(EnvironmentContext& context)
{
	float normalized_volume_for_reverb = context.RoomVolume / 1000.0f;
	clamp(normalized_volume_for_reverb, 0.005f, 50.0f);

	// ���������� ������: ������������ ������� � ����������
	float volume_reverb_component =
		0.4f + 0.8f * (1.0f - EnvironmentContext::SmoothStep(normalized_volume_for_reverb / 40.0f));

	// ���������� ������������ - ������� ��� ���������
	float reflectivity_component = 0.7f + 0.3f * context.Reflectivity;
	float complexity_component = 1.0f + context.GeometryAnalysis.fComplexity * 0.5f;
	float openness_component = 1.0f - context.Openness * 0.7f; // ������� ���������� �� �������� �������������
	float enclosed_component = 1.0f + context.Enclosedness * 0.6f; // �������� � ����������
	float path_tracing_component = 1.0f + context.PathTracingResult.iNumBounces * 0.1f;

	// ��������������� ����� ������������
	context.BaseReverb = volume_reverb_component * reflectivity_component * complexity_component * openness_component *
						 enclosed_component * path_tracing_component;

	// ����������� � ������ �������� ��� ���������
	clamp(context.BaseReverb, 0.3f, 12.0f);
}

void EnvironmentCalculations::CalculateReflectionParameters(EnvironmentContext& context)
{
	// �������������� ������: ��������� ������� � ����������
	float base_reflection_intensity = context.PathTracingResult.fTotalEnergy * (0.4f + 0.6f * context.Reflectivity);

	// �������� ���������: �������� ����������� �� ������
	// � ���������� (����� �����) - ������� ���������, �� �������� ������������� (������� �����) - ������
	float volume_correction = 1.0f - EnvironmentContext::SmoothStep(context.NormalizedVolume / 50.0f);
	volume_correction = 0.3f + 0.7f * volume_correction; // ����������� ������� 0.3

	// �������� � ��������� �������������
	float enclosed_boost = 1.0f + context.Enclosedness * 0.8f;

	// ���������� �� �������� �������������
	float openness_reduction = 1.0f - context.Openness * 0.8f;

	// ���������� ����������
	float reflection_energy = base_reflection_intensity * 0.8f * volume_correction * enclosed_boost *
							  openness_reduction * context.ComplexityBoostFactor;

	float primary_reflections = base_reflection_intensity * 0.5f * volume_correction * enclosed_boost *
								openness_reduction * context.ComplexityBoostFactor;

	float secondary_reflections = base_reflection_intensity * 0.3f * volume_correction * enclosed_boost *
								  openness_reduction * context.ComplexityBoostFactor;

	// ������ �������� ��������� - ������ � ����������
	float base_reflection_delay =
		0.002f + 0.035f * EnvironmentContext::LogisticGrowth(context.NormalizedVolume, 0.2f, 15.0f);

	// �������������: � ���������� ��������� ��������
	float reflection_delay = base_reflection_delay * (0.5f + 0.5f * (1.0f - context.Enclosedness));

	// �����������: ��������� ������������ �������� ��� ���������
	clamp(reflection_energy, 0.01f, 0.8f);
	clamp(primary_reflections, 0.005f, 0.6f);
	clamp(secondary_reflections, 0.003f, 0.4f);
	clamp(reflection_delay, 0.001f, 0.05f);

	// ���������� ����������
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
	// �������� ���������: �������� ���������� EAX
	// � ���������� - ����� �������������/������������� ��������, �� �������� ������������� - ����� �������������

	// Room parameters - ��������� ��������� ����� ����� ������������� ��������
	float room_intensity = context.Enclosedness * (1.0f - context.Openness * 0.3f);
	context.EAXData.lRoom =
		(long)EnvironmentContext::InterpolateLinear(-1500, -50, room_intensity); // ����� ������������� ��������

	context.EAXData.lRoomHF = (long)EnvironmentContext::InterpolateLinear(-800, -10, context.Openness);
	context.EAXData.flRoomRolloffFactor = 0.05f + 0.95f * context.Enclosedness;

	// Decay parameters - ������� � ����������
	float decay_time = context.BaseReverb * (0.8f + 0.4f * context.Enclosedness);
	clamp(decay_time, 0.4f, 15.0f); // �������� �������
	context.EAXData.flDecayTime = decay_time;

	context.EAXData.flDecayHFRatio = 0.1f + 0.8f * context.Openness;

	// Reflections parameters - ������� � ����������
	float reflectionStrength = context.VolumeFactor * context.Reflectivity;

	// ��������: � ���������� ����� ������������� �������� ���������
	float reflections_min =
		EnvironmentContext::InterpolateLinear(-800, -2000, context.Openness); // � ����������: -800, �� ��������: -2000
	float reflections_max =
		EnvironmentContext::InterpolateLinear(300, -800, context.Openness); // � ����������: 300, �� ��������: -800

	context.EAXData.lReflections = (long)(reflections_min + (reflections_max - reflections_min) * reflectionStrength);

	context.EAXData.flReflectionsDelay = context.EchoDelay;
	clamp(context.EAXData.flReflectionsDelay, 0.002f, 0.2f);

	// Reverb parameters - ������� � ����������
	float reverbIntensity = context.VolumeFactor * context.Enclosedness;

	// ��������: � ���������� ����� ������������� �������� ������������
	float reverb_min =
		EnvironmentContext::InterpolateLinear(-800, -2000, context.Openness); // � ����������: -800, �� ��������: -2000
	float reverb_max =
		EnvironmentContext::InterpolateLinear(500, -500, context.Openness); // � ����������: 500, �� ��������: -500

	context.EAXData.lReverb = (long)(reverb_min + (reverb_max - reverb_min) * reverbIntensity);
	context.EAXData.flReverbDelay = context.EchoDelay * 1.3f;
	clamp(context.EAXData.flReverbDelay, 0.003f, 0.25f);

	// Environment parameters
	float environment_size = 5.0f + 95.0f * (1.0f - context.Enclosedness);
	context.EAXData.flEnvironmentSize = environment_size;
	clamp(context.EAXData.flEnvironmentSize, 5.0f, 100.0f);

	context.EAXData.flEnvironmentDiffusion = 0.1f + 0.8f * context.Openness;
	context.EAXData.flAirAbsorptionHF = -1.0f - 10.0f * context.Openness;

	// ��������� ������������� ��� ����� �������� �����������
	if (context.Openness > 0.8f)
	{
		float open_reduction = 1.0f - (context.Openness - 0.8f) * 5.0f;
		clamp(open_reduction, 0.1f, 1.0f);

		context.EAXData.lReflections = (long)(context.EAXData.lReflections * open_reduction);
		context.EAXData.lReverb = (long)(context.EAXData.lReverb * open_reduction);
	}

	// ������ ��������� ����������
	context.EAXData.lRoom = _max(context.EAXData.lRoom, -3000);
	context.EAXData.lRoomHF = _max(context.EAXData.lRoomHF, -1500);
	context.EAXData.lReflections = _min(_max(context.EAXData.lReflections, -3000), 1000);
	context.EAXData.lReverb = _min(_max(context.EAXData.lReverb, -2500), 1000);

	// ���������� �����
	Msg("[EAX Final] Room=%d, Reflections=%d, Reverb=%d, Decay=%.2fs", context.EAXData.lRoom,
		context.EAXData.lReflections, context.EAXData.lReverb, context.EAXData.flDecayTime);
	Msg("[EAX Final] Openness=%.2f, Enclosedness=%.2f, Volume=%.1f", context.Openness, context.Enclosedness,
		context.NormalizedVolume);
}
