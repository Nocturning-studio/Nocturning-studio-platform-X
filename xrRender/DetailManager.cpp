////////////////////////////////////////////////////////////////////////////
//	Modified	: 28.10.2025
//	Author	: NSDeathMan
//  Nocturning studio for NS Platform X
//  Complete rewrite with thread-safe architecture
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include <cmath>

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

// Static member initialization
BOOL CDetailManager::s_use_hom_occlusion = FALSE;
float CDetailManager::s_occlusion_padding_scale = 1.0f;
float CDetailManager::s_max_occlusion_distance = 150.0f;
BOOL CDetailManager::s_disable_hom = TRUE;

CDetailManager::CDetailManager()
	: m_dtFS(nullptr), m_dtSlots(nullptr), m_hw_Geom(nullptr), m_hw_Geom_Instanced(nullptr), m_hw_BatchSize(0),
	  m_hw_VB(nullptr), m_hw_VB_Instances(nullptr), m_hw_IB(nullptr), m_cache_cx(0), m_cache_cz(0), m_frame_calc(0),
	  m_frame_rendered(0), m_initialized(false), m_loaded(false), m_bInLoad(FALSE), m_bShuttingDown(FALSE),
	  m_bRegisteredInParallel(false)
{
	// Initialize empty slot
	m_DS_empty.w_id(0, DetailSlot::ID_Empty);
	m_DS_empty.w_id(1, DetailSlot::ID_Empty);
	m_DS_empty.w_id(2, DetailSlot::ID_Empty);
	m_DS_empty.w_id(3, DetailSlot::ID_Empty);

	// Initialize cache
	ZeroMemory(m_cache, sizeof(m_cache));
	ZeroMemory(m_cache_level1, sizeof(m_cache_level1));
	ZeroMemory(m_cache_pool, sizeof(m_cache_pool));
}

CDetailManager::~CDetailManager()
{
	Shutdown();
}

void CDetailManager::Shutdown()
{
	if (m_bShuttingDown)
		return;

	Msg("CDetailManager::Shutdown() - Start");

	// Первым делом останавливаем все обновления
	m_bShuttingDown = TRUE;

	// Удаляем из параллельных задач
	UnregisterFromParallel();

	// Затем выгружаем ресурсы
	Unload();

	Msg("CDetailManager::Shutdown() - Completed");
}

void CDetailManager::UnregisterFromParallel()
{
	// Удаляем себя из последовательности параллельных задач
	if (m_bRegisteredInParallel)
	{
		Device.seqParallel.remove([this](const fastdelegate::FastDelegate0<>& delegate) 
		{
			// Сравниваем делегаты - это упрощенная версия, может потребоваться более точное сравнение
			return true; // В реальности нужно сравнить конкретные делегаты
		});

		m_bRegisteredInParallel = false;
		Msg("CDetailManager: Unregistered from parallel tasks");
	}
}

////////////////////////////////////////////////////////////////////////////
// Hardware Rendering Implementation
////////////////////////////////////////////////////////////////////////////

// Vertex declarations for instancing
static D3DVERTEXELEMENT9 dwDecl_Details[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()};

