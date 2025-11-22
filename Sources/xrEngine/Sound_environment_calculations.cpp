///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_calculations.cpp
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
#include "Sound_environment_calculations.h"
#include "igame_persistent.h"
#include <algorithm>

namespace Internal
{
float EyringReverberationTime(float volume, float surface_area, float absorption)
{
	if (surface_area < 0.1f || absorption >= 0.99f)
		return 0.1f;
	return 0.161f * volume / (-surface_area * logf(1.0f - absorption));
}
} // namespace Internal

void EnvironmentCalculations::CalculateGeometricAcoustics(EnvironmentContext& context)
{
	float num_rays = (float)DETAILED_DIRECTIONS_COUNT;
	if (num_rays < 1.0f)
		num_rays = 1.0f;

	// 1. Mean Free Path
	context.MeanFreePath = context.TotalDistance / num_rays;
	context.MeanFreePath = _max(context.MeanFreePath, 1.0f);

	// 2. Enclosedness
	float hit_ratio = (float)context.TotalHits / num_rays;
	context.GeometricEnclosedness = clampr(hit_ratio, 0.0f, 1.0f);
	context.GeometricOpenness = 1.0f - context.GeometricEnclosedness;

	// 3. Physical Volume
	float estimated_vol = (4.0f / 3.0f) * PI * powf(context.MeanFreePath, 3.0f);

	// Ограничение объема на улице (виртуальный потолок)
	if (context.GeometricOpenness > 0.6f)
	{
		float max_outdoor_vol = 20000.0f;
		estimated_vol = _min(estimated_vol, max_outdoor_vol);
	}
	context.PhysicalVolume = clampr(estimated_vol, 10.0f, 500000.0f);
}

void EnvironmentCalculations::CalculateMaterialProperties(EnvironmentContext& context)
{
	float mat_weights[] = {0.0f, 0.90f, 0.95f, 0.4f, 0.1f, 0.8f};
	float total_mat_hits = 0;
	float accum_refl = 0.0f;

	for (u32 m = 1; m <= 5; ++m)
	{
		total_mat_hits += context.MaterialHits[m];
		accum_refl += context.MaterialHits[m] * mat_weights[m];
	}

	if (total_mat_hits > 0)
		context.PhysicalReflectivity = accum_refl / total_mat_hits;
	else
		context.PhysicalReflectivity = 0.1f;

	context.PhysicalReflectivity = clampr(context.PhysicalReflectivity, 0.05f, 0.98f);
}

void EnvironmentCalculations::CalculateRoomAcoustics(EnvironmentContext& context)
{
	float volume = context.PhysicalVolume;
	float surface_area = 6.0f * powf(volume, 2.0f / 3.0f);
	float abs_coeff = 1.0f - context.PhysicalReflectivity;
	abs_coeff = clampr(abs_coeff, 0.04f, 0.99f);

	float rt60 = Internal::EyringReverberationTime(volume, surface_area, abs_coeff);

	// === AIR ABSORPTION CORRECTION ===
	if (context.GeometricOpenness > 0.4f)
	{
		// На улице RT60 падает быстрее (нет потолка)
		float outdoor_damp = 1.0f - (context.GeometricOpenness - 0.4f) * 1.5f;
		rt60 *= clampr(outdoor_damp, 0.15f, 1.0f);
	}

	context.ReverbTime = clampr(rt60, 0.2f, 6.0f);
}

void EnvironmentCalculations::CalculateEnvironmentalProperties(EnvironmentContext& context)
{
	CalculateGeometricAcoustics(context);
	CalculateMaterialProperties(context);
	CalculateRoomAcoustics(context);
	context.PerceivedReflectivity = powf(context.PhysicalReflectivity, 0.5f);
}

