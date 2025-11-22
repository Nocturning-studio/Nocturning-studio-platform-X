///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_pathtracing.cpp
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop
#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "igame_level.h"
#include "xr_area.h"
#include "xr_object.h"
#include "xr_collide_defs.h"
#endif
#include "Sound_environment_pathtracing.h"

const float CPathTracingSystem::ENERGY_THRESHOLD = 0.05f;
const float CPathTracingSystem::MAX_RAY_DIST = 200.0f;

extern const Fvector3 DetailedSphereDirections[];
extern const u32 DETAILED_DIRECTIONS_COUNT;

Fvector CPathTracingSystem::RandomHemisphereDir(Fvector normal)
{
	Fvector dir;
	dir.random_dir(normal, PI_DIV_2); // Cosine weighted distribution
	return dir;
}

void CPathTracingSystem::TraceRecursive(Fvector start, Fvector dir, int depth, float current_energy,
										EnvironmentContext& ctx)
{
	if (depth >= MAX_BOUNCES || current_energy < ENERGY_THRESHOLD)
		return;

	collide::rq_result hit;
	BOOL bHit = g_pGameLevel->ObjectSpace.RayPick(start, dir, MAX_RAY_DIST, collide::rqtStatic, hit, NULL);

	float dist = (bHit) ? hit.range : MAX_RAY_DIST;

	// Взвешиваем дистанцию энергией (чем слабее луч, тем меньше вклад)
	ctx.Accum_TotalDistance += dist * current_energy;

	// === ВАЖНО: Считаем Enclosedness (стены) только для первого луча ===
	if (depth == 0)
	{
		if (bHit)
			ctx.Accum_TotalHits++;
	}

	if (!bHit)
		return; // Улетел в небо - конец пути

	// === Материалы собираем со всех отскоков ===
	CDB::TRI* tri = g_pGameLevel->ObjectSpace.GetStaticTris() + hit.element;
	Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
	Fvector normal;
	normal.mknormal(verts[tri->verts[0]], verts[tri->verts[1]], verts[tri->verts[2]]);

	u32 mat_type = MATERIAL_STONE;
	float cos_angle = _abs(dir.dotproduct(normal));

	if (dist < 2.0f && cos_angle > 0.6f)
		mat_type = MATERIAL_STONE;
	else if (dist < 15.0f && cos_angle > 0.4f)
		mat_type = MATERIAL_WOOD;
	else if (dist > 80.0f)
		mat_type = MATERIAL_STONE;
	else
		mat_type = MATERIAL_SOFT;

	if (mat_type < 6)
		ctx.Accum_MaterialHits[mat_type]++;

	// === Расчет поглощения для следующего отскока ===
	float absorption = 0.2f;
	switch (mat_type)
	{
	case MATERIAL_METAL:
		absorption = 0.05f;
		break;
	case MATERIAL_STONE:
		absorption = 0.15f;
		break;
	case MATERIAL_WOOD:
		absorption = 0.40f;
		break;
	case MATERIAL_SOFT:
		absorption = 0.80f;
		break;
	case MATERIAL_GLASS:
		absorption = 0.05f;
		break;
	}

	float new_energy = current_energy * (1.0f - absorption);

	// Новый луч
	Fvector new_pos;
	new_pos.mad(start, dir, dist - 0.05f);
	Fvector bounce_dir = RandomHemisphereDir(normal);

	TraceRecursive(new_pos, bounce_dir, depth + 1, new_energy, ctx);
}

void CPathTracingSystem::PerformPathTracingAnalysis(EnvironmentContext& ctx)
{
	for (u32 i = 0; i < DETAILED_DIRECTIONS_COUNT; ++i)
	{
		TraceRecursive(ctx.CameraPosition, DetailedSphereDirections[i], 0, 1.0f, ctx);
	}
}