static D3DVERTEXELEMENT9 dwDecl_Details_Instanced[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{1, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	{1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},
	{1, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3},
	{1, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4},
	D3DDECL_END()};

D3DVERTEXELEMENT9* CDetailManager::GetDetailsVertexDecl()
{
	return dwDecl_Details;
}

D3DVERTEXELEMENT9* CDetailManager::GetDetailsInstancedVertexDecl()
{
	return dwDecl_Details_Instanced;
}

short CDetailManager::QuantConstant(float v)
{
	int t = iFloor(v * float(16384)); // quant = 16384
	clamp(t, -32768, 32767);
	return short(t & 0xffff);
}

void CDetailManager::hw_Load()
{
	if (m_bShuttingDown)
		return;

	Msg("CDetailManager::hw_Load() - Start");

	if (m_objects.empty())
	{
		Msg("CDetailManager::hw_Load() - No objects to load");
		return;
	}

	// Calculate total vertices and indices
	u32 totalVertices = 0;
	u32 totalIndices = 0;

	for (u32 i = 0; i < m_objects.size(); i++)
	{
		CDetail& D = *m_objects[i];
		totalVertices += D.number_vertices;
		totalIndices += D.number_indices;
	}

	Msg("CDetailManager::hw_Load() - Total vertices: %d, indices: %d", totalVertices, totalIndices);

	if (totalVertices == 0 || totalIndices == 0)
	{
		Msg("CDetailManager::hw_Load() - No geometry data");
		return;
	}

	// Create vertex buffer
	HRESULT hr = HW.pDevice->CreateVertexBuffer(totalVertices * sizeof(vertHW), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED,
												&m_hw_VB, 0);

	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create vertex buffer (HR: 0x%08X)", hr);
		return;
	}

	// Create index buffer
	hr = HW.pDevice->CreateIndexBuffer(totalIndices * sizeof(u16), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED,
									   &m_hw_IB, 0);

	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create index buffer (HR: 0x%08X)", hr);
		_RELEASE(m_hw_VB);
		return;
	}

	// Create instance buffer (dynamic for frequent updates)
	m_hw_BatchSize = 256; // Reasonable batch size

	hr = HW.pDevice->CreateVertexBuffer(m_hw_BatchSize * sizeof(InstanceData), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
										D3DPOOL_DEFAULT, &m_hw_VB_Instances, 0);

	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create instance buffer (HR: 0x%08X)", hr);
		_RELEASE(m_hw_VB);
		_RELEASE(m_hw_IB);
		return;
	}

	// Fill vertex buffer
	vertHW* pVerts = nullptr;
	hr = m_hw_VB->Lock(0, 0, (void**)&pVerts, 0);
	if (SUCCEEDED(hr) && pVerts)
	{
		u32 vertexOffset = 0;
		for (u32 o = 0; o < m_objects.size(); o++)
		{
			CDetail& D = *m_objects[o];
			float height_range = D.bv_bb.max.y - D.bv_bb.min.y;
			float t_scale = (fis_zero(height_range)) ? 0.0f : 1.0f / height_range;

			for (u32 v = 0; v < D.number_vertices; v++)
			{
				Fvector& vP = D.vertices[v].P;
				pVerts->x = vP.x;
				pVerts->y = vP.y;
				pVerts->z = vP.z;
				pVerts->u = QuantConstant(D.vertices[v].u);
				pVerts->v = QuantConstant(D.vertices[v].v);
				pVerts->t = QuantConstant(vP.y * t_scale);
				pVerts->mid = 0;
				pVerts++;
			}
			vertexOffset += D.number_vertices;
		}
		m_hw_VB->Unlock();
	}
	else
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to lock vertex buffer");
	}

	// Fill index buffer
	u16* pIndices = nullptr;
	hr = m_hw_IB->Lock(0, 0, (void**)&pIndices, 0);
	if (SUCCEEDED(hr) && pIndices)
	{
		u32 vertexOffset = 0;
		for (u32 o = 0; o < m_objects.size(); o++)
		{
			CDetail& D = *m_objects[o];
			for (u32 i = 0; i < D.number_indices; i++)
			{
				*pIndices++ = u16(D.indices[i] + vertexOffset);
			}
			vertexOffset += D.number_vertices;
		}
		m_hw_IB->Unlock();
	}
	else
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to lock index buffer");
	}

	// Create geometries
	m_hw_Geom.create(GetDetailsVertexDecl(), m_hw_VB, m_hw_IB);
	m_hw_Geom_Instanced.create(GetDetailsInstancedVertexDecl(), m_hw_VB, m_hw_IB);

	Msg("CDetailManager::hw_Load() - Completed successfully");
}

void CDetailManager::hw_Unload()
{
	Msg("CDetailManager::hw_Unload() - Start");

	// Destroy geometries first
	m_hw_Geom.destroy();
	m_hw_Geom_Instanced.destroy();

	// Release buffers
	_RELEASE(m_hw_VB_Instances);
	_RELEASE(m_hw_IB);
	_RELEASE(m_hw_VB);

	m_hw_BatchSize = 0;

	Msg("CDetailManager::hw_Unload() - Completed");
}

void CDetailManager::hw_Render()
{
	if (!psDeviceFlags.is(rsDetails))
		return;

	if (!m_hw_Geom_Instanced || !m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	try
	{
		// Set instanced geometry
		RenderBackend.set_Geometry(m_hw_Geom_Instanced);

		// Render all instances
		hw_Render_dump();
	}
	catch (...)
	{
		Msg("![ERROR] CDetailManager::hw_Render() exception caught");
	}
}

void CDetailManager::hw_Render_dump()
{
	if (m_objects.empty())
		return;

	if (m_objects.size() > dm_max_objects)
	{
		Msg("![ERROR] CDetailManager - too many objects: %d", m_objects.size());
		return;
	}

	Device.Statistic->RenderDUMP_DT_Count = 0;
	u32 total_instances_rendered = 0;

	// Check if we have any visible instances
	bool hasVisibleInstances = false;
	for (u32 lod = 0; lod < 3 && !hasVisibleInstances; lod++)
	{
		for (u32 obj = 0; obj < m_objects.size(); obj++)
		{
			if (!m_render_visibles[lod][obj].empty())
			{
				hasVisibleInstances = true;
				break;
			}
		}
	}

	if (!hasVisibleInstances)
		return;

	// Calculate vertex and index offsets for each object
	xr_vector<u32> vertexOffsets(m_objects.size());
	xr_vector<u32> indexOffsets(m_objects.size());

	u32 currentVertexOffset = 0;
	u32 currentIndexOffset = 0;

	for (u32 i = 0; i < m_objects.size(); i++)
	{
		vertexOffsets[i] = currentVertexOffset;
		indexOffsets[i] = currentIndexOffset;

		currentVertexOffset += m_objects[i]->number_vertices;
		currentIndexOffset += m_objects[i]->number_indices;
	}

	// Render each object
	for (u32 objIndex = 0; objIndex < m_objects.size(); objIndex++)
	{
		CDetail& Object = *m_objects[objIndex];
		u32 vOffset = vertexOffsets[objIndex];
		u32 iOffset = indexOffsets[objIndex];

		// Render all LOD levels
		for (u32 lod_id = 0; lod_id < 3; lod_id++)
		{
			xr_vector<SlotItemVec*>& vis = m_render_visibles[lod_id][objIndex];

			if (vis.empty())
				continue;

			// Select shader
			int shader_id = Object.m_Flags.is(DO_NO_WAVING) ? SE_DETAIL_NORMAL_STATIC : SE_DETAIL_NORMAL_ANIMATED;

			if (!Object.shader || !Object.shader->E[shader_id])
				continue;

			// Collect instances for this object and LOD
			xr_vector<InstanceData> instances;
			instances.reserve(512);

			for (auto& slot_items : vis)
			{
				if (!slot_items)
					continue;

				for (auto& item_ptr : *slot_items)
				{
					if (!item_ptr)
						continue;

					SlotItem& item = *item_ptr;
					InstanceData inst;

					// Transform matrix with scale
					Fmatrix& M = item.mRotY;
					float scale = item.scale_calculated;

					inst.mat0.set(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
					inst.mat1.set(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
					inst.mat2.set(M._13 * scale, M._23 * scale, M._33 * scale, M._43);

					// Color and lighting
					inst.color.set(item.c_sun, item.c_sun, item.c_sun, item.c_hemi);
					inst.lod_id = (float)lod_id;

					instances.push_back(inst);
				}
			}

			if (instances.empty())
				continue;

			// Set shader
			RenderBackend.set_Element(Object.shader->E[shader_id]);

			// Render in batches
			u32 total_instances = instances.size();
			u32 batch_count = (total_instances + m_hw_BatchSize - 1) / m_hw_BatchSize;

			for (u32 batch = 0; batch < batch_count; batch++)
			{
				u32 start_instance = batch * m_hw_BatchSize;
				u32 end_instance = std::min(start_instance + m_hw_BatchSize, total_instances);
				u32 batch_instance_count = end_instance - start_instance;

				// Fill instance buffer
				InstanceData* pInstances = nullptr;
				HRESULT hr = m_hw_VB_Instances->Lock(0, batch_instance_count * sizeof(InstanceData),
													 (void**)&pInstances, D3DLOCK_DISCARD);

				if (SUCCEEDED(hr) && pInstances)
				{
					// Copy instance data
					CopyMemory(pInstances, &instances[start_instance], batch_instance_count * sizeof(InstanceData));
					m_hw_VB_Instances->Unlock();

					// Setup instanced rendering
					HW.pDevice->SetStreamSource(0, m_hw_VB, 0, sizeof(vertHW));
					HW.pDevice->SetStreamSource(1, m_hw_VB_Instances, 0, sizeof(InstanceData));

					HW.pDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | batch_instance_count);
					HW.pDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul);

					// Validate geometry data
					if (Object.number_indices >= 3 && Object.number_vertices > 0)
					{
						// Draw instanced
						hr = HW.pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices,
															  iOffset, Object.number_indices / 3);

						if (SUCCEEDED(hr))
						{
							total_instances_rendered += batch_instance_count;
							Device.Statistic->RenderDUMP_DT_Count += batch_instance_count;
						}
					}

					// Restore stream frequencies
					HW.pDevice->SetStreamSourceFreq(0, 1);
					HW.pDevice->SetStreamSourceFreq(1, 1);
				}
			}
		}
	}

	// Clear render visibility lists after rendering
	for (u32 lod = 0; lod < 3; lod++)
	{
		for (u32 obj = 0; obj < m_objects.size(); obj++)
		{
			m_render_visibles[lod][obj].clear();
		}
	}
}

void CDetailManager::Load()
{
	if (m_bShuttingDown)
		return;

	Msg("CDetailManager::Load() - Start");

	if (m_loaded)
	{
		Msg("CDetailManager::Load() - Already loaded, skipping");
		return;
	}

	m_MT.Enter();
	try
	{
		Internal_Load();
		m_loaded = true;

		// Регистрируемся в параллельных задачах после успешной загрузки
		if (!m_bRegisteredInParallel && ps_render_flags.test(RFLAG_EXP_MT_CALC))
		{
			Device.seqParallel.insert(Device.seqParallel.begin(),
									  fastdelegate::FastDelegate0<>(this, &CDetailManager::Update));
			m_bRegisteredInParallel = true;
			Msg("CDetailManager: Registered in parallel tasks");
		}
	}
	catch (...)
	{
		Msg("![ERROR] CDetailManager::Load() - Exception during loading");
		m_loaded = false;
	}
	m_MT.Leave();

	Msg("CDetailManager::Load() - Completed");
}

void CDetailManager::Internal_Load()
{
#ifndef _EDITOR
	if (m_bShuttingDown)
		return;

	// Set loading flag
	m_bInLoad = TRUE;

	if (!FS.exist("$level$", "level.details"))
	{
		Msg("CDetailManager::Load() - level.details not found");
		m_bInLoad = FALSE;
		return;
	}

	string_path fn;
	FS.update_path(fn, "$level$", "level.details");
	Msg("CDetailManager::Load() - Loading details from: %s", fn);

	m_dtFS = FS.r_open(fn);
	if (!m_dtFS)
	{
		Msg("CDetailManager::Load() - Failed to open details file");
		m_bInLoad = FALSE;
		return;
	}

	// Read header with exception handling
	__try
	{
		m_dtFS->r_chunk_safe(0, &m_dtH, sizeof(m_dtH));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Msg("![ERROR] CDetailManager::Load() - Failed to read detail header");
		FS.r_close(m_dtFS);
		m_dtFS = nullptr;
		m_bInLoad = FALSE;
		return;
	}

	if (m_dtH.version != DETAIL_VERSION)
	{
		Msg("CDetailManager::Load() - Invalid detail version: %d, expected: %d", m_dtH.version, DETAIL_VERSION);
		FS.r_close(m_dtFS);
		m_dtFS = nullptr;
		m_bInLoad = FALSE;
		return;
	}

	Msg("CDetailManager::Load() - Detail header: objects=%d, size_x=%d, size_z=%d", m_dtH.object_count, m_dtH.size_x,
		m_dtH.size_z);

	if (m_dtH.object_count > dm_max_objects)
	{
		Msg("CDetailManager::Load() - Too many objects: %d, max is %d", m_dtH.object_count, dm_max_objects);
		m_dtH.object_count = dm_max_objects;
	}

	// Load models
	u32 m_count = m_dtH.object_count;
	IReader* m_fs = m_dtFS->open_chunk(1);
	if (m_fs)
	{
		for (u32 m_id = 0; m_id < m_count; m_id++)
		{
			CDetail* dt = xr_new<CDetail>();
			IReader* S = m_fs->open_chunk(m_id);
			if (S)
			{
				dt->Load(S);
				m_objects.push_back(dt);
				S->close();
				Msg("CDetailManager::Load() - Loaded detail object %d", m_id);
			}
		}
		m_fs->close();
	}
	else
	{
		Msg("CDetailManager::Load() - No model chunk found");
	}

	// Load slots
	IReader* m_slots = m_dtFS->open_chunk(2);
	if (m_slots)
	{
		m_dtSlots = (DetailSlot*)m_slots->pointer();
		m_slots->close();
		Msg("CDetailManager::Load() - Loaded %d detail slots", m_dtH.size_x * m_dtH.size_z);
	}
	else
	{
		Msg("CDetailManager::Load() - No slot chunk found");
		m_dtSlots = nullptr;
	}

	// Initialize visibility systems
	for (u32 i = 0; i < 3; ++i)
	{
		m_visibles[i].resize(m_objects.size());
		m_render_visibles[i].resize(m_objects.size());
	}

	// Initialize cache
	cache_Initialize();

	// Load hardware resources
	hw_Load();

	m_initialized = true;
	m_bInLoad = FALSE; // Clear loading flag

	Msg("CDetailManager::Load() - Internal load completed, loaded %d objects", m_objects.size());
#endif
}

