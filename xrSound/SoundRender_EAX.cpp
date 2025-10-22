#include "stdafx.h"
#pragma hdrstop

#include "soundrender_core.h"
#pragma warning(push)
#pragma warning(disable : 4995)
#include <eax/eax.h>
#pragma warning(pop)

void CSoundRender_Core::InitializeEAX()
{
}

#ifndef lerp
#define lerp(a, b, t) ((a) + ((b) - (a)) * (t))
#endif

void CSoundRender_Core::commit_eax(SEAXEnvironmentData* EAXEnvData)
{
	const SEAXEnvironmentData& env = *EAXEnvData;

	float EchoStrength = env.sPhysicalData.fEchoStrength;
	float ReverbTime = env.sPhysicalData.fReverbTime;
	float AdjustedRadius = env.sPhysicalData.fAdjustedRadius;

	float Openness = env.fOpenness;
	float Enclosedness = env.fEnclosedness;

	EAXLISTENERPROPERTIES ep;
	ZeroMemory(&ep, sizeof(ep));

	// СУБТИЛЬНЫЕ ПАРАМЕТРЫ EAX:

	// Room - уменьшаем влияние
	ep.lRoom = (long)lerp(-2000, -500, Enclosedness);

	// RoomHF - больше затухания высоких частот
	ep.lRoomHF = (long)lerp(-1000, -100, Openness);

	// Room Rolloff Factor
	ep.flRoomRolloffFactor = lerp(0.1f, 1.0f, Enclosedness);

	// Decay Time
	ep.flDecayTime = ReverbTime;

	// Decay HF Ratio - больше затухания ВЧ
	ep.flDecayHFRatio = lerp(0.1f, 0.6f, Openness);

	// REFLECTIONS - СИЛЬНО УМЕНЬШАЕМ
	float reflectionStrength = env.sReflectionData.fReflectionEnergy * EchoStrength * 0.5f; // Уменьшили множитель
	clamp(reflectionStrength, 0.0f, 1.0f);

	// ДИНАМИЧЕСКИЙ ДИАПАЗОН ДЛЯ REFLECTIONS
	if (AdjustedRadius < 8.0f)
	{
		// Маленькие помещения: от -100 до 200
		ep.lReflections = (long)lerp(-100, 200, reflectionStrength);
	}
	else if (AdjustedRadius < 25.0f)
	{
		// Средние помещения: от -300 до 100
		ep.lReflections = (long)lerp(-300, 100, reflectionStrength);
	}
	else
	{
		// Большие помещения: от -800 до -200
		ep.lReflections = (long)lerp(-800, -200, reflectionStrength);
	}

	// Reflections Delay
	ep.flReflectionsDelay = env.sReflectionData.fReflectionDelay * (0.3f + 0.7f * Enclosedness);
	clamp(ep.flReflectionsDelay, 0.005f, 0.05f);

	// REVERB - СИЛЬНО УМЕНЬШАЕМ
	ep.lReverb = (long)lerp(-800, 300, EchoStrength * Enclosedness * 0.5f);

	// Reverb Delay
	ep.flReverbDelay = lerp(0.005f, 0.03f, AdjustedRadius / 100.0f);

	// Environment Size
	ep.flEnvironmentSize = AdjustedRadius;
	clamp(ep.flEnvironmentSize, 1.0f, 100.0f);

	// Environment Diffusion
	ep.flEnvironmentDiffusion = lerp(0.2f, 0.8f, 1.0f - Openness);

	// Air Absorption
	ep.flAirAbsorptionHF = lerp(-0.5f, -8.0f, env.fFogDensity);

	// АГРЕССИВНАЯ КОРРЕКЦИЯ ДЛЯ ОТКРЫТЫХ ПРОСТРАНСТВ
	if (Openness > 0.3f)
	{
		float openFactor = (Openness - 0.3f) / 0.7f;

		// СИЛЬНОЕ УМЕНЬШЕНИЕ ВСЕХ ЭФФЕКТОВ
		ep.lReflections = (long)(ep.lReflections * (1.0f - openFactor * 0.9f));
		ep.lReverb = (long)(ep.lReverb * (1.0f - openFactor));
		ep.flDecayTime *= (1.0f - openFactor * 0.6f);
		ep.flEnvironmentDiffusion = lerp(ep.flEnvironmentDiffusion, 0.95f, openFactor);
	}

	ep.lRoom = _max(ep.lRoom, -3000);
	ep.lRoomHF = _max(ep.lRoomHF, -1500);
	ep.lReflections = _max(ep.lReflections, -1500);
	ep.lReverb = _min(_max(ep.lReverb, -1500), 500);
	ep.flDecayTime = _min(ep.flDecayTime, 5.0f);

	u32 deferred = bDeferredEAX ? DSPROPERTY_EAXLISTENER_DEFERRED : 0;

	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR, &ep.flRoomRolloffFactor, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYTIME, &ep.flDecayTime, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYHFRATIO, &ep.flDecayHFRatio, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONS, &ep.lReflections, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY, &ep.flReflectionsDelay, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERBDELAY, &ep.flReverbDelay, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION, &ep.flEnvironmentDiffusion, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF, &ep.flAirAbsorptionHF, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags, sizeof(DWORD));

	if (bDeferredEAX)
		i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS, NULL, 0);
}