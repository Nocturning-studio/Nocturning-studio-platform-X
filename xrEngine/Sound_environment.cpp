///////////////////////////////////////////////////////////////////////////////////
// Created: 10.12.2024
// Author: Deathman
// Nocturning studio for NS Project X
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
#define DIRECTIONS_COUNT 21
#define PI 3.14159265358979323846f

#define SKIP_UPDATE_FRAMES_COUNT 30
///////////////////////////////////////////////////////////////////////////////////
static const Fvector3 HemisphereDirectionsArray[DIRECTIONS_COUNT] = 
{
	{-0.809017, -0.309017,  0.500000},
	{-0.500000, -0.809017,  0.309017},
	{-0.309017, -0.500000,  0.809017},
	{-0.525731,  0.000000,  0.850651},
	{ 0.000000, -0.850651,  0.525731},
	{-0.309017,  0.500000,  0.809017},
	{-0.500000,  0.809017,  0.309017},
	{-0.809017,  0.309017,  0.500000},
	{ 0.000000,  0.850651,  0.525731},
	{-1.000000,  0.000000,  0.000000},
	{ 0.000000, -1.000000,  0.000000},
	{ 0.500000, -0.809017,  0.309017},
	{ 0.809017, -0.309017,  0.500000},
	{ 0.309017, -0.500000,  0.809017},
	{ 0.525731,  0.000000,  0.850651},
	{ 0.000000,  0.000000,  1.000000},
	{ 0.309017,  0.500000,  0.809017},
	{ 0.809017,  0.309017,  0.500000},
	{ 0.500000,  0.809017,  0.309017},
	{ 1.000000,  0.000000,  0.000000},
	{ 0.000000,  1.000000,  0.000000},
};
///////////////////////////////////////////////////////////////////////////////////
CSoundEnvironment::CSoundEnvironment()
{
	m_EnvironmentRadius = -1.0f;
	m_PositionPrev = {0, 0, 0};
	m_LastUpdatedFrame = 0;
	m_FogDensityPrev = 0.0f;
}

CSoundEnvironment::~CSoundEnvironment()
{
}
///////////////////////////////////////////////////////////////////////////////////
/*----------------------------------------
Tracing of rays on a sphere around the 
camera position, with subsequent averaging 
of data on the distance of intersection 
of rays with the geometry into the 
radius of the conditional sphere
----------------------------------------*/
void CSoundEnvironment::CalcAverageEnvironmentRadius()
{
	Fvector RayPosition = Device.vCameraPosition;
	float MaxDistance = g_pGamePersistent->Environment().CurrentEnv->far_plane;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	float AverageIntersectionDistance = 0.0f;

	for (int ArrayIterator = 0; ArrayIterator < DIRECTIONS_COUNT; ArrayIterator++)
	{
		Fvector3 SampleRayDirection = HemisphereDirectionsArray[ArrayIterator];
		collide::rq_result SampleRayQuaryResult;
		BOOL bRayIntersected = g_pGameLevel->ObjectSpace.RayPick(RayPosition, SampleRayDirection, MaxDistance, collide::rqtStatic, SampleRayQuaryResult, ViewEntity);
		AverageIntersectionDistance += SampleRayQuaryResult.range / DIRECTIONS_COUNT;
	}

	m_EnvironmentRadius = AverageIntersectionDistance;
}

/*----------------------------------------
Send method as task to 
X-Ray Secondary thread 
----------------------------------------*/
void __stdcall CSoundEnvironment::MT_CALC()
{
	CalcAverageEnvironmentRadius();
}

void CSoundEnvironment::Update()
{
	bool bNeedToUpdate = NeedUpdate();
	if (bNeedToUpdate)
	{
		Msg("bNeedToUpdate");
		Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CSoundEnvironment::MT_CALC));

		::Sound->set_environment_radius(m_EnvironmentRadius);

		float FogDensity = g_pGamePersistent->Environment().CurrentEnv->fog_density;
		::Sound->set_environment_fog_density(FogDensity);

		m_PositionPrev = Device.vCameraPosition;
		m_LastUpdatedFrame = Device.dwFrame;
		m_FogDensityPrev = FogDensity;
	}
	::Sound->set_need_update_environment(bNeedToUpdate);
}

bool CSoundEnvironment::NeedUpdate()
{
	float FogDensity = g_pGamePersistent->Environment().CurrentEnv->fog_density;
	if (Device.dwFrame > (m_LastUpdatedFrame + SKIP_UPDATE_FRAMES_COUNT))
		if (!(Device.vCameraPosition.similar(m_PositionPrev)) || !(FogDensity == m_FogDensityPrev))
			return true;

	return false;
}
///////////////////////////////////////////////////////////////////////////////////
