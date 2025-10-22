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

void CSoundRender_Core::commit_eax(SEAXEnvironmentData* EAXEnvData)
{
	const SEAXEnvironmentData& env = *EAXEnvData;

	EAXLISTENERPROPERTIES ep;
	ZeroMemory(&ep, sizeof(ep));

	// Direct mapping - no calculations here!
	ep.lRoom = env.lRoom;
	ep.lRoomHF = env.lRoomHF;
	ep.flRoomRolloffFactor = env.flRoomRolloffFactor;
	ep.flDecayTime = env.flDecayTime;
	ep.flDecayHFRatio = env.flDecayHFRatio;
	ep.lReflections = env.lReflections;
	ep.flReflectionsDelay = env.flReflectionsDelay;
	ep.lReverb = env.lReverb;
	ep.flReverbDelay = env.flReverbDelay;
	ep.flEnvironmentSize = env.flEnvironmentSize;
	ep.flEnvironmentDiffusion = env.flEnvironmentDiffusion;
	ep.flAirAbsorptionHF = env.flAirAbsorptionHF;
	ep.dwFlags = env.dwFlags;

	u32 deferred = bDeferredEAX ? DSPROPERTY_EAXLISTENER_DEFERRED : 0;

	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOM, &ep.lRoom, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMHF, &ep.lRoomHF, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR,
			  &ep.flRoomRolloffFactor, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYTIME, &ep.flDecayTime,
			  sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_DECAYHFRATIO, &ep.flDecayHFRatio,
			  sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONS, &ep.lReflections,
			  sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY,
			  &ep.flReflectionsDelay, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERB, &ep.lReverb, sizeof(LONG));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_REVERBDELAY, &ep.flReverbDelay,
			  sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION,
			  &ep.flEnvironmentDiffusion, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF,
			  &ep.flAirAbsorptionHF, sizeof(float));
	i_eax_set(&DSPROPSETID_EAX_ListenerProperties, deferred | DSPROPERTY_EAXLISTENER_FLAGS, &ep.dwFlags, sizeof(DWORD));

	if (bDeferredEAX)
		i_eax_set(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS, NULL, 0);
}