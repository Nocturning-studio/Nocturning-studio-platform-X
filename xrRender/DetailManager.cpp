// DetailManager.cpp: implementation of the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "DetailManager.h"
#include "cl_intersect.h"

#ifdef _EDITOR
#include "ESceneClassList.h"
#include "Scene.h"
#include "SceneObject.h"
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif

const float dbgOffset = 0.f;
const int dbgItems = 128;

//--------------------------------------------------- Decompression
static int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};

void bwdithermap(int levels, int magic[16][16])
{
	/* Get size of each step */
	float N = 255.0f / (levels - 1);

	float magicfact = (N - 1) / 16;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				for (int l = 0; l < 4; l++)
					magic[4 * k + i][4 * l + j] =
						(int)(0.5 + magic4x4[i][j] * magicfact + (magic4x4[k][l] / 16.) * magicfact);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDetailManager::CDetailManager()
{
	dtFS = 0;
	dtSlots = 0;
	hw_Geom = 0;
	hw_Geom_Instanced = 0;
	hw_BatchSize = 0;
	hw_VB = 0;
	hw_VB_Instances = 0;
	hw_IB = 0;
}

CDetailManager::~CDetailManager()
{
}

#ifndef _EDITOR

void CDetailManager::Load()
{
	// Open file stream
	if (!FS.exist("$level$", "level.details"))
	{
		dtFS = NULL;
		return;
	}

	string_path fn;
	FS.update_path(fn, "$level$", "level.details");
	dtFS = FS.r_open(fn);

	// Header
	dtFS->r_chunk_safe(0, &dtH, sizeof(dtH));
	R_ASSERT(dtH.version == DETAIL_VERSION);
	u32 m_count = dtH.object_count;

	// Models
	IReader* m_fs = dtFS->open_chunk(1);
	for (u32 m_id = 0; m_id < m_count; m_id++)
	{
		CDetail* dt = xr_new<CDetail>();
		IReader* S = m_fs->open_chunk(m_id);
		dt->Load(S);
		objects.push_back(dt);
		S->close();
	}
	m_fs->close();

	// Get pointer to database (slots)
	IReader* m_slots = dtFS->open_chunk(2);
	dtSlots = (DetailSlot*)m_slots->pointer();
	m_slots->close();

	// Initialize 'vis' and 'cache'
	for (u32 i = 0; i < 3; ++i)
		m_visibles[i].resize(objects.size());
	cache_Initialize();

	// Make dither matrix
	bwdithermap(2, dither);

	// Hardware resources
	hw_Load();
}
#endif

void CDetailManager::Unload()
{
	hw_Unload();

	for (DetailIt it = objects.begin(); it != objects.end(); it++)
	{
		(*it)->Unload();
		xr_delete(*it);
	}
	objects.clear();
	m_visibles[0].clear();
	m_visibles[1].clear();
	m_visibles[2].clear();
	FS.r_close(dtFS);
}

extern ECORE_API float r_ssaDISCARD;

void CDetailManager::UpdateVisibleM()
{
	Fvector EYE = Device.vCameraPosition;

	CFrustum View;
	View.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

	float fade_limit = dm_fade;
	fade_limit = fade_limit * fade_limit;
	float fade_start = 1.f;
	fade_start = fade_start * fade_start;
	float fade_range = fade_limit - fade_start;
	float r_ssaCHEAP = 16 * r_ssaDISCARD;

	Device.Statistic->RenderDUMP_DT_VIS.Begin();
	for (int _mz = 0; _mz < dm_cache1_line; _mz++)
	{
		for (int _mx = 0; _mx < dm_cache1_line; _mx++)
		{
			CacheSlot1& MS = cache_level1[_mz][_mx];
			if (MS.empty)
				continue;
			u32 mask = 0xff;
			u32 res = View.testSAABB(MS.vis.sphere.P, MS.vis.sphere.R, MS.vis.box.data(), mask);
			if (fcvNone == res)
				continue;

			for (int _i = 0; _i < dm_cache1_count * dm_cache1_count; _i++)
			{
				Slot* PS = *MS.slots[_i];
				Slot& S = *PS;

				if (S.empty)
					continue;

				if (fcvPartial == res)
				{
					u32 _mask = mask;
					u32 _res = View.testSAABB(S.vis.sphere.P, S.vis.sphere.R, S.vis.box.data(), _mask);
					if (fcvNone == _res)
						continue;
				}
#ifndef _EDITOR
				if (!RenderImplementation.HOM.visible(S.vis))
					continue;
#endif
				if (Device.dwFrame > S.frame)
				{
					float dist_sq = EYE.distance_to_sqr(S.vis.sphere.P);
					if (dist_sq > fade_limit)
						continue;
					float alpha = (dist_sq < fade_start) ? 0.f : (dist_sq - fade_start) / fade_range;
					float alpha_i = 1.f - alpha;
					float dist_sq_rcp = 1.f / dist_sq;

					S.frame = Device.dwFrame + Random.randI(15, 30);
					for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
					{
						SlotPart& sp = S.G[sp_id];
						if (sp.id == DetailSlot::ID_Empty)
							continue;

						sp.r_items[0].clear();
						sp.r_items[1].clear();
						sp.r_items[2].clear();

						float R = objects[sp.id]->bv_sphere.R;
						float Rq_drcp = R * R * dist_sq_rcp;

						SlotItem **siIT = &(*sp.items.begin()), **siEND = &(*sp.items.end());
						for (; siIT != siEND; siIT++)
						{
							SlotItem& Item = *(*siIT);
							float scale = Item.scale_calculated = Item.scale * alpha_i;
							float ssa = scale * scale * Rq_drcp;
							if (ssa < r_ssaDISCARD)
								continue;
							u32 vis_id = 0;
							if (ssa > r_ssaCHEAP)
								vis_id = Item.vis_ID;

							sp.r_items[vis_id].push_back(*siIT);
						}
					}
				}
				for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
				{
					SlotPart& sp = S.G[sp_id];
					if (sp.id == DetailSlot::ID_Empty)
						continue;
					if (!sp.r_items[0].empty())
						m_visibles[0][sp.id].push_back(&sp.r_items[0]);
					if (!sp.r_items[1].empty())
						m_visibles[1][sp.id].push_back(&sp.r_items[1]);
					if (!sp.r_items[2].empty())
						m_visibles[2][sp.id].push_back(&sp.r_items[2]);
				}
			}
		}
	}
	Device.Statistic->RenderDUMP_DT_VIS.End();
}

void CDetailManager::Render()
{
#ifndef _EDITOR
	if (0 == dtFS)
		return;
	if (!psDeviceFlags.is(rsDetails))
		return;
#endif

	MT_SYNC();

	Device.Statistic->RenderDUMP_DT_Render.Begin();

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_xform_world(Fidentity);

	hw_Render();

	RenderBackend.set_CullMode(CULL_CCW);
	Device.Statistic->RenderDUMP_DT_Render.End();
	m_frame_rendered = Device.dwFrame;
}

void __stdcall CDetailManager::MT_CALC()
{
#ifndef _EDITOR
	if (0 == RenderImplementation.Details)
		return;
	if (0 == dtFS)
		return;
	if (!psDeviceFlags.is(rsDetails))
		return;
#endif

	MT.Enter();
	if (m_frame_calc != Device.dwFrame)
		if ((m_frame_rendered + 1) == Device.dwFrame)
		{
			Fvector EYE = Device.vCameraPosition;
			int s_x = iFloor(EYE.x / dm_slot_size + .5f);
			int s_z = iFloor(EYE.z / dm_slot_size + .5f);

			Device.Statistic->RenderDUMP_DT_Cache.Begin();
			cache_Update(s_x, s_z, EYE, dm_max_decompress);
			Device.Statistic->RenderDUMP_DT_Cache.End();

			UpdateVisibleM();
			m_frame_calc = Device.dwFrame;
		}
	MT.Leave();
}

void CDetailManager::cache_Initialize()
{
	cache_cx = 0;
	cache_cz = 0;

	Slot* slt = cache_pool;
	for (u32 i = 0; i < dm_cache_line; i++)
		for (u32 j = 0; j < dm_cache_line; j++, slt++)
		{
			cache[i][j] = slt;
			cache_Task(j, i, slt);
		}
	VERIFY(cache_Validate());

	for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
	{
		for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
		{
			CacheSlot1& MS = cache_level1[_mz1][_mx1];
			for (int _z = 0; _z < dm_cache1_count; _z++)
				for (int _x = 0; _x < dm_cache1_count; _x++)
					MS.slots[_z * dm_cache1_count + _x] =
						&cache[_mz1 * dm_cache1_count + _z][_mx1 * dm_cache1_count + _x];
		}
	}
}

CDetailManager::Slot* CDetailManager::cache_Query(int r_x, int r_z)
{
	int gx = w2cg_X(r_x + cache_cx);
	VERIFY(gx >= 0 && gx < dm_cache_line);
	int gz = w2cg_Z(r_z + cache_cz);
	VERIFY(gz >= 0 && gz < dm_cache_line);
	return cache[gz][gx];
}

void CDetailManager::cache_Task(int gx, int gz, Slot* D)
{
	int sx = cg2w_X(gx);
	int sz = cg2w_Z(gz);
	DetailSlot& DS = QueryDB(sx, sz);

	D->empty = (DS.id0 == DetailSlot::ID_Empty) && (DS.id1 == DetailSlot::ID_Empty) &&
			   (DS.id2 == DetailSlot::ID_Empty) && (DS.id3 == DetailSlot::ID_Empty);

	u32 old_type = D->type;
	D->type = stPending;
	D->sx = sx;
	D->sz = sz;

	D->vis.box.min.set(sx * dm_slot_size, DS.r_ybase(), sz * dm_slot_size);
	D->vis.box.max.set(D->vis.box.min.x + dm_slot_size, DS.r_ybase() + DS.r_yheight(), D->vis.box.min.z + dm_slot_size);
	D->vis.box.grow(EPS_L);

	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		D->G[i].id = DS.r_id(i);
		for (u32 clr = 0; clr < D->G[i].items.size(); clr++)
			poolSI.destroy(D->G[i].items[clr]);
		D->G[i].items.clear();
	}

	if (old_type != stPending)
	{
		VERIFY(stPending == D->type);
		cache_task.push_back(D);
	}
}