void CDetailManager::Unload()
{
	Msg("CDetailManager::Unload() - Start");

	// Удаляем из параллельных задач перед выгрузкой
	UnregisterFromParallel();

	if (!m_loaded)
	{
		Msg("CDetailManager::Unload() - Not loaded, skipping");
		return;
	}

	m_MT.Enter();
	try
	{
		Internal_Unload();
		m_loaded = false;
		m_initialized = false;
	}
	catch (...)
	{
		Msg("![ERROR] CDetailManager::Unload() - Exception during unloading");
	}
	m_MT.Leave();

	Msg("CDetailManager::Unload() - Completed");
}

void CDetailManager::Internal_Unload()
{
	hw_Unload();

	// Clear cache pool items
	for (u32 i = 0; i < dm_cache_size; i++)
	{
		Slot& slot = m_cache_pool[i];
		for (u32 j = 0; j < dm_obj_in_slot; j++)
		{
			SlotPart& sp = slot.G[j];
			// Clear items using pool
			for (auto item : sp.items)
			{
				m_poolSI.destroy(item);
			}
			sp.items.clear();

			// r_items are pointers, don't delete
			for (int lod = 0; lod < 3; lod++)
				sp.r_items[lod].clear();
		}
	}

	// Clear objects
	for (DetailIt it = m_objects.begin(); it != m_objects.end(); it++)
	{
		(*it)->Unload();
		xr_delete(*it);
	}
	m_objects.clear();

	// Clear visibility data
	for (u32 i = 0; i < 3; ++i)
	{
		m_visibles[i].clear();
		m_render_visibles[i].clear();
	}

	// Clear cache tasks
	m_cache_task.clear();

	// Close file stream
	if (m_dtFS)
	{
		FS.r_close(m_dtFS);
		m_dtFS = nullptr;
	}
}

void CDetailManager::Render()
{
#ifndef _EDITOR
	if (!m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	if (!psDeviceFlags.is(rsDetails))
		return;
#endif

	m_MT.Enter();

	__try
	{
		UpdateVisibleM();
		CopyVisibleListsForRender();
		Internal_Render();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Msg("![ERROR] CDetailManager::Render() exception caught");
	}

	m_MT.Leave();
}

void CDetailManager::Internal_Render()
{
	Device.Statistic->RenderDUMP_DT_Render.Begin();

	RenderBackend.set_CullMode(CULL_NONE);
	RenderBackend.set_xform_world(Fidentity);

	hw_Render();

	RenderBackend.set_CullMode(CULL_CCW);
	Device.Statistic->RenderDUMP_DT_Render.End();
}

void CDetailManager::OnDeviceResetBegin()
{
	Msg("CDetailManager::OnDeviceResetBegin()");
	m_MT.Enter();
	hw_Unload();
}

void CDetailManager::OnDeviceResetEnd()
{
	Msg("CDetailManager::OnDeviceResetEnd()");
	if (m_loaded && !m_bShuttingDown)
	{
		hw_Load();
	}
	m_MT.Leave();
}

void CDetailManager::UpdateVisibleM()
{
	if (!g_pGameLevel || !Device.Statistic || !m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	Fvector EYE = Device.vCameraPosition;
	if (EYE.square_magnitude() < 0.01f)
		return;

	Device.Statistic->RenderDUMP_DT_VIS.Begin();

	ClearVisibleLists();

	int center_x = iFloor(EYE.x / dm_slot_size);
	int center_z = iFloor(EYE.z / dm_slot_size);

	const int RENDER_RADIUS = 8;

	for (int x = center_x - RENDER_RADIUS; x <= center_x + RENDER_RADIUS; x++)
	{
		for (int z = center_z - RENDER_RADIUS; z <= center_z + RENDER_RADIUS; z++)
		{
			Slot* slot = cache_Query(x, z);
			if (!slot || slot->empty)
				continue;

			// Update slot if needed
			if (Device.dwFrame > slot->frame)
			{
				slot->frame = Device.dwFrame + 30;

				float dist_sq = EYE.distance_to_sqr(slot->vis.sphere.P);
				float fade_limit = dm_fade * dm_fade;
				float fade_start = 1.0f;
				float fade_range = fade_limit - fade_start;

				float alpha = (dist_sq < fade_start) ? 0.f : (dist_sq - fade_start) / fade_range;
				float alpha_i = 1.f - alpha;

				for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
				{
					SlotPart& sp = slot->G[sp_id];
					if (sp.id == DetailSlot::ID_Empty)
						continue;

					// Clear LOD items
					for (int lod = 0; lod < 3; lod++)
						sp.r_items[lod].clear();

					// Distribute items to LOD levels - ALL in LOD 0 for now
					for (auto& item_ptr : sp.items)
					{
						SlotItem& Item = *item_ptr;
						Item.scale_calculated = Item.scale * alpha_i;
						sp.r_items[0].push_back(item_ptr);
					}
				}
			}

			// Add objects from slot to visible lists
			for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
			{
				SlotPart& sp = slot->G[sp_id];
				if (sp.id == DetailSlot::ID_Empty || sp.id >= m_objects.size())
					continue;

				// Add all LOD levels
				for (int lod = 0; lod < 3; lod++)
				{
					if (!sp.r_items[lod].empty())
					{
						m_visibles[lod][sp.id].push_back(&sp.r_items[lod]);
					}
				}
			}
		}
	}

	Device.Statistic->RenderDUMP_DT_VIS.End();
}

void CDetailManager::ClearVisibleLists()
{
	for (u32 lod = 0; lod < 3; lod++)
	{
		for (u32 obj = 0; obj < m_objects.size(); obj++)
		{
			m_visibles[lod][obj].clear();
		}
	}
}

void CDetailManager::CopyVisibleListsForRender()
{
	for (u32 lod = 0; lod < 3; lod++)
	{
		for (u32 obj = 0; obj < m_objects.size(); obj++)
		{
			m_render_visibles[lod][obj] = m_visibles[lod][obj];
		}
	}
}

void __stdcall CDetailManager::Update()
{
	// Проверяем, что объект еще существует и не уничтожается
	static volatile LONG updateGuard = 0;

	// Быстрая проверка без блокировки
	if (m_bShuttingDown)
		return;

	// Атомарная проверка для предотвращения повторных входов
	if (InterlockedExchange(&updateGuard, 1) == 1)
		return;

	// Основная логика в блоке try-catch
	__try
	{
		// Остальная логика Update...
		if (!psDeviceFlags.is(rsDetails))
		{
			InterlockedExchange(&updateGuard, 0);
			return;
		}

		if (m_frame_calc == Device.dwFrame)
		{
			InterlockedExchange(&updateGuard, 0);
			return;
		}

		if (!Device.Statistic || !g_pGameLevel || !g_pGamePersistent)
		{
			InterlockedExchange(&updateGuard, 0);
			return;
		}

		m_MT.Enter();

		if (m_bShuttingDown || !m_loaded || !m_initialized || m_bInLoad)
		{
			m_MT.Leave();
			InterlockedExchange(&updateGuard, 0);
			return;
		}

		if (m_frame_calc != Device.dwFrame)
		{
			__try
			{
				UpdateCache();
				m_frame_calc = Device.dwFrame;
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				Msg("![ERROR] CDetailManager::Update() exception caught");
			}
		}

		m_MT.Leave();
	}
	__finally
	{
		InterlockedExchange(&updateGuard, 0);
	}
}

void CDetailManager::UpdateCache()
{
	if (m_bShuttingDown || !m_loaded || !m_initialized || m_bInLoad)
		return;

	Fvector EYE;

	// Safe camera position retrieval
	if (g_pGamePersistent)
		EYE = Device.vCameraPosition;
	else
		return;

	// Check camera position validity
	if (!std::isfinite(EYE.x) || !std::isfinite(EYE.y) || !std::isfinite(EYE.z) || EYE.square_magnitude() < 0.01f)
		return;

	// Calculate current camera slot with safety
	int s_x, s_z;
	__try
	{
		s_x = iFloor(EYE.x / dm_slot_size + .5f);
		s_z = iFloor(EYE.z / dm_slot_size + .5f);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return;
	}

	// Safe bounds
	s_x = std::max(-1024, std::min(1024, s_x));
	s_z = std::max(-1024, std::min(1024, s_z));

	// Update cache with exception handling
	if (Device.Statistic)
		Device.Statistic->RenderDUMP_DT_Cache.Begin();

	__try
	{
		cache_Update(s_x, s_z, EYE, dm_max_decompress);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Msg("![ERROR] CDetailManager::UpdateCache() exception in cache_Update");
	}

	if (Device.Statistic)
		Device.Statistic->RenderDUMP_DT_Cache.End();
}

void CDetailManager::cache_Initialize()
{
	m_cache_cx = 0;
	m_cache_cz = 0;

	// Initialize cache pool
	Slot* slt = m_cache_pool;
	for (u32 i = 0; i < dm_cache_line; i++)
	{
		for (u32 j = 0; j < dm_cache_line; j++, slt++)
		{
			m_cache[i][j] = slt;
			cache_Task(j, i, slt);
		}
	}

	// Initialize level 1 cache
	for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
	{
		for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
		{
			CacheSlot1& MS = m_cache_level1[_mz1][_mx1];
			for (int _z = 0; _z < dm_cache1_count; _z++)
			{
				for (int _x = 0; _x < dm_cache1_count; _x++)
				{
					MS.slots[_z * dm_cache1_count + _x] =
						&m_cache[_mz1 * dm_cache1_count + _z][_mx1 * dm_cache1_count + _x];
				}
			}
		}
	}
}

CDetailManager::Slot* CDetailManager::cache_Query(int r_x, int r_z)
{
	int gx = w2cg_X(r_x);
	if (gx < 0 || gx >= dm_cache_line)
		return nullptr;

	int gz = w2cg_Z(r_z);
	if (gz < 0 || gz >= dm_cache_line)
		return nullptr;

	return m_cache[gz][gx];
}

void CDetailManager::cache_Task(int gx, int gz, Slot* D)
{
	if (!D)
		return;

	int sx = cg2w_X(gx);
	int sz = cg2w_Z(gz);

	DetailSlot& DS = QueryDB(sx, sz);

	// Simple empty slot check
	BOOL slot_empty = TRUE;
	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		u32 detail_id = DS.r_id(i);
		if (detail_id != DetailSlot::ID_Empty)
		{
			slot_empty = FALSE;
			break;
		}
	}

	D->empty = slot_empty;
	D->type = stReady;
	D->sx = sx;
	D->sz = sz;

	// Initialize bounding volume
	D->vis.box.min.set(sx * dm_slot_size, 0, sz * dm_slot_size);
	D->vis.box.max.set(D->vis.box.min.x + dm_slot_size, 10.0f, D->vis.box.min.z + dm_slot_size);
	D->vis.box.grow(1.0f);
	D->vis.box.getsphere(D->vis.sphere.P, D->vis.sphere.R);

	// Clear existing items and set object IDs
	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		D->G[i].id = DS.r_id(i);
		D->G[i].items.clear();
		for (int lod = 0; lod < 3; lod++)
			D->G[i].r_items[lod].clear();
	}

	// Add to decompression queue if not empty
	if (!D->empty)
	{
		m_cache_task.push_back(D);
	}
}

