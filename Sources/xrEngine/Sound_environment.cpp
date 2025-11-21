///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment.cpp
// Created: 22.10.2025
// Modified: 21.11.2025 (Integrated Physics Update)
// Author: NSDeathman
// Path tracing EAX
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

namespace Internal
{
	float EyringReverberationTime(float volume, float surface_area, float absorption);
}

// Include Detailed Directions array here if not in common.h (usually defined in cpp or separate header)
extern const Fvector DetailedSphereDirections[]; // Assuming it's defined elsewhere or included

CSoundEnvironment::CSoundEnvironment() : m_LastUpdatedFrame(0)
{
	ZeroMemory(&m_CurrentData, sizeof(m_CurrentData));
	ZeroMemory(&m_PrevData, sizeof(m_PrevData));
	m_CurrentData.Reset();
}

CSoundEnvironment::~CSoundEnvironment()
{
}

float CSoundEnvironment::PerformDetailedRaycast(Fvector start, Fvector dir, float max_dist, u32& material_type)
{
	collide::rq_result hit;
	CObject* ViewEntity = g_pGameLevel->CurrentViewEntity();

	// X-Ray native raycast
	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, max_dist, collide::rqtStatic, hit, ViewEntity);

	if (bHit && hit.range > 0.1f)
	{
		// Calculate normal using geometry (if available) or reverse direction
		Fvector hit_normal;
		// Trying to get actual triangle for normal
		CDB::TRI* tri = g_pGameLevel->ObjectSpace.GetStaticTris() + hit.element;
		Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
		hit_normal.mknormal(verts[tri->verts[0]], verts[tri->verts[1]], verts[tri->verts[2]]);

		float cos_angle = _abs(dir.dotproduct(hit_normal));

		// === UPDATED MATERIAL LOGIC (Matches Standalone Tests) ===
		// Default logic: Walls are Stone/Concrete, Floor/Ceiling might vary.

		if (hit.range < 2.0f)
		{
			// Very close - detailed check
			if (cos_angle > 0.9f)
				material_type = MATERIAL_STONE;
			else if (cos_angle > 0.7f)
				material_type = MATERIAL_METAL;
			else
				material_type = MATERIAL_WOOD;
		}
		else if (hit.range < 10.0f)
		{
			// Medium range
			if (cos_angle > 0.8f)
				material_type = MATERIAL_STONE;
			else if (cos_angle > 0.5f)
				material_type = MATERIAL_WOOD;
			else
				material_type = MATERIAL_SOFT;
		}
		else if (hit.range < 50.0f)
		{
			// Far surfaces - likely structural (Stone/Concrete)
			if (cos_angle > 0.6f)
				material_type = MATERIAL_STONE;
			else
				material_type = MATERIAL_WOOD;
		}
		else
		{
			// Very far
			if (hit.range < max_dist * 0.9f)
				material_type = MATERIAL_STONE; // Far buildings
			else
				material_type = MATERIAL_GLASS; // Sky/Void
		}

		return hit.range;
	}

	material_type = MATERIAL_AIR;
	return max_dist;
}

void CSoundEnvironment::GatherRaycastData(EnvironmentContext& context)
{
	// Use detailed directions
	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		// Assuming DetailedSphereDirections is available globally or as a static member
		Fvector RayDirection = DetailedSphereDirections[i];
		u32 material_type = 0;

		float distance =
			PerformDetailedRaycast(context.CameraPosition, RayDirection, SMART_RAY_DISTANCE, material_type);

		context.RaycastDistances.push_back(distance);
		context.TotalDistance += distance;

		if (material_type > MATERIAL_AIR)
		{
			context.TotalHits++;
			// Ensure index bounds
			if (material_type < 6)
			{
				context.MaterialHits[material_type]++;
				context.MaterialDistances[material_type] += distance;
			}
		}
	}
}