BOOL CDetailManager::cache_Validate()
{
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			int w_x = cg2w_X(x);
			int w_z = cg2w_Z(z);
			Slot* D = cache[z][x];

			if (D->sx != w_x)
				return FALSE;
			if (D->sz != w_z)
				return FALSE;
		}
	}
	return TRUE;
}

void CDetailManager::cache_Update(int v_x, int v_z, Fvector& view, int limit)
{
	bool bNeedMegaUpdate = (cache_cx != v_x) || (cache_cz != v_z);

	while (cache_cx != v_x)
	{
		if (v_x > cache_cx)
		{
			cache_cx++;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = cache[z][0];
				for (int x = 1; x < dm_cache_line; x++)
					cache[z][x - 1] = cache[z][x];
				cache[z][dm_cache_line - 1] = S;
				cache_Task(dm_cache_line - 1, z, S);
			}
		}
		else
		{
			cache_cx--;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = cache[z][dm_cache_line - 1];
				for (int x = dm_cache_line - 1; x > 0; x--)
					cache[z][x] = cache[z][x - 1];
				cache[z][0] = S;
				cache_Task(0, z, S);
			}
		}
	}

	while (cache_cz != v_z)
	{
		if (v_z > cache_cz)
		{
			cache_cz++;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = cache[dm_cache_line - 1][x];
				for (int z = dm_cache_line - 1; z > 0; z--)
					cache[z][x] = cache[z - 1][x];
				cache[0][x] = S;
				cache_Task(x, 0, S);
			}
		}
		else
		{
			cache_cz--;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = cache[0][x];
				for (int z = 1; z < dm_cache_line; z++)
					cache[z - 1][x] = cache[z][x];
				cache[dm_cache_line - 1][x] = S;
				cache_Task(x, dm_cache_line - 1, S);
			}
		}
	}

	BOOL bFullUnpack = FALSE;
	if (cache_task.size() == dm_cache_size)
	{
		limit = dm_cache_size;
		bFullUnpack = TRUE;
	}

	for (int iteration = 0; cache_task.size() && (iteration < limit); iteration++)
	{
		u32 best_id = 0;
		float best_dist = flt_max;

		if (bFullUnpack)
		{
			best_id = cache_task.size() - 1;
		}
		else
		{
			for (u32 entry = 0; entry < cache_task.size(); entry++)
			{
				Slot* S = cache_task[entry];
				VERIFY(stPending == S->type);

				Fvector C;
				S->vis.box.getcenter(C);
				float D = view.distance_to_sqr(C);

				if (D < best_dist)
				{
					best_dist = D;
					best_id = entry;
				}
			}
		}

		cache_Decompress(cache_task[best_id]);
		cache_task.erase(best_id);
	}

	if (bNeedMegaUpdate)
	{
		for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
		{
			for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
			{
				CacheSlot1& MS = cache_level1[_mz1][_mx1];
				MS.empty = TRUE;
				MS.vis.clear();
				for (int _i = 0; _i < dm_cache1_count * dm_cache1_count; _i++)
				{
					Slot* PS = *MS.slots[_i];
					Slot& S = *PS;
					MS.vis.box.merge(S.vis.box);
					if (!S.empty)
						MS.empty = FALSE;
				}
				MS.vis.box.getsphere(MS.vis.sphere.P, MS.vis.sphere.R);
			}
		}
	}
}

