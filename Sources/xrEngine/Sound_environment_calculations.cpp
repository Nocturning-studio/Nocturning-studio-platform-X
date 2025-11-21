///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_calculations.cpp
// Tuned for Realistic EAX + Fog Support
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
#include "Sound_environment_calculations.h"
#include "igame_persistent.h" // Для доступа к Environment
#include <algorithm>

namespace Internal
{
float EyringReverberationTime(float volume, float surface_area, float absorption)
{
	if (surface_area < 0.1f || absorption >= 1.0f)
		return 0.1f;
	// Sabine/Eyring constant 0.161
	return 0.161f * volume / (-surface_area * logf(1.0f - absorption));
}
} // namespace Internal

void EnvironmentCalculations::CalculateGeometricAcoustics(EnvironmentContext& context)
{
	if (context.RaycastDistances.empty())
	{
		context.PhysicalVolume = 1000.0f;
		context.MeanFreePath = 10.0f;
		return;
	}

	float mean_distance = 0.0f;
	for (float dist : context.RaycastDistances)
		mean_distance += dist;
	mean_distance /= context.RaycastDistances.size();

	context.MeanFreePath = _max(mean_distance, 1.0f);
	context.PhysicalVolume = (4.0f / 3.0f) * PI * powf(mean_distance, 3.0f);
	context.PhysicalVolume = clampr(context.PhysicalVolume, 10.0f, 250000.0f);

	float hit_ratio = (float)context.TotalHits / context.RaycastDistances.size();
	context.GeometricEnclosedness = hit_ratio;

	// Хак: Если объем мал, но лучи попали в скайбокс (дыры в геометрии),
	// считаем что мы все равно внутри.
	if (context.PhysicalVolume < 3000.0f && hit_ratio > 0.3f)
	{
		context.GeometricEnclosedness = _max(context.GeometricEnclosedness, 0.9f);
	}

	context.GeometricOpenness = 1.0f - context.GeometricEnclosedness;
}

void EnvironmentCalculations::CalculateMaterialProperties(EnvironmentContext& context)
{
	// Stone/Metal = 0.95, Wood = 0.5, Soft = 0.2
	float mat_weights[] = {0.0f, 0.95f, 0.92f, 0.5f, 0.2f, 0.8f};

	if (context.TotalHits > 0)
	{
		float accum_refl = 0.0f;
		for (u32 m = 1; m <= 5; ++m)
			if (context.MaterialHits[m] > 0)
				accum_refl += (float)context.MaterialHits[m] * mat_weights[m];

		context.PhysicalReflectivity = accum_refl / context.TotalHits;
	}
	else
	{
		context.PhysicalReflectivity = 0.1f;
	}

	context.PhysicalReflectivity = clampr(context.PhysicalReflectivity, 0.1f, 0.98f);
}

void EnvironmentCalculations::CalculateRoomAcoustics(EnvironmentContext& context)
{
	// === 1. ТЮНИНГ ВРЕМЕНИ (DECAY TIME) ===
	float volume = context.PhysicalVolume;
	float surface_area = 6.0f * powf(volume, 2.0f / 3.0f);
	float abs_coeff = 1.0f - context.PhysicalReflectivity;
	abs_coeff = clampr(abs_coeff, 0.05f, 0.95f);

	float rt60 = Internal::EyringReverberationTime(volume, surface_area, abs_coeff);

	// [FIX] Более аккуратный множитель.
	// Раньше было 1.5, что делало маленькие комнаты гулкими.
	// Теперь для маленьких комнат почти нет буста, для больших - есть.
	float size_boost = 1.0f + (clampr(volume / 10000.0f, 0.0f, 0.5f));
	rt60 *= size_boost;

	// [FIX] Снижаем минимальный порог.
	// 0.35s - это "сухая" комната. 1.5s - это уже зал.
	context.ReverbTime = clampr(rt60, 0.35f, 10.0f);

	// Levels
	context.ReverbLevel = context.PhysicalReflectivity;
	if (context.GeometricEnclosedness < 0.4f)
		context.ReverbLevel *= 0.6f; // На улице хвост тише
}

void EnvironmentCalculations::ApplyPsychoacoustics(EnvironmentContext& context)
{
	context.PerceivedReflectivity = powf(context.PhysicalReflectivity, 0.5f);
}

void EnvironmentCalculations::CalculateEnvironmentalProperties(EnvironmentContext& context)
{
	CalculateGeometricAcoustics(context);
	CalculateMaterialProperties(context);
	CalculateRoomAcoustics(context);
	ApplyPsychoacoustics(context);
}

void EnvironmentCalculations::CalculateEAXParameters(EnvironmentContext& context)
{
	SEAXEnvironmentData& eax = context.EAXData;

	// === ПОЛУЧЕНИЕ ДАННЫХ О ТУМАНЕ ===
	float fog_density = 0.0f;
	if (g_pGamePersistent && g_pGamePersistent->Environment().CurrentEnv)
	{
		fog_density = g_pGamePersistent->Environment().CurrentEnv->fog_density;
	}
	// Обычно fog_density идет от 0 до 1, но на всякий случай клэмп
	fog_density = clampr(fog_density, 0.0f, 1.0f);

	// =========================================================================
	// 1. ROOM (MASTER VOLUME)
	// =========================================================================
	float fRoomIntensity = 0.5f + (0.5f * context.GeometricEnclosedness);
	fRoomIntensity *= context.PerceivedReflectivity;

	// Чем плотнее туман, тем тише распространяется звук (общий демпфинг)
	// Максимальный туман снизит громкость реверберации на ~500 mB
	float fog_attenuation = fog_density * 0.2f;

	eax.lRoom = (LONG)EnvironmentContext::Lerp(-3000.0f, -300.0f, fRoomIntensity - fog_attenuation);

	// [FIX] Small Room Correction
	// Если комната очень маленькая (< 80 m3), делаем её значительно тише,
	// чтобы не было эффекта "ведра на голове".
	if (context.PhysicalVolume < 80.0f)
	{
		eax.lRoom -= 500; // Дополнительное приглушение
	}

	// =========================================================================
	// 2. DECAY TIME
	// =========================================================================
	eax.flDecayTime = context.ReverbTime;

	// В густом тумане хвост реверберации затухает чуть быстрее (влажный воздух)
	if (fog_density > 0.5f)
		eax.flDecayTime *= 0.9f;

	// =========================================================================
	// 3. REFLECTIONS & REVERB
	// =========================================================================
	float fReflIntensity = context.PhysicalReflectivity;

	// Ранние отражения
	eax.lReflections = (LONG)EnvironmentContext::Lerp(-2500.0f, -100.0f, fReflIntensity);
	eax.flReflectionsDelay = clampr(context.MeanFreePath / 340.0f, 0.0f, 0.3f);

	// Хвост реверберации (чуть тише ранних)
	float fReverbIntensity = fRoomIntensity;
	eax.lReverb = (LONG)EnvironmentContext::Lerp(-3000.0f, -300.0f, fReverbIntensity);

	// Коррекция для маленьких комнат (хвост там почти отсутствует)
	if (context.PhysicalVolume < 80.0f)
	{
		eax.lReverb -= 800;
	}

	eax.flReverbDelay = clampr(eax.flReflectionsDelay + 0.04f, 0.0f, 0.1f);

	// =========================================================================
	// 4. ROOM HF & FOG ABSORPTION
	// =========================================================================
	// Базовое поглощение материалами
	float hf_loss = 0.0f;
	if (context.MaterialHits[MATERIAL_SOFT] > context.TotalHits / 2)
		hf_loss = -2000.0f;
	else if (context.MaterialHits[MATERIAL_WOOD] > context.TotalHits / 2)
		hf_loss = -1000.0f;
	else
		hf_loss = -300.0f;

	// [FEATURE] FOG EFFECT
	// Туман сильно "ест" высокие частоты.
	// При fog=1.0 добавляем -2000 mB к HF (звук станет очень глухим).
	float fog_hf_loss = fog_density * -2000.0f;

	eax.lRoomHF = (LONG)(eax.lRoom + hf_loss + fog_hf_loss);
	eax.lRoomHF = _max(eax.lRoomHF, -10000);

	// =========================================================================
	// 5. OTHER PARAMS
	// =========================================================================
	eax.flRoomRolloffFactor = 0.0f;

	eax.flDecayHFRatio = 0.83f;
	if (context.MaterialHits[MATERIAL_METAL] > 0)
		eax.flDecayHFRatio = 1.3f;
	else if (context.MaterialHits[MATERIAL_SOFT] > 0)
		eax.flDecayHFRatio = 0.6f;

	// Туман также уменьшает соотношение затухания ВЧ
	if (fog_density > 0.3f)
		eax.flDecayHFRatio *= 0.8f;

	eax.flEnvironmentSize = powf(context.PhysicalVolume, 1.0f / 3.0f);
	eax.flEnvironmentSize = clampr(eax.flEnvironmentSize, 1.0f, 100.0f);
	eax.flEnvironmentDiffusion = 1.0f;

	// Air Absorption: Базовое -5.0. Туман может опустить до -10.0 (сильное поглощение).
	eax.flAirAbsorptionHF = -5.0f - (fog_density * 5.0f);

	// Флаги (Без авто-скейлинга!)
	eax.dwFlags = NULL;//EAXLISTENERFLAGS_DECAYHFLIMIT;

#ifdef DEBUG
	// Можно раскомментировать для проверки влияния тумана
	// Msg("[EAX] Fog: %.2f | RoomHF: %d | Decay: %.2f", fog_density, eax.lRoomHF, eax.flDecayTime);
#endif
}

bool EnvironmentCalculations::ValidatePhysicalContext(const EnvironmentContext& context)
{
	return true;
}
