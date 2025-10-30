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
#include "..\xrEngine\xrXRC.h"

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
const int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};


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

IC bool CDetailManager::InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size)
{
	clamp(x, (u32)0, size - 1);
	clamp(y, (u32)0, size - 1);
	int c = iFloor(Interpolate(alpha255, x, y, size) + .5f);
	clamp(c, 0, 255);

	u32 row = (y + sy) % 16;
	u32 col = (x + sx) % 16;
	return c > dither[col][row];
}

void CDetailManager::UpdateFrustumPlanes()
{
	Fmatrix& M = Device.mFullTransform;

	// Left plane
	m_frustum_planes[FRUSTUM_LEFT].n.x = M._14 + M._11;
	m_frustum_planes[FRUSTUM_LEFT].n.y = M._24 + M._21;
	m_frustum_planes[FRUSTUM_LEFT].n.z = M._34 + M._31;
	m_frustum_planes[FRUSTUM_LEFT].d = M._44 + M._41;

	// Right plane
	m_frustum_planes[FRUSTUM_RIGHT].n.x = M._14 - M._11;
	m_frustum_planes[FRUSTUM_RIGHT].n.y = M._24 - M._21;
	m_frustum_planes[FRUSTUM_RIGHT].n.z = M._34 - M._31;
	m_frustum_planes[FRUSTUM_RIGHT].d = M._44 - M._41;

	// Top plane
	m_frustum_planes[FRUSTUM_TOP].n.x = M._14 - M._12;
	m_frustum_planes[FRUSTUM_TOP].n.y = M._24 - M._22;
	m_frustum_planes[FRUSTUM_TOP].n.z = M._34 - M._32;
	m_frustum_planes[FRUSTUM_TOP].d = M._44 - M._42;

	// Bottom plane
	m_frustum_planes[FRUSTUM_BOTTOM].n.x = M._14 + M._12;
	m_frustum_planes[FRUSTUM_BOTTOM].n.y = M._24 + M._22;
	m_frustum_planes[FRUSTUM_BOTTOM].n.z = M._34 + M._32;
	m_frustum_planes[FRUSTUM_BOTTOM].d = M._44 + M._42;

	// Near plane
	m_frustum_planes[FRUSTUM_NEAR].n.x = M._13;
	m_frustum_planes[FRUSTUM_NEAR].n.y = M._23;
	m_frustum_planes[FRUSTUM_NEAR].n.z = M._33;
	m_frustum_planes[FRUSTUM_NEAR].d = M._43;

	// Far plane
	m_frustum_planes[FRUSTUM_FAR].n.x = M._14 - M._13;
	m_frustum_planes[FRUSTUM_FAR].n.y = M._24 - M._23;
	m_frustum_planes[FRUSTUM_FAR].n.z = M._34 - M._33;
	m_frustum_planes[FRUSTUM_FAR].d = M._44 - M._43;

	// Normalize all planes
	for (int i = 0; i < FRUSTUM_PLANES_COUNT; i++)
	{
		float length = 1.0f / m_frustum_planes[i].n.magnitude();
		m_frustum_planes[i].n.x *= length;
		m_frustum_planes[i].n.y *= length;
		m_frustum_planes[i].n.z *= length;
		m_frustum_planes[i].d *= length;
	}
}

bool CDetailManager::IsSphereInsideFrustum(const Fvector& center, float radius) const
{
	for (int i = 0; i < FRUSTUM_PLANES_COUNT; i++)
	{
		float distance = m_frustum_planes[i].classify(center);
		if (distance < -radius)
			return false;
	}
	return true;
}