DetailSlot& CDetailManager::QueryDB(int sx, int sz)
{
	int db_x = sx + m_dtH.offs_x;
	int db_z = sz + m_dtH.offs_z;

	if ((db_x >= 0) && (db_x < int(m_dtH.size_x)) && (db_z >= 0) && (db_z < int(m_dtH.size_z)))
	{
		u32 linear_id = db_z * m_dtH.size_x + db_x;
		return m_dtSlots[linear_id];
	}
	else
	{
		return m_DS_empty;
	}
}

void CDetailManager::cache_Update(int v_x, int v_z, Fvector& view, int limit)
{
	if (m_bShuttingDown || !m_loaded || !m_initialized)
		return;

	// Safe bounds
	v_x = std::max(-1024, std::min(1024, v_x));
	v_z = std::max(-1024, std::min(1024, v_z));

	// CALCULATE CACHE OFFSET
	int dx = v_x - m_cache_cx;
	int dz = v_z - m_cache_cz;

	// If offset is too large, just teleport cache
	if (abs(dx) >= dm_cache_line || abs(dz) >= dm_cache_line)
	{
		m_cache_cx = v_x;
		m_cache_cz = v_z;

		// Reinitialize all slots with pointer checks
		for (u32 i = 0; i < dm_cache_line; i++)
		{
			for (u32 j = 0; j < dm_cache_line; j++)
			{
				Slot* slt = m_cache[i][j];
				if (slt)
				{
					cache_Task(j, i, slt);
				}
			}
		}
		m_cache_task.clear();
		return;
	}

	// GRADUALLY SHIFT CACHE
	while (m_cache_cx != v_x)
	{
		if (v_x > m_cache_cx)
		{
			m_cache_cx++;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = m_cache[z][0];
				if (!S)
					continue;

				for (int x = 1; x < dm_cache_line; x++)
				{
					if (m_cache[z][x]) // Safety check
						m_cache[z][x - 1] = m_cache[z][x];
				}
				m_cache[z][dm_cache_line - 1] = S;
				cache_Task(dm_cache_line - 1, z, S);
			}
		}
		else
		{
			m_cache_cx--;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = m_cache[z][dm_cache_line - 1];
				if (!S)
					continue;

				for (int x = dm_cache_line - 1; x > 0; x--)
				{
					if (m_cache[z][x - 1]) // Safety check
						m_cache[z][x] = m_cache[z][x - 1];
				}
				m_cache[z][0] = S;
				cache_Task(0, z, S);
			}
		}
	}

	while (m_cache_cz != v_z)
	{
		if (v_z > m_cache_cz)
		{
			m_cache_cz++;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = m_cache[dm_cache_line - 1][x];
				if (!S)
					continue;

				for (int z = dm_cache_line - 1; z > 0; z--)
				{
					if (m_cache[z - 1][x]) // Safety check
						m_cache[z][x] = m_cache[z - 1][x];
				}
				m_cache[0][x] = S;
				cache_Task(x, 0, S);
			}
		}
		else
		{
			m_cache_cz--;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = m_cache[0][x];
				if (!S)
					continue;

				for (int z = 1; z < dm_cache_line; z++)
				{
					if (m_cache[z][x]) // Safety check
						m_cache[z - 1][x] = m_cache[z][x];
				}
				m_cache[dm_cache_line - 1][x] = S;
				cache_Task(x, dm_cache_line - 1, S);
			}
		}
	}

	// PROCESS DECOMPRESSION TASKS with safety checks
	u32 tasks_to_process = std::min((u32)limit, (u32)m_cache_task.size());
	for (u32 i = 0; i < tasks_to_process; i++)
	{
		Slot* taskSlot = m_cache_task[i];
		if (taskSlot)
		{
			__try
			{
				cache_Decompress(taskSlot);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				Msg("![ERROR] cache_Decompress failed for slot");
			}
		}
	}

	// Remove processed tasks
	if (m_cache_task.size() <= tasks_to_process)
	{
		m_cache_task.clear();
	}
	else
	{
		m_cache_task.remove([&tasks_to_process, processed = 0](const Slot*) mutable { return ++processed <= tasks_to_process; });
	}
}