DetailSlot& CDetailManager::QueryDB(int sx, int sz)
{
	int db_x = sx + dtH.offs_x;
	int db_z = sz + dtH.offs_z;
	if ((db_x >= 0) && (db_x < int(dtH.size_x)) && (db_z >= 0) && (db_z < int(dtH.size_z)))
	{
		u32 linear_id = db_z * dtH.size_x + db_x;
		return dtSlots[linear_id];
	}
	else
	{
		DS_empty.w_id(0, DetailSlot::ID_Empty);
		DS_empty.w_id(1, DetailSlot::ID_Empty);
		DS_empty.w_id(2, DetailSlot::ID_Empty);
		DS_empty.w_id(3, DetailSlot::ID_Empty);
		return DS_empty;
	}
}

//--------------------------------------------------- Decompression
IC float Interpolate(float* base, u32 x, u32 y, u32 size)
{
	float f = float(size);
	float fx = float(x) / f;
	float ifx = 1.f - fx;
	float fy = float(y) / f;
	float ify = 1.f - fy;

	float c01 = base[0] * ifx + base[1] * fx;
	float c23 = base[2] * ifx + base[3] * fx;
	float c02 = base[0] * ify + base[2] * fy;
	float c13 = base[1] * ify + base[3] * fy;

	float cx = ify * c01 + fy * c23;
	float cy = ifx * c02 + fx * c13;
	return (cx + cy) / 2;
}