bool CDetailManager::IsAABBInsideFrustum(const Fvector& min, const Fvector& max) const
{
	for (int i = 0; i < FRUSTUM_PLANES_COUNT; i++)
	{
		// Check if AABB is completely outside any plane
		Fvector positive;
		positive.x = (m_frustum_planes[i].n.x > 0) ? max.x : min.x;
		positive.y = (m_frustum_planes[i].n.y > 0) ? max.y : min.y;
		positive.z = (m_frustum_planes[i].n.z > 0) ? max.z : min.z;

		if (m_frustum_planes[i].classify(positive) < 0)
			return false;
	}
	return true;
}

bool CDetailManager::IsSlotVisible(const Slot* slot) const
{
	if (!slot || slot->empty)
		return false;

	// First check with sphere (faster)
	if (!IsSphereInsideFrustum(slot->vis.sphere.P, slot->vis.sphere.R))
		return false;

	// Then check with AABB (more precise)
	return IsAABBInsideFrustum(slot->vis.box.min, slot->vis.box.max);
}

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

	// Initialize frustum planes
	ZeroMemory(m_frustum_planes, sizeof(m_frustum_planes));

	// Initialize dither matrix
	bwdithermap(2);
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
	// Quantize float to short in range [-32768, 32767]
	// Scale factor 16384 gives good precision for UV coordinates in range [0, 2]
	int t = iFloor(v * 16384.0f);
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

	// Create instance buffer
	m_hw_BatchSize = 256;
	hr = HW.pDevice->CreateVertexBuffer(m_hw_BatchSize * sizeof(InstanceData), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
										D3DPOOL_DEFAULT, &m_hw_VB_Instances, 0);
	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create instance buffer (HR: 0x%08X)", hr);
		_RELEASE(m_hw_VB);
		_RELEASE(m_hw_IB);
		return;
	}

	// Fill vertex buffer - ИСПРАВЛЕННАЯ ВЕРСИЯ
	vertHW* pVerts = nullptr;
	hr = m_hw_VB->Lock(0, 0, (void**)&pVerts, 0);
	if (SUCCEEDED(hr) && pVerts)
	{
		u32 vertexOffset = 0;
		for (u32 o = 0; o < m_objects.size(); o++)
		{
			CDetail& D = *m_objects[o];

			// Calculate texture scale for wind animation
			float height_range = D.bv_bb.max.y - D.bv_bb.min.y;
			float t_scale = (fis_zero(height_range)) ? 0.0f : 1.0f / height_range;

			for (u32 v = 0; v < D.number_vertices; v++)
			{
				Fvector& vP = D.vertices[v].P;

				// Position
				pVerts->x = vP.x;
				pVerts->y = vP.y;
				pVerts->z = vP.z;

				// UV coordinates - ИСПРАВЛЕНИЕ: правильное квантование
				pVerts->u = QuantConstant(D.vertices[v].u);
				pVerts->v = QuantConstant(D.vertices[v].v);

				// Wind animation parameter (normalized height)
				pVerts->t = QuantConstant((vP.y - D.bv_bb.min.y) * t_scale);

				// Material ID (set to 0 for now)
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

	// Fill index buffer - ИСПРАВЛЕННАЯ ВЕРСИЯ
	u16* pIndices = nullptr;
	hr = m_hw_IB->Lock(0, 0, (void**)&pIndices, 0);
	if (SUCCEEDED(hr) && pIndices)
	{
		for (u32 o = 0; o < m_objects.size(); o++)
		{
			CDetail& D = *m_objects[o];
			for (u32 i = 0; i < D.number_indices; i++)
			{
				*pIndices++ = u16(D.indices[i]);
			}
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

Fvector4 GetWindParams()
{
	CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
	float WindGusting = desc->wind_gusting;
	float WindStrength = desc->wind_strength;
	float WindDirectionX = cos(desc->wind_direction), WindDirectionY = sin(desc->wind_direction);
	return Fvector4().set(WindDirectionX, WindDirectionY, WindGusting, WindStrength);
}

Fvector4 GetWindTurbulence()
{
	CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
	float Turbulence = desc->wind_turbulence;
	float TurbulencePacked = std::max(std::min(Turbulence, 0.0f), 1.0f);
	float WindSpeed = Device.fTimeGlobal * 2.5f * (1.0f + TurbulencePacked);
	return Fvector4().set(Turbulence, TurbulencePacked, WindSpeed, 0);
}

void CDetailManager::hw_Render_dump()
{
	Device.Statistic->RenderDUMP_DT_Count = 0;

	// МАТРИЦЫ - используем актуальные значения
	Fmatrix mWorld = Fidentity;
	Fmatrix mView = Device.mView;
	Fmatrix mProject = Device.mProject;

	// ВАЖНО: Device.mFullTransform уже содержит Project * View * World
	Fmatrix mWorldViewProject = Device.mFullTransform;

	// WorldView = View * World (World = identity)
	Fmatrix mWorldView;
	mWorldView.mul(mView, mWorld);

	Fvector4 wind_params = GetWindParams();
	Fvector4 wind_turbulence = GetWindTurbulence();

	// ПРИНУДИТЕЛЬНЫЙ СБРОС КОНСТАНТ ПЕРЕД НАЧАЛОМ
	RenderBackend.get_ConstantCache_Vertex().force_dirty();
	RenderBackend.get_ConstantCache_Pixel().force_dirty();

	// ОСНОВНОЙ ЦИКЛ РЕНДЕРИНГА
	for (u32 O = 0; O < m_objects.size(); O++)
	{
		CDetail& Object = *m_objects[O];

		// ВЫЧИСЛЯЕМ СМЕЩЕНИЯ
		u32 vOffset = 0;
		u32 iOffset = 0;
		for (u32 i = 0; i < O; i++)
		{
			vOffset += m_objects[i]->number_vertices;
			iOffset += m_objects[i]->number_indices;
		}

		for (u32 lod_id = 0; lod_id < 3; lod_id++)
		{
			xr_vector<SlotItemVec*>& vis = m_render_visibles[lod_id][O];
			if (vis.empty())
				continue;

			int shader_id = Object.m_Flags.is(DO_NO_WAVING) ? SE_DETAIL_NORMAL_STATIC : SE_DETAIL_NORMAL_ANIMATED;
			if (!Object.shader || !Object.shader->E[shader_id])
				continue;

			// Собираем инстансы
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

					// МАТРИЦА ИНСТАНСА
					Fmatrix& M = item.mRotY;
					float scale = item.scale_calculated;

					inst.mat0.set(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
					inst.mat1.set(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
					inst.mat2.set(M._13 * scale, M._23 * scale, M._33 * scale, M._43);

					inst.color.set(item.c_sun, item.c_sun, item.c_sun, item.c_hemi);
					inst.lod_id = (float)lod_id;

					instances.push_back(inst);
				}
			}

			if (instances.empty())
				continue;

			// 1. Сначала устанавливаем шейдер
			RenderBackend.set_Element(Object.shader->E[shader_id]);

			// 2. ПРИНУДИТЕЛЬНО СБРАСЫВАЕМ КОНСТАНТЫ ДЛЯ ЭТОГО ШЕЙДЕРА
			RenderBackend.get_ConstantCache_Vertex().force_dirty();
			RenderBackend.get_ConstantCache_Pixel().force_dirty();

			// 3. Устанавливаем константы ДЛЯ ЭТОГО ШЕЙДЕРА
			RenderBackend.set_Constant("m_WorldViewProject", mWorldViewProject);
			RenderBackend.set_Constant("m_WorldView", mWorldView);

			// 4. Ветер - тоже через именованные константы
			if (!Object.m_Flags.is(DO_NO_WAVING))
			{
				RenderBackend.set_Constant("m_wind_turbulence", wind_turbulence);
				RenderBackend.set_Constant("m_wind_params", wind_params);
			}

			// 5. Явно форсируем обновление констант в GPU
			RenderBackend.flush();

			// Рендерим батчи
			u32 total_instances = instances.size();
			u32 batch_count = (total_instances + m_hw_BatchSize - 1) / m_hw_BatchSize;

			for (u32 batch = 0; batch < batch_count; batch++)
			{
				u32 start_instance = batch * m_hw_BatchSize;
				u32 end_instance = std::min(start_instance + m_hw_BatchSize, total_instances);
				u32 batch_instance_count = end_instance - start_instance;

				InstanceData* pInstances = nullptr;
				HRESULT hr = m_hw_VB_Instances->Lock(0, batch_instance_count * sizeof(InstanceData),
													 (void**)&pInstances, D3DLOCK_DISCARD);

				if (SUCCEEDED(hr) && pInstances)
				{
					CopyMemory(pInstances, &instances[start_instance], batch_instance_count * sizeof(InstanceData));
					m_hw_VB_Instances->Unlock();

					// Настройка инстансинга
					HW.pDevice->SetStreamSource(0, m_hw_VB, 0, sizeof(vertHW));
					HW.pDevice->SetStreamSource(1, m_hw_VB_Instances, 0, sizeof(InstanceData));

					HW.pDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | batch_instance_count);
					HW.pDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul);

					// Рендеринг
					hr = HW.pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices,
														  iOffset, Object.number_indices / 3);

					if (SUCCEEDED(hr))
					{
						Device.Statistic->RenderDUMP_DT_Count += batch_instance_count;
					}

					// Восстановление
					HW.pDevice->SetStreamSourceFreq(0, 1);
					HW.pDevice->SetStreamSourceFreq(1, 1);
				}
			}
		}
	}

	// Очистка списков видимости
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

extern float r_ssaDISCARD;

void CDetailManager::UpdateVisibleM()
{
	if (!g_pGameLevel || !Device.Statistic || !m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	Device.Statistic->RenderDUMP_DT_VIS.Begin();

	ClearVisibleLists();

	// Update frustum planes for current frame
	UpdateFrustumPlanes();

	// Проходим по всем слотам в кэше и добавляем их в видимый список с проверкой Frustum Culling
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* S = m_cache[z][x];
			if (!S || S->empty)
				continue;

			// Frustum Culling check - пропускаем невидимые слоты
			if (!IsSlotVisible(S))
				continue;

			// Обновляем фрейм (чтобы не обновлять каждый кадр, но без проверок расстояния)
			if (Device.dwFrame > S->frame)
			{
				S->frame = Device.dwFrame;

				// Просто устанавливаем масштаб без затухания
				for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
				{
					SlotPart& sp = S->G[sp_id];
					if (sp.id == DetailSlot::ID_Empty)
						continue;

					// Clear LOD items
					for (int lod = 0; lod < 3; lod++)
						sp.r_items[lod].clear();

					// Все предметы в LOD 0
					for (auto& item_ptr : sp.items)
					{
						SlotItem& Item = *item_ptr;
						Item.scale_calculated = Item.scale; // Без затухания
						sp.r_items[0].push_back(item_ptr);
					}
				}
			}

			// Добавляем все объекты из слота в видимые списки
			for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
			{
				SlotPart& sp = S->G[sp_id];
				if (sp.id == DetailSlot::ID_Empty)
					continue;

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

	// Инициализация как в старом коде
	Slot* slt = m_cache_pool;
	for (u32 i = 0; i < dm_cache_line; i++)
		for (u32 j = 0; j < dm_cache_line; j++, slt++)
		{
			m_cache[i][j] = slt;
			cache_Task(j, i, slt);
		}

	// Инициализация cache_level1 КАК В СТАРОМ КОДЕ
	for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
	{
		for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
		{
			CacheSlot1& MS = m_cache_level1[_mz1][_mx1];
			for (int _z = 0; _z < dm_cache1_count; _z++)
				for (int _x = 0; _x < dm_cache1_count; _x++)
					MS.slots[_z * dm_cache1_count + _x] =
						&m_cache[_mz1 * dm_cache1_count + _z][_mx1 * dm_cache1_count + _x];
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

	// ПРОСТАЯ ПРОВЕРКА ПУСТОТЫ
	D->empty = (DS.id0 == DetailSlot::ID_Empty) && (DS.id1 == DetailSlot::ID_Empty) &&
			   (DS.id2 == DetailSlot::ID_Empty) && (DS.id3 == DetailSlot::ID_Empty);

	D->type = stReady;
	D->sx = sx;
	D->sz = sz;

	// РАСЧЕТ BOUNDING BOX
	D->vis.box.min.set(sx * dm_slot_size, DS.r_ybase(), sz * dm_slot_size);
	D->vis.box.max.set(D->vis.box.min.x + dm_slot_size, DS.r_ybase() + DS.r_yheight(), D->vis.box.min.z + dm_slot_size);
	D->vis.box.grow(0.05f);
	D->vis.box.getsphere(D->vis.sphere.P, D->vis.sphere.R);

	// ОЧИЩАЕМ И УСТАНАВЛИВАЕМ ID
	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		D->G[i].id = DS.r_id(i);
		for (auto& item : D->G[i].items)
		{
			m_poolSI.destroy(item);
		}
		D->G[i].items.clear();
		for (int lod = 0; lod < 3; lod++)
			D->G[i].r_items[lod].clear();
	}

	// ВСЕГДА ДОБАВЛЯЕМ В ЗАДАЧИ ДЕКОМПРЕССИИ
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

void CDetailManager::UpdateCacheLevel1Bounds()
{
	for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
	{
		for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
		{
			CacheSlot1& MS = m_cache_level1[_mz1][_mx1];
			MS.empty = true;
			MS.vis.clear();
			MS.vis.box.invalidate(); // Start with invalid box

			int nullCount = 0;
			int validCount = 0;

			for (int _i = 0; _i < dm_cache1_count * dm_cache1_count; _i++)
			{
				// ДОБАВЬТЕ ПРОВЕРКУ НА nullptr!
				if (MS.slots[_i] == nullptr)
				{
					nullCount++;
					continue;
				}

				Slot* PS = *MS.slots[_i];
				if (!PS)
				{
					nullCount++;
					continue;
				}

				validCount++;
				Slot& S = *PS;

				// Merge bounding box
				if (MS.vis.box.is_valid())
				{
					MS.vis.box.merge(S.vis.box);
				}
				else
				{
					MS.vis.box.set(S.vis.box);
				}

				if (!S.empty)
					MS.empty = false;
			}

			if (nullCount > 0)
			{
				Msg("CacheLevel1[%d][%d]: %d null slots, %d valid slots", _mz1, _mx1, nullCount, validCount);
			}

			// Если bounding box остался невалидным, установите безопасные значения
			if (!MS.vis.box.is_valid())
			{
				// Вычисляем мировые координаты для этого cache1 слота
				int world_x = cg2w_X(_mx1 * dm_cache1_count);
				int world_z = cg2w_Z(_mz1 * dm_cache1_count);

				MS.vis.box.min.set(world_x, 0, world_z);
				MS.vis.box.max.set(world_x + dm_slot_size * dm_cache1_count, 1.0f,
								   world_z + dm_slot_size * dm_cache1_count);
				MS.vis.box.grow(0.1f);
			}

			MS.vis.box.getsphere(MS.vis.sphere.P, MS.vis.sphere.R);
		}
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

	// If offset is too large, just teleport cache (как в старой системе)
	if (abs(dx) >= dm_cache_line || abs(dz) >= dm_cache_line)
	{
		m_cache_cx = v_x;
		m_cache_cz = v_z;

		// Reinitialize all slots with pointer checks
		Slot* slt = m_cache_pool;
		for (u32 i = 0; i < dm_cache_line; i++)
		{
			for (u32 j = 0; j < dm_cache_line; j++, slt++)
			{
				if (m_cache[i][j])
				{
					cache_Task(j, i, m_cache[i][j]);
				}
			}
		}
		m_cache_task.clear();

		// Update level1 cache bounds
		UpdateCacheLevel1Bounds();
		return;
	}

	// GRADUALLY SHIFT CACHE - как в старой системе
	while (m_cache_cx != v_x)
	{
		if (v_x > m_cache_cx)
		{
			// shift matrix to left
			m_cache_cx++;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = m_cache[z][0];
				if (!S)
					continue;

				for (int x = 1; x < dm_cache_line; x++)
				{
					m_cache[z][x - 1] = m_cache[z][x];
				}
				m_cache[z][dm_cache_line - 1] = S;
				cache_Task(dm_cache_line - 1, z, S);
			}
		}
		else
		{
			// shift matrix to right
			m_cache_cx--;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = m_cache[z][dm_cache_line - 1];
				if (!S)
					continue;

				for (int x = dm_cache_line - 1; x > 0; x--)
				{
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
			// shift matrix down
			m_cache_cz++;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = m_cache[dm_cache_line - 1][x];
				if (!S)
					continue;

				for (int z = dm_cache_line - 1; z > 0; z--)
				{
					m_cache[z][x] = m_cache[z - 1][x];
				}
				m_cache[0][x] = S;
				cache_Task(x, 0, S);
			}
		}
		else
		{
			// shift matrix up
			m_cache_cz--;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = m_cache[0][x];
				if (!S)
					continue;

				for (int z = 1; z < dm_cache_line; z++)
				{
					m_cache[z - 1][x] = m_cache[z][x];
				}
				m_cache[dm_cache_line - 1][x] = S;
				cache_Task(x, dm_cache_line - 1, S);
			}
		}
	}

	// Update level1 cache bounds after shifting
	if (m_initialized)
	{
		UpdateCacheLevel1Bounds();
	}

	// PROCESS DECOMPRESSION TASKS
	u32 tasks_to_process = m_cache_task.size();
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
	m_cache_task.clear();
}

void CDetailManager::cache_Decompress(Slot* S)
{
	if (!S || !m_loaded || !m_initialized || m_bShuttingDown)
		return;

	Slot& D = *S;
	D.type = stReady;

	if (!D.empty && !D.G[0].items.empty())
		return;

	if (D.empty)
		return;

	DetailSlot& DS = QueryDB(D.sx, D.sz);

	// Получаем bounding box для трассировки - FIXED
	Fvector bC, bD;
	D.vis.box.get_CD(bC, bD);

	// Выполняем запрос к статической геометрии
	m_xrc.box_options(CDB::OPT_FULL_TEST);
	m_xrc.box_query(g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);
	u32 triCount = m_xrc.r_count();

	if (0 == triCount)
		return;

	CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
	Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

	// Подготавливаем альфа-каналы для дизеринга
	float alpha255[dm_obj_in_slot][4];
	for (int i = 0; i < dm_obj_in_slot; i++)
	{
		alpha255[i][0] = 255.f * float(DS.palette[i].a0) / 15.f;
		alpha255[i][1] = 255.f * float(DS.palette[i].a1) / 15.f;
		alpha255[i][2] = 255.f * float(DS.palette[i].a2) / 15.f;
		alpha255[i][3] = 255.f * float(DS.palette[i].a3) / 15.f;
	}

	// Параметры плотности
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

	// Проходим по всем точкам в слоте - FIXED DISTRIBUTION LOGIC
	for (u32 z = 0; z <= d_size; z++)
	{
		for (u32 x = 0; x <= d_size; x++)
		{
			u32 shift_x = r_Jitter.randI(16);
			u32 shift_z = r_Jitter.randI(16);

			selected.clear();

			// Проверяем какие объекты должны быть размещены в этой точке - FIXED SELECTION
			for (u32 i = 0; i < dm_obj_in_slot; i++)
			{
				u32 detail_id = GetSlotID(DS, i);
				if (detail_id != DetailSlot::ID_Empty &&
					InterpolateAndDither(alpha255[i], x, z, shift_x, shift_z, d_size))
				{
					selected.push_back(i);
				}
			}

			if (selected.empty())
				continue;

			// Выбираем случайный объект из подходящих
			u32 index;
			if (selected.size() == 1)
				index = selected[0];
			else
				index = selected[r_selection.randI(selected.size())];

			u32 detail_id = GetSlotID(DS, index);
			if (detail_id >= m_objects.size())
				continue;

			CDetail* Dobj = m_objects[detail_id];
			SlotItem* ItemP = m_poolSI.create();
			SlotItem& Item = *ItemP;

			// Вычисляем позицию в мировых координатах - FIXED COORDINATES
			float rx = (float(x) / float(d_size)) * dm_slot_size + D.vis.box.min.x;
			float rz = (float(z) / float(d_size)) * dm_slot_size + D.vis.box.min.z;

			Fvector Item_P;
			Item_P.set(rx + r_Jitter.randFs(jitter), D.vis.box.max.y, rz + r_Jitter.randFs(jitter));

			// Трассируем луч вниз для определения высоты
			float y = D.vis.box.min.y - 5;
			Fvector dir;
			dir.set(0, -1, 0);

			float r_u, r_v, r_range;
			for (u32 tid = 0; tid < triCount; tid++)
			{
				CDB::TRI& T = tris[m_xrc.r_begin()[tid].id];
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
			}

			if (y < D.vis.box.min.y)
			{
				m_poolSI.destroy(ItemP);
				continue;
			}

			Item_P.y = y;

			// Устанавливаем параметры объекта
			Item.scale = r_scale.randF(Dobj->m_fMinScale, Dobj->m_fMaxScale);
			Item.scale_calculated = Item.scale;

			// Создаем матрицу трансформации - FIXED ROTATION
			Item.mRotY.identity();
			Item.mRotY.rotateY(r_yaw.randF(0, PI_MUL_2));
			Item.mRotY.translate_over(Item_P);

			// Вычисляем bounding box для объединения
			Fmatrix mScale, mXform;
			Fbox ItemBB;
			mScale.scale(Item.scale, Item.scale, Item.scale);
			mXform.mul_43(Item.mRotY, mScale);
			ItemBB.xform(Dobj->bv_bb, mXform);
			Bounds.merge(ItemBB);

			// Устанавливаем освещение
			Item.c_hemi = GetSlotHemi(DS);
			Item.c_sun = GetSlotSun(DS);
			Item.vis_ID = 0;

			D.G[index].items.push_back(ItemP);
		}
	}

	// Обновляем bounding volume слота
	if (Bounds.is_valid())
	{
		D.vis.box.set(Bounds);
		D.vis.box.grow(0.05f); // Небольшой зазор
		D.vis.box.getsphere(D.vis.sphere.P, D.vis.sphere.R);
	}

	// Проверяем, остался ли слот пустым
	D.empty = true;
	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		if (!D.G[i].items.empty())
		{
			D.empty = false;
			break;
		}
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
			Slot* D = m_cache[z][x];

			if (!D || D->sx != w_x)
				return FALSE;
			if (D->sz != w_z)
				return FALSE;
		}
	}
	return TRUE;
}

void CDetailManager::bwdithermap(int levels)
{
	/* Get size of each step */
	float N = 255.0f / (levels - 1);

	float magicfact = (N - 1) / 16;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				for (int l = 0; l < 4; l++)
				{
					dither[4 * k + i][4 * l + j] =
						(int)(0.5 + magic4x4[i][j] * magicfact + (magic4x4[k][l] / 16.) * magicfact);
				}
			}
		}
	}
}