void CSoundEnvironment::CalculateEnvironmentData()
{
	EnvironmentContext context;
	context.CameraPosition = Device.vCameraPosition;

	// 1. Run subsystems
	m_PathTracer.PerformPathTracingAnalysis(context.CameraPosition, context.PathTracingResult);
	m_GeometryAnalyzer.AnalyzeEnvironmentGeometry(context.CameraPosition, context.GeometryAnalysis);

	// 2. Gather Raycast Data
	GatherRaycastData(context);

	// 3. Execute NEW PHYSICS PIPELINE
	// Note: We map the namespace functions from the standalone code to the context here
	EnvironmentCalculations::CalculateEnvironmentalProperties(context);
	EnvironmentCalculations::CalculateEAXParameters(context);

	// 4. Validate
	if (!EnvironmentCalculations::ValidatePhysicalContext(context))
	{
		Msg("! [SoundEnv] Validation failed. Vol: %.1f", context.PhysicalVolume);
		context.EAXData.Reset();
	}

	// 5. Finalize
	context.EAXData.dwFrameStamp = Device.dwFrame;
	context.EAXData.bDataValid = true;

	m_PrevData.CopyFrom(m_CurrentData);
	m_CurrentData.CopyFrom(context.EAXData);

	// Debug output
#ifdef DEBUG
	Msg("[EAX Physics] Vol=%.1f m3, RT60=%.2fs, Room=%d, RoomHF=%d", context.PhysicalVolume, context.ReverbTime,
		context.EAXData.lRoom, context.EAXData.lRoomHF);
#endif

	Msg("============== [EAX ENVIRONMENT DEBUG] ==============");

	// 1. ГЕОМЕТРИЯ: Что видят лучи?
	// Vol: Объем. Если он маленький в большом зале -> лучи короткие или упираются в препятствия.
	// MFP: Средняя длина луча.
	// Encl: Замкнутость (0.0 - поле, 1.0 - бункер).
	Msg("[1. GEO  ] Vol: %7.1f m3 | MeanDist: %5.2f m | Enclosed: %4.2f | Hits: %d/%d", context.PhysicalVolume,
		context.MeanFreePath, context.GeometricEnclosedness, context.TotalHits, DETAILED_DIRECTIONS_COUNT);

	// 2. МАТЕРИАЛЫ: Из чего сделаны стены?
	// Важно проверить, определяет ли он Stone/Concrete, или все считает Air/Default.
	Msg("[2. MATS ] Stone: %d | Metal: %d | Wood: %d | Soft: %d | Air: %d", context.MaterialHits[MATERIAL_STONE],
		context.MaterialHits[MATERIAL_METAL], context.MaterialHits[MATERIAL_WOOD], context.MaterialHits[MATERIAL_SOFT],
		(DETAILED_DIRECTIONS_COUNT - context.TotalHits));

	// 3. АКУСТИКА (РАСЧЕТНАЯ): Что насчитала физика?
	// PhysRefl: Насколько стены отражают звук (0.1 - вата, 0.9 - зеркало).
	// CalcRT60: Чистое время реверберации по формуле (без EAX лимитов).
	Msg("[3. CALC ] PhysRefl: %4.2f | CalcRT60: %4.2f sec", context.PhysicalReflectivity,
		Internal::EyringReverberationTime(context.PhysicalVolume, 6.0f * powf(context.PhysicalVolume, 2.0f / 3.0f),
										  1.0f - context.PhysicalReflectivity));

	// 4. EAX ВЫХОД: Что уходит в драйвер?
	Msg("[4. FINAL] Decay: %4.2f s | Room: %d mB | Refl: %d mB | Rev: %d mB", context.EAXData.flDecayTime,
		context.EAXData.lRoom, context.EAXData.lReflections, context.EAXData.lReverb);

	Msg("=====================================================");

	if (::Sound)
		::Sound->commit_eax(&m_CurrentData);
}

void __stdcall CSoundEnvironment::MT_CALC()
{
	CalculateEnvironmentData();
}

bool CSoundEnvironment::NeedUpdate() const
{
	if (!psSoundFlags.test(ss_EAX))
		return false;
	if (Device.dwFrame < (m_LastUpdatedFrame + UPDATE_INTERVAL))
		return false;
	return true;
}

void CSoundEnvironment::Update()
{
	if (NeedUpdate())
	{
		Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CSoundEnvironment::MT_CALC));
		m_LastUpdatedFrame = Device.dwFrame;
	}
}