IC bool InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size, int dither[16][16])
{
	clamp(x, (u32)0, size - 1);
	clamp(y, (u32)0, size - 1);
	int c = iFloor(Interpolate(alpha255, x, y, size) + .5f);
	clamp(c, 0, 255);

	u32 row = (y + sy) % 16;
	u32 col = (x + sx) % 16;
	return c > dither[col][row];
}

void CDetailManager::cache_Decompress(Slot* S)
{
	VERIFY(S);
	Slot& D = *S;
	D.type = stReady;
	if (D.empty)
		return;

	DetailSlot& DS = QueryDB(D.sx, D.sz);

	Fvector bC, bD;
	D.vis.box.get_CD(bC, bD);

#ifdef _EDITOR
	ETOOLS::box_options(CDB::OPT_FULL_TEST);
	SBoxPickInfoVec pinf;
	Scene->BoxPickObjects(D.vis.box, pinf, GetSnapList());
	u32 triCount = pinf.size();
#else
	xrc.box_options(CDB::OPT_FULL_TEST);
	xrc.box_query(g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);
	u32 triCount = xrc.r_count();
	CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
	Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
#endif

	if (0 == triCount)
		return;

	float alpha255[dm_obj_in_slot][4];
	for (int i = 0; i < dm_obj_in_slot; i++)
	{
		alpha255[i][0] = 255.f * float(DS.palette[i].a0) / 15.f;
		alpha255[i][1] = 255.f * float(DS.palette[i].a1) / 15.f;
		alpha255[i][2] = 255.f * float(DS.palette[i].a2) / 15.f;
		alpha255[i][3] = 255.f * float(DS.palette[i].a3) / 15.f;
	}

	float density = ps_r_Detail_density;
	float jitter = density / 1.7f;
	u32 d_size = iCeil(dm_slot_size / density);
	svector<int, dm_obj_in_slot> selected;

	u32 p_rnd = D.sx * D.sz;
	CRandom r_selection(0x12071980 ^ p_rnd);
	CRandom r_Jitter(0x12071980 ^ p_rnd);
	CRandom r_yaw(0x12071980 ^ p_rnd);
	CRandom r_scale(0x12071980 ^ p_rnd);

	Fbox Bounds;
	Bounds.invalidate();

	for (u32 z = 0; z <= d_size; z++)
	{
		for (u32 x = 0; x <= d_size; x++)
		{
			u32 shift_x = r_Jitter.randI(16);
			u32 shift_z = r_Jitter.randI(16);

			selected.clear();
			if ((DS.id0 != DetailSlot::ID_Empty) &&
				InterpolateAndDither(alpha255[0], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(0);
			if ((DS.id1 != DetailSlot::ID_Empty) &&
				InterpolateAndDither(alpha255[1], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(1);
			if ((DS.id2 != DetailSlot::ID_Empty) &&
				InterpolateAndDither(alpha255[2], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(2);
			if ((DS.id3 != DetailSlot::ID_Empty) &&
				InterpolateAndDither(alpha255[3], x, z, shift_x, shift_z, d_size, dither))
				selected.push_back(3);

			if (selected.empty())
				continue;
			u32 index;
			if (selected.size() == 1)
				index = selected[0];
			else
				index = selected[r_selection.randI(selected.size())];

			CDetail* Dobj = objects[DS.r_id(index)];
			SlotItem* ItemP = poolSI.create();
			SlotItem& Item = *ItemP;

			float rx = (float(x) / float(d_size)) * dm_slot_size + D.vis.box.min.x;
			float rz = (float(z) / float(d_size)) * dm_slot_size + D.vis.box.min.z;
			Fvector Item_P;
			Item_P.set(rx + r_Jitter.randFs(jitter), D.vis.box.max.y, rz + r_Jitter.randFs(jitter));

			float y = D.vis.box.min.y - 5;
			Fvector dir;
			dir.set(0, -1, 0);

			float r_u, r_v, r_range;
			for (u32 tid = 0; tid < triCount; tid++)
			{
#ifdef _EDITOR
				Fvector verts[3];
				SBoxPickInfo& I = pinf[tid];
				for (int k = 0; k < (int)I.inf.size(); k++)
				{
					VERIFY(I.s_obj);
					I.e_obj->GetFaceWorld(I.s_obj->_Transform(), I.e_mesh, I.inf[k].id, verts);
					if (CDB::TestRayTri(Item_P, dir, verts, r_u, r_v, r_range, TRUE))
					{
						if (r_range >= 0)
						{
							float y_test = Item_P.y - r_range;
							if (y_test > y)
								y = y_test;
						}
					}
				}
#else
				CDB::TRI& T = tris[xrc.r_begin()[tid].id];
				Fvector Tv[3] = {verts[T.verts[0]], verts[T.verts[1]], verts[T.verts[2]]};
				if (CDB::TestRayTri(Item_P, dir, Tv, r_u, r_v, r_range, TRUE))
				{
					if (r_range >= 0)
					{
						float y_test = Item_P.y - r_range;
						if (y_test > y)
							y = y_test;
					}
				}
#endif
			}
			if (y < D.vis.box.min.y)
				continue;
			Item_P.y = y;

			Item.scale = r_scale.randF(Dobj->m_fMinScale * 0.5f, Dobj->m_fMaxScale * 0.9f);

			Fmatrix mScale, mXform;
			Fbox ItemBB;
			Item.mRotY.rotateY(r_yaw.randF(0, PI_MUL_2));
			Item.mRotY.translate_over(Item_P);
			mScale.scale(Item.scale, Item.scale, Item.scale);
			mXform.mul_43(Item.mRotY, mScale);
			ItemBB.xform(Dobj->bv_bb, mXform);
			Bounds.merge(ItemBB);

			Item.c_hemi = DS.r_qclr(DS.c_hemi, 15);
			Item.c_sun = DS.r_qclr(DS.c_dir, 15);

			if (Dobj->m_Flags.is(DO_NO_WAVING))
			{
				Item.vis_ID = 0;
			}
			else
			{
				if (::Random.randI(0, 3) == 0)
					Item.vis_ID = 2;
				else
					Item.vis_ID = 1;
			}

			D.G[index].items.push_back(ItemP);
		}
	}

	D.vis.clear();
	D.vis.box.set(Bounds);
	D.vis.box.getsphere(D.vis.sphere.P, D.vis.sphere.R);
}