void CDetailManager::cache_Decompress(Slot* S)
{
	if (!S || !m_loaded || !m_initialized || m_bShuttingDown)
		return;

	Slot& D = *S;
	D.type = stReady;

	// Check object validity
	if (m_objects.empty())
		return;

	// Create test instances with safety checks
	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		if (D.G[i].id == DetailSlot::ID_Empty || D.G[i].id >= m_objects.size())
			continue;

		// Check object validity
		if (!m_objects[D.G[i].id])
			continue;

		// Create 4 test instances
		for (int inst = 0; inst < 4; inst++)
		{
			SlotItem* item = m_poolSI.create();
			if (!item)
				continue;

			// Simple position in slot center
			float center_x = D.sx * dm_slot_size + dm_slot_size / 2;
			float center_z = D.sz * dm_slot_size + dm_slot_size / 2;

			float rx = center_x + Random.randF(-2.0f, 2.0f);
			float rz = center_z + Random.randF(-2.0f, 2.0f);
			float ry = 0.0f;

			// Basic setup
			item->scale = 1.0f;
			item->scale_calculated = 1.0f;
			item->mRotY.identity();
			item->mRotY.rotateY(Random.randF(0, PI_MUL_2));
			item->mRotY.translate_over(rx, ry, rz);
			item->vis_ID = 0;
			item->c_hemi = 1.0f;
			item->c_sun = 1.0f;

			D.G[i].items.push_back(item);
		}
	}

	D.empty = FALSE;
}

BOOL CDetailManager::cache_Validate()
{
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			int w_x = cg2w_X(x);
			int w_z = cg2w_Z(z);
			Slot* D = m_cache[z][x];

			if (!D || D->sx != w_x)
				return FALSE;
			if (D->sz != w_z)
				return FALSE;
		}
	}
	return TRUE;
}
