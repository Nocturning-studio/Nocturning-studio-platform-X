///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment.cpp
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
#include "Sound_environment.h"

CSoundEnvironment::CSoundEnvironment() : m_LastUpdatedFrame(0), m_LastEnclosedness(0.5f)
{
	ZeroMemory(&m_CurrentData, sizeof(m_CurrentData));
	ZeroMemory(&m_PrevData, sizeof(m_PrevData));
	m_CurrentData.Reset();
	m_Context.Reset();
}

CSoundEnvironment::~CSoundEnvironment()
{
	m_Thread.Stop();
}

void CSoundEnvironment::OnLevelLoad()
{
	m_Thread.Start();
}

void CSoundEnvironment::OnLevelUnload()
{
	m_Thread.Stop();
}

template <typename T> T EAXLerp(T current, T target, float step)
{
	float diff = (float)(target - current);
	if (_abs(diff) < 0.05f)
		return target;
	return (T)(current + diff * step);
}

void SmoothEAXData(SEAXEnvironmentData& current, const SEAXEnvironmentData& target, float speed_factor)
{
	current.lRoom = EAXLerp(current.lRoom, target.lRoom, speed_factor);
	current.lRoomHF = EAXLerp(current.lRoomHF, target.lRoomHF, speed_factor);
	current.lReflections = EAXLerp(current.lReflections, target.lReflections, speed_factor);
	current.lReverb = EAXLerp(current.lReverb, target.lReverb, speed_factor);

	current.flDecayTime = EAXLerp(current.flDecayTime, target.flDecayTime, speed_factor);
	current.flReflectionsDelay = EAXLerp(current.flReflectionsDelay, target.flReflectionsDelay, speed_factor);
	current.flReverbDelay = EAXLerp(current.flReverbDelay, target.flReverbDelay, speed_factor);

	current.flEnvironmentSize = EAXLerp(current.flEnvironmentSize, target.flEnvironmentSize, speed_factor);
	current.flEnvironmentDiffusion =
		EAXLerp(current.flEnvironmentDiffusion, target.flEnvironmentDiffusion, speed_factor);
	current.flAirAbsorptionHF = EAXLerp(current.flAirAbsorptionHF, target.flAirAbsorptionHF, speed_factor);

	current.flDecayHFRatio = target.flDecayHFRatio;
	current.dwFlags = target.dwFlags;
	current.flRoomRolloffFactor = target.flRoomRolloffFactor;
}

void CSoundEnvironment::ProcessNewData()
{
	EnvironmentCalculations::CalculateEnvironmentalProperties(m_Context);
	EnvironmentCalculations::CalculateEAXParameters(m_Context);

	m_Context.EAXData.dwFrameStamp = Device.dwFrame;
	m_Context.EAXData.bDataValid = true;

	// Shock Detection: если резко вышли/вошли, ускоряем переход
	float encl_diff = _abs(m_Context.GeometricEnclosedness - m_LastEnclosedness);
	float lerp_speed = 0.1f;

	if (encl_diff > 0.15f)
		lerp_speed = 0.7f;

	m_LastEnclosedness = m_Context.GeometricEnclosedness;

	if (m_CurrentData.lRoom <= -9000)
		m_CurrentData.CopyFrom(m_Context.EAXData);
	else
		SmoothEAXData(m_CurrentData, m_Context.EAXData, lerp_speed);

	m_PrevData.CopyFrom(m_CurrentData);

	if (::Sound)
		::Sound->commit_eax(&m_CurrentData);
}

void CSoundEnvironment::Update()
{
	if (!psSoundFlags.test(ss_EAX))
		return;

	if (m_Thread.GetResult(m_Context))
	{
		ProcessNewData();
		m_LastUpdatedFrame = Device.dwFrame;
	}

	if (!m_Thread.IsCalculating() && Device.dwFrame > (m_LastUpdatedFrame + UPDATE_INTERVAL))
	{
		m_Thread.RequestUpdate(Device.vCameraPosition);
	}
}