void EnvironmentCalculations::CalculateEAXParameters(EnvironmentContext& context)
{
	SEAXEnvironmentData& eax = context.EAXData;

	float fog_density = 0.0f;
	if (g_pGamePersistent && g_pGamePersistent->Environment().CurrentEnv)
		fog_density = g_pGamePersistent->Environment().CurrentEnv->fog_density;
	fog_density = clampr(fog_density, 0.0f, 1.0f);

	float fOpenness = context.GeometricOpenness;

	// === 1. ROOM (MASTER) ===
	float fRoomIntensity = 0.3f + (0.7f * context.GeometricEnclosedness);
	fRoomIntensity *= sqrtf(context.PerceivedReflectivity);

	// S-Curve Outdoor Falloff
	if (fOpenness > 0.4f)
	{
		float t = (fOpenness - 0.4f) / 0.6f;
		t = clampr(t, 0.0f, 1.0f);
		float damp = 1.0f - (t * t);	 // Quadratic falloff
		damp = clampr(damp, 0.1f, 1.0f); // Min floor
		fRoomIntensity *= damp;
	}

	float fog_attenuation = fog_density * 0.4f;
	fRoomIntensity = clampr(fRoomIntensity, 0.0f, 1.1f);

	// Динамический диапазон: от -4000 (улица) до +400 (бункер)
	eax.lRoom = (LONG)EnvironmentContext::Lerp(-4000.0f, 400.0f, fRoomIntensity - fog_attenuation);

	if (context.PhysicalVolume < 70.0f && context.GeometricEnclosedness > 0.8f)
		eax.lRoom += 500; // Small Room Boost

	eax.lRoom = clampr(eax.lRoom, (LONG)-10000, (LONG)600);

	// === 2. DECAY ===
	eax.flDecayTime = context.ReverbTime;
	if (fog_density > 0.5f)
		eax.flDecayTime *= 0.8f;

	// === 3. REFLECTIONS ===
	float fReflIntensity = context.PhysicalReflectivity;
	float outdoor_refl_scale = (fOpenness > 0.6f) ? 0.8f : 1.0f;

	eax.lReflections = (LONG)EnvironmentContext::Lerp(-2500.0f, 200.0f, fReflIntensity * outdoor_refl_scale);
	eax.lReflections = clampr(eax.lReflections, (LONG)-10000, (LONG)500);

	// Outdoor Delay Boost (Slap-back)
	float base_delay = context.MeanFreePath / 340.0f;
	if (fOpenness > 0.6f)
		base_delay *= 1.5f;
	eax.flReflectionsDelay = clampr(base_delay, 0.0f, 0.3f);

	// === 4. REVERB ===
	eax.lReverb = (LONG)EnvironmentContext::Lerp(-3000.0f, 0.0f, fRoomIntensity);
	if (fOpenness > 0.7f)
		eax.lReverb -= 600;
	eax.lReverb = clampr(eax.lReverb, (LONG)-10000, (LONG)200);
	eax.flReverbDelay = clampr(eax.flReflectionsDelay + 0.04f, 0.0f, 0.1f);

	// === 5. TONE ===
	float hf_loss = -100.0f;
	if (context.MaterialHits[MATERIAL_SOFT] > context.TotalHits / 2)
		hf_loss = -2000.0f;
	else if (context.MaterialHits[MATERIAL_WOOD] > context.TotalHits / 2)
		hf_loss = -600.0f;

	float outdoor_hf_cut = fOpenness * -1200.0f;
	eax.lRoomHF = (LONG)(eax.lRoom + hf_loss + outdoor_hf_cut);
	eax.lRoomHF = clampr(eax.lRoomHF, (LONG)-10000, (LONG)-100);

	eax.flRoomRolloffFactor = 0.0f;
	eax.flDecayHFRatio = (fOpenness > 0.6f) ? 0.5f : 0.83f; // Bass boost outdoors
	if (context.MaterialHits[MATERIAL_METAL] > 0 && fOpenness < 0.5f)
		eax.flDecayHFRatio = 1.3f;

	eax.flEnvironmentDiffusion = (fOpenness > 0.7f) ? 0.6f : 1.0f;
	eax.flAirAbsorptionHF = -5.0f + (fOpenness * -8.0f) - (fog_density * 5.0f);

	eax.flEnvironmentSize = powf(context.PhysicalVolume, 1.0f / 3.0f);
	eax.flEnvironmentSize = clampr(eax.flEnvironmentSize, 1.0f, 100.0f);

	eax.dwFlags = 0x3F;
}

bool EnvironmentCalculations::ValidatePhysicalContext(const EnvironmentContext& context)
{
	return true;
}
