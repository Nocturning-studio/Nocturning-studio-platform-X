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
BOOL CDetailManager::s_disable_hom = TRUE;
BOOL CDetailManager::s_use_hom_occlusion = FALSE;
float CDetailManager::s_occlusion_padding_scale = 1.0f;
float CDetailManager::s_max_occlusion_distance = 150.0f;
BOOL CDetailManager::s_use_occlusion_culling = TRUE;
u32 CDetailManager::s_occlusion_frame_interval = 4;
u32 CDetailManager::s_occlusion_min_visible_frames = 2;
EOcclusionMethod CDetailManager::s_preferred_occlusion_method = OCCLUSION_HYBRID;
float CDetailManager::s_occlusion_confidence_threshold = 0.7f;

const int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};

// Vertex declarations
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

static D3DVERTEXELEMENT9 dwDecl_Occlusion[] = {
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};

D3DVERTEXELEMENT9* CDetailManager::GetDetailsVertexDecl()
{
	return dwDecl_Details;
}

D3DVERTEXELEMENT9* CDetailManager::GetDetailsInstancedVertexDecl()
{
	return dwDecl_Details_Instanced;
}

D3DVERTEXELEMENT9* CDetailManager::GetOcclusionVertexDecl()
{
	return dwDecl_Occlusion;
}

////////////////////////////////////////////////////////////////////////////
// Core Implementation
////////////////////////////////////////////////////////////////////////////

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

CDetailManager::CDetailManager()
	: m_dtFS(nullptr), m_dtSlots(nullptr), m_hw_Geom(nullptr), m_hw_Geom_Instanced(nullptr), m_hw_BatchSize(0),
	  m_hw_VB(nullptr), m_hw_VB_Instances(nullptr), m_hw_IB(nullptr), m_cache_cx(0), m_cache_cz(0), m_frame_calc(0),
	  m_frame_rendered(0), m_initialized(false), m_loaded(false), m_bInLoad(FALSE), m_bShuttingDown(FALSE),
	  m_bRegisteredInParallel(false), m_occlusion_frame(0), m_occlusion_enabled(false),
	  m_primary_occlusion_method(OCCLUSION_NONE), m_fallback_occlusion_method(OCCLUSION_NONE),
	  m_software_depth_buffer(nullptr), m_software_depth_width(0), m_software_depth_height(0),
	  m_software_depth_scale(1.0f), m_occlusion_geom(nullptr), m_occlusion_vb(nullptr), m_occlusion_ib(nullptr)
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
		Device.seqParallel.remove([this](const fastdelegate::FastDelegate0<>& delegate) { return true; });

		m_bRegisteredInParallel = false;
		Msg("CDetailManager: Unregistered from parallel tasks");
	}
}

////////////////////////////////////////////////////////////////////////////
// Frustum Culling Implementation
////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////
// Multi-Method Occlusion Culling Implementation
////////////////////////////////////////////////////////////////////////////

void CDetailManager::DetectAvailableOcclusionMethods()
{
	m_primary_occlusion_method = OCCLUSION_NONE;
	m_fallback_occlusion_method = OCCLUSION_NONE;

	// Проверяем доступность HOM
	bool hom_available = (s_use_hom_occlusion && !s_disable_hom);

	// Проверяем поддержку hardware queries
	bool hardware_queries_available = false;
	IDirect3DQuery9* test_query = nullptr;
	if (SUCCEEDED(HW.pDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &test_query)))
	{
		hardware_queries_available = true;
		test_query->Release();
	}

	// Проверяем наличие geometry occlusion данных
	bool geometry_available = !m_occlusion_geometries.empty() || !m_simple_occluders.empty();

	// Проверяем наличие portal данных
	bool portals_available = !m_occlusion_portals.empty();

	// Выбираем primary метод на основе предпочтений и доступности
	switch (s_preferred_occlusion_method)
	{
	case OCCLUSION_HOM:
		if (hom_available)
			m_primary_occlusion_method = OCCLUSION_HOM;
		break;
	case OCCLUSION_HARDWARE_QUERY:
		if (hardware_queries_available)
			m_primary_occlusion_method = OCCLUSION_HARDWARE_QUERY;
		break;
	case OCCLUSION_GEOMETRY:
		if (geometry_available)
			m_primary_occlusion_method = OCCLUSION_GEOMETRY;
		break;
	case OCCLUSION_PORTALS:
		if (portals_available)
			m_primary_occlusion_method = OCCLUSION_PORTALS;
		break;
	case OCCLUSION_RASTER:
		m_primary_occlusion_method = OCCLUSION_RASTER;
		break;
	case OCCLUSION_HYBRID:
		if (hardware_queries_available)
			m_primary_occlusion_method = OCCLUSION_HARDWARE_QUERY;
		else if (geometry_available)
			m_primary_occlusion_method = OCCLUSION_GEOMETRY;
		else if (portals_available)
			m_primary_occlusion_method = OCCLUSION_PORTALS;
		else
			m_primary_occlusion_method = OCCLUSION_RASTER;
		break;
	default:
		m_primary_occlusion_method = OCCLUSION_NONE;
	}

	// Выбираем fallback метод
	m_fallback_occlusion_method = OCCLUSION_RASTER;

	Msg("CDetailManager: Primary occlusion method: %d, Fallback: %d", m_primary_occlusion_method,
		m_fallback_occlusion_method);
}

EOcclusionMethod CDetailManager::SelectOcclusionMethod() const
{
	return m_primary_occlusion_method;
}

void CDetailManager::LoadOcclusionGeometry()
{
	Msg("CDetailManager::LoadOcclusionGeometry() - Loading occlusion geometry");
	BuildSimpleOccluders();
}

void CDetailManager::BuildSimpleOccluders()
{
	m_simple_occluders.clear();

	// Создаем более разумные occluders - только вокруг значимых объектов
	// и ограничиваем их размер

	Fvector occluder1_min = {-20, 0, -20}; // Уменьшили размер
	Fvector occluder1_max = {20, 15, 20};  // Уменьшили высоту
	m_simple_occluders.push_back(occluder1_min);
	m_simple_occluders.push_back(occluder1_max);

	Fvector occluder2_min = {80, 0, -15};  // Уменьшили размер
	Fvector occluder2_max = {120, 12, 15}; // Уменьшили высоту
	m_simple_occluders.push_back(occluder2_min);
	m_simple_occluders.push_back(occluder2_max);

	Msg("CDetailManager::BuildSimpleOccluders() - Created %d simple occluders", m_simple_occluders.size() / 2);
}

bool CDetailManager::TestGeometryOcclusion(const Fvector& test_point, float test_radius) const
{
	if (m_occlusion_geometries.empty() && m_simple_occluders.empty())
		return false;

	Fvector camera_pos = Device.vCameraPosition;
	float distance_to_test = camera_pos.distance_to(test_point);

	// Быстрая проверка: если объект слишком близко, не occlusion'им
	if (distance_to_test < 5.0f)
		return false;

	// Если объект слишком далеко, используем упрощенную проверку
	if (distance_to_test > s_max_occlusion_distance)
		return false;

	// Проверяем простые occluders
	for (size_t i = 0; i < m_simple_occluders.size(); i += 2)
	{
		const Fvector& min = m_simple_occluders[i];
		const Fvector& max = m_simple_occluders[i + 1];

		// Быстрая проверка расстояния до occluder'а
		Fvector occluder_center;
		occluder_center.add(min, max).mul(0.5f);
		float distance_to_occluder = camera_pos.distance_to(occluder_center);

		// Если occluder дальше тестовой точки, он не может occlusion'ить
		if (distance_to_occluder > distance_to_test)
			continue;

		// Проверяем, находится ли тестовая точка за bounding box'ом occluder'а
		if (test_point.x < min.x - test_radius && camera_pos.x > max.x + test_radius)
			continue;
		if (test_point.x > max.x + test_radius && camera_pos.x < min.x - test_radius)
			continue;
		if (test_point.z < min.z - test_radius && camera_pos.z > max.z + test_radius)
			continue;
		if (test_point.z > max.z + test_radius && camera_pos.z < min.z - test_radius)
			continue;

		// Более точная проверка с raycast
		Fvector dir_to_test = Fvector().sub(test_point, camera_pos);
		dir_to_test.normalize();

		Fvector intersection;
		Fbox occluder_box;
		occluder_box.set(min, max);

		if (Fbox::rpOriginOutside == occluder_box.Pick2(camera_pos, dir_to_test, intersection))
		{
			float intersection_dist = camera_pos.distance_to(intersection);
			if (intersection_dist < distance_to_test - test_radius)
			{
				return true;
			}
		}
	}

	return false;
}

bool CDetailManager::TestSimpleOccluders(const Fvector& test_point, float test_radius) const
{
	Fvector camera_pos = Device.vCameraPosition;

	for (size_t i = 0; i < m_simple_occluders.size(); i += 2)
	{
		const Fvector& min = m_simple_occluders[i];
		const Fvector& max = m_simple_occluders[i + 1];

		Fvector dir = Fvector().sub(test_point, camera_pos);
		float distance = dir.magnitude();
		dir.normalize();

		Fvector outR; // точка пересечения
		Fbox test_box;
		test_box.set(min, max);
		if (Fbox::rpOriginOutside == test_box.Pick2(camera_pos, dir, outR))
		{
			// Вычисляем расстояние от камеры до точки пересечения
			float dist = camera_pos.distance_to(outR);
			if (dist < distance - test_radius)
			{
				return true;
			}
		}
	}

	return false;
}

void CDetailManager::LoadOcclusionPortals()
{
	Msg("CDetailManager::LoadOcclusionPortals() - Loading portal system");

	OcclusionPortal portal1;
	portal1.center.set(0, 10, 0);
	portal1.radius = 5.0f;
	portal1.active = true;

	portal1.polygon.push_back(Fvector().set(-3, 8, 0));
	portal1.polygon.push_back(Fvector().set(3, 8, 0));
	portal1.polygon.push_back(Fvector().set(3, 12, 0));
	portal1.polygon.push_back(Fvector().set(-3, 12, 0));

	m_occlusion_portals.push_back(portal1);

	Msg("CDetailManager::LoadOcclusionPortals() - Loaded %d portals", m_occlusion_portals.size());
}

bool CDetailManager::TestPortalOcclusion(const Fvector& test_point, float test_radius) const
{
	if (m_occlusion_portals.empty())
		return false;

	Fvector camera_pos = Device.vCameraPosition;

	for (const auto& portal : m_occlusion_portals)
	{
		if (!portal.active)
			continue;

		// Быстрая проверка расстояния
		float portal_dist = camera_pos.distance_to(portal.center);
		float test_dist = camera_pos.distance_to(test_point);

		// Если портал дальше тестовой точки, он не может occlusion'ить
		if (portal_dist > test_dist)
			continue;

		bool camera_behind = IsPointBehindPortal(camera_pos, portal);
		bool test_point_behind = IsPointBehindPortal(test_point, portal);

		if (camera_behind != test_point_behind)
		{
			// Улучшенная проверка: учитываем размер портала и тестовой области
			float angular_size = portal.radius / portal_dist;
			float test_angular_size = test_radius / test_dist;

			// Если портал значительно меньше тестового объекта, он не может его полностью occlusion'ить
			if (angular_size < test_angular_size * 0.5f)
				continue;

			return true;
		}
	}

	return false;
}

bool CDetailManager::IsPointBehindPortal(const Fvector& point, const OcclusionPortal& portal) const
{
	if (portal.polygon.size() < 3)
		return false;

	Fvector v1 = Fvector().sub(portal.polygon[1], portal.polygon[0]);
	Fvector v2 = Fvector().sub(portal.polygon[2], portal.polygon[0]);
	Fvector normal;
	normal.crossproduct(v1, v2).normalize();

	Fvector to_point = Fvector().sub(point, portal.polygon[0]);

	return to_point.dotproduct(normal) < 0;
}

void CDetailManager::InitializeSoftwareRasterizer()
{
	Msg("CDetailManager::InitializeSoftwareRasterizer() - Initializing software rasterizer");

	m_software_depth_width = 64;
	m_software_depth_height = 64;
	m_software_depth_scale = 1.0f;

	size_t buffer_size = m_software_depth_width * m_software_depth_height;
	m_software_depth_buffer = xr_alloc<u32>(buffer_size);

	u32 max_depth = 0xFFFFFFFF;
	for (size_t i = 0; i < buffer_size; i++)
	{
		m_software_depth_buffer[i] = max_depth;
	}

	Msg("CDetailManager::InitializeSoftwareRasterizer() - Software rasterizer initialized: %dx%d",
		m_software_depth_width, m_software_depth_height);
}

void CDetailManager::CleanupSoftwareRasterizer()
{
	if (m_software_depth_buffer)
	{
		xr_free(m_software_depth_buffer);
		m_software_depth_buffer = nullptr;
	}
}

void CDetailManager::UpdateSoftwareDepthBuffer()
{
	size_t buffer_size = m_software_depth_width * m_software_depth_height;
	u32 max_depth = 0xFFFFFFFF;
	for (size_t i = 0; i < buffer_size; i++)
	{
		m_software_depth_buffer[i] = max_depth;
	}

	RasterizeOcclusionGeometry();
}

void CDetailManager::RasterizeOcclusionGeometry()
{
	// Упрощенная реализация - в реальном проекте нужен полноценный rasterizer
	// Здесь просто отмечаем, что geometry присутствует
}

bool CDetailManager::TestSoftwareOcclusion(const Fvector& test_point, float test_radius) const
{
	// Простая реализация без сложного rasterizer'а
	// Используем приближенную проверку глубины на основе расстояния

	Fvector camera_pos = Device.vCameraPosition;
	float test_distance = camera_pos.distance_to(test_point);

	// Для software метода используем упрощенную эвристику:
	// Если объект находится за крупными статическими объектами, считаем его occlusion'нутым

	// Проверяем, нет ли крупных объектов между камерой и тестовой точкой
	collide::rq_result RQ;
	Fvector dir_to_test = Fvector().sub(test_point, camera_pos);
	float dir_length = dir_to_test.magnitude();
	dir_to_test.normalize();

	if (g_pGameLevel->ObjectSpace.RayPick(camera_pos, dir_to_test, dir_length, collide::rqtStatic, RQ, nullptr))
	{
		// Если столкновение произошло значительно ближе тестовой точки
		if (RQ.range < test_distance - test_radius - 1.0f)
		{
			return true;
		}
	}

	return false;
}

bool CDetailManager::PerformOcclusionTest(Slot* slot)
{
	if (!slot || slot->empty)
		return true;

	Fvector test_point = slot->vis.sphere.P;
	float test_radius = slot->vis.sphere.R * s_occlusion_padding_scale;

	Fvector camera_pos = Device.vCameraPosition;
	float distance = camera_pos.distance_to(test_point);

	// Быстрые проверки
	if (distance < 3.0f)
		return false; // Слишком близко - не occlusion'им
	if (distance > s_max_occlusion_distance)
		return false; // Слишком далеко

	bool occluded = false;
	float confidence = 0.0f;

	// Приоритет методов в зависимости от расстояния
	if (distance < 30.0f)
	{
		// Близкие объекты - точная геометрическая проверка
		occluded = TestGeometryOcclusion(test_point, test_radius);
		confidence = 0.8f;
	}
	else if (distance < 80.0f)
	{
		// Средняя дистанция - комбинированная проверка
		occluded = TestGeometryOcclusion(test_point, test_radius) || TestSoftwareOcclusion(test_point, test_radius);
		confidence = 0.7f;
	}
	else
	{
		// Дальние объекты - быстрая проверка
		occluded = TestSoftwareOcclusion(test_point, test_radius);
		confidence = 0.6f;
	}

	slot->occlusion_confidence = confidence;
	return occluded;
}

bool CDetailManager::PerformOcclusionTest(CacheSlot1* cache1)
{
	if (!cache1 || cache1->empty)
		return true;

	Fvector test_point = cache1->vis.sphere.P;
	float test_radius = cache1->vis.sphere.R * s_occlusion_padding_scale;

	bool occluded = false;

	switch (m_primary_occlusion_method)
	{
	case OCCLUSION_GEOMETRY:
		occluded = TestGeometryOcclusion(test_point, test_radius);
		break;

	case OCCLUSION_PORTALS:
		occluded = TestPortalOcclusion(test_point, test_radius);
		break;

	case OCCLUSION_RASTER:
		occluded = TestSoftwareOcclusion(test_point, test_radius);
		break;

	case OCCLUSION_HYBRID:
		occluded = TestGeometryOcclusion(test_point, test_radius) || TestPortalOcclusion(test_point, test_radius) ||
				   TestSoftwareOcclusion(test_point, test_radius);
		break;

	default:
		occluded = false;
	}

	cache1->occlusion_confidence = 0.8f;
	return occluded;
}

float CDetailManager::CalculateOcclusionConfidence(const Fvector& test_point, EOcclusionMethod method) const
{
	Fvector camera_pos = Device.vCameraPosition;
	float distance = camera_pos.distance_to(test_point);

	float base_confidence = 0.0f;

	switch (method)
	{
	case OCCLUSION_HOM:
		base_confidence = 0.95f;
		break;
	case OCCLUSION_HARDWARE_QUERY:
		base_confidence = 0.90f;
		break;
	case OCCLUSION_GEOMETRY:
		base_confidence = 0.85f;
		break;
	case OCCLUSION_PORTALS:
		base_confidence = 0.80f;
		break;
	case OCCLUSION_RASTER:
		base_confidence = 0.70f;
		break;
	default:
		base_confidence = 0.50f;
	}

	float distance_factor = 1.0f - (distance / s_max_occlusion_distance);
	clamp(distance_factor, 0.1f, 1.0f);

	return base_confidence * distance_factor;
}

////////////////////////////////////////////////////////////////////////////
// Hardware Query Methods
////////////////////////////////////////////////////////////////////////////

void CDetailManager::UpdateHardwareOcclusionTests()
{
	//if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return;

	m_occlusion_frame++;

	Fvector camera_pos = Device.vCameraPosition;

	m_occlusion_frame++;

	m_occlusion_queries.clear();
	m_occlusion_test_slots.clear();
	m_occlusion_test_cache1.clear();

	// Test cache level1 first
	for (int z = 0; z < dm_cache1_line; z++)
	{
		for (int x = 0; x < dm_cache1_line; x++)
		{
			CacheSlot1& cache1 = m_cache_level1[z][x];

			if (cache1.empty)
				continue;

			if ((m_occlusion_frame - cache1.last_occlusion_test_frame) < s_occlusion_frame_interval)
				continue;

			cache1.last_occlusion_test_frame = m_occlusion_frame;

			if (IsAABBInsideFrustum(cache1.vis.box.min, cache1.vis.box.max))
			{
				m_occlusion_test_cache1.push_back(&cache1);
				m_occlusion_queries.push_back(cache1.occlusion_query);
				RenderHardwareOcclusionTest(cache1.vis.box.min, cache1.vis.box.max);
			}
			else
			{
				cache1.occlusion_visible = false;
				cache1.occlusion_visible_frames = 0;
			}
		}
	}

	// Test individual slots
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (!slot || slot->empty)
				continue;

			// Проверяем расстояние до камеры - не тестируем слишком далекие объекты
			float distance = camera_pos.distance_to(slot->vis.sphere.P);
			if (distance > s_max_occlusion_distance)
				continue;

			int cache1_x = x / dm_cache1_count;
			int cache1_z = z / dm_cache1_count;
			CacheSlot1& parent_cache1 = m_cache_level1[cache1_z][cache1_x];

			if (!parent_cache1.occlusion_visible && parent_cache1.last_occlusion_test_frame > 0)
			{
				slot->occlusion_visible = false;
				slot->occlusion_visible_frames = 0;
				continue;
			}

			if ((m_occlusion_frame - slot->last_occlusion_test_frame) < s_occlusion_frame_interval)
				continue;

			slot->last_occlusion_test_frame = m_occlusion_frame;

			if (IsSlotVisible(slot))
			{
				m_occlusion_test_slots.push_back(slot);
				m_occlusion_queries.push_back(slot->occlusion_query);
				RenderHardwareOcclusionTest(slot->vis.sphere.P, slot->vis.sphere.R);
			}
			else
			{
				slot->occlusion_visible = false;
				slot->occlusion_visible_frames = 0;
			}
		}
	}
}

void CDetailManager::ProcessHardwareOcclusionResults()
{
	//if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return;

	// Process cache1 occlusion results
	for (u32 i = 0; i < m_occlusion_test_cache1.size(); i++)
	{
		CacheSlot1* cache1 = m_occlusion_test_cache1[i];
		if (!cache1 || !cache1->occlusion_query)
			continue;

		DWORD pixels_visible = 0;
		HRESULT hr = cache1->occlusion_query->GetData(&pixels_visible, sizeof(DWORD), D3DGETDATA_FLUSH);

		if (SUCCEEDED(hr) && hr != S_FALSE)
		{
			bool visible = (pixels_visible > 0);

			if (visible)
			{
				cache1->occlusion_visible_frames++;
			}
			else
			{
				cache1->occlusion_visible_frames = 0;
			}

			cache1->occlusion_visible = (cache1->occlusion_visible_frames >= s_occlusion_min_visible_frames);
		}
	}

	// Process slot occlusion results
	for (u32 i = 0; i < m_occlusion_test_slots.size(); i++)
	{
		Slot* slot = m_occlusion_test_slots[i];
		if (!slot || !slot->occlusion_query)
			continue;

		DWORD pixels_visible = 0;
		HRESULT hr = slot->occlusion_query->GetData(&pixels_visible, sizeof(DWORD), D3DGETDATA_FLUSH);

		if (SUCCEEDED(hr) && hr != S_FALSE)
		{
			bool visible = (pixels_visible > 0);

			if (visible)
			{
				slot->occlusion_visible_frames++;
			}
			else
			{
				slot->occlusion_visible_frames = 0;
			}

			slot->occlusion_visible = (slot->occlusion_visible_frames >= s_occlusion_min_visible_frames);
		}
	}
}

bool CDetailManager::IsSlotHardwareOccluded(Slot* slot)
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return false;

	if ((m_occlusion_frame - slot->last_occlusion_test_frame) > s_occlusion_frame_interval * 2)
		return false;

	return !slot->occlusion_visible;
}

bool CDetailManager::IsCache1HardwareOccluded(CacheSlot1* cache1)
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return false;

	if ((m_occlusion_frame - cache1->last_occlusion_test_frame) > s_occlusion_frame_interval * 2)
		return false;

	return !cache1->occlusion_visible;
}

void CDetailManager::RenderHardwareOcclusionTest(const Fvector& center, float radius)
{
	if (!m_occlusion_geom)
		return;

	// Проверяем валидность параметров
	if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(center.z) || !std::isfinite(radius) ||
		radius <= 0.0f || radius > 100.0f)
	{
		return;
	}

	// ВАЖНО: Полностью отключаем любой рендеринг в основной буфер
	// Используем альтернативный подход - рендерим в отдельную поверхность

	// Сохраняем ВСЕ состояния рендера
	IDirect3DSurface9* oldRT = nullptr;
	IDirect3DSurface9* oldDS = nullptr;
	DWORD oldColorWriteEnable, oldZEnable, oldZWriteEnable, oldAlphaBlendEnable;

	HW.pDevice->GetRenderTarget(0, &oldRT);
	HW.pDevice->GetDepthStencilSurface(&oldDS);
	HW.pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);
	HW.pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	HW.pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	HW.pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlendEnable);

	// Создаем временную поверхность для occlusion query (1x1 пиксель)
	IDirect3DSurface9* tempSurface = nullptr;
	IDirect3DSurface9* tempDepthSurface = nullptr;

	if (SUCCEEDED(HW.pDevice->CreateRenderTarget(1, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &tempSurface,
												 nullptr)) &&
		SUCCEEDED(HW.pDevice->CreateDepthStencilSurface(1, 1, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, FALSE,
														&tempDepthSurface, nullptr)))
	{
		// Устанавливаем временные поверхности как цели рендера
		HW.pDevice->SetRenderTarget(0, tempSurface);
		HW.pDevice->SetDepthStencilSurface(tempDepthSurface);

		// Очищаем поверхности
		HW.pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

		// Настраиваем рендер-стейты для невидимого рендера
		HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		HW.pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		HW.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		HW.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

		// Дополнительные настройки для полного отключения визуализации
		HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		HW.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		HW.pDevice->SetRenderState(D3DRS_CLIPPING, FALSE);

		// Создаем матрицу преобразования
		Fmatrix transform;
		transform.identity();
		transform.scale(radius, radius, radius);
		transform.translate_over(center);

		// Устанавливаем трансформации
		RenderBackend.set_xform_world(transform);

		// Используем максимально простой шейдер
		RenderBackend.set_Shader(Device.m_WireShader);
		RenderBackend.set_Geometry(m_occlusion_geom);

		// Начинаем occlusion query
		IDirect3DQuery9* current_query = m_occlusion_queries.back();
		if (current_query)
			current_query->Issue(D3DISSUE_BEGIN);

		// Рендерим (в временную поверхность 1x1)
		HW.pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

		// Заканчиваем occlusion query
		if (current_query)
			current_query->Issue(D3DISSUE_END);

		// Восстанавливаем поверхности рендера
		HW.pDevice->SetRenderTarget(0, oldRT);
		HW.pDevice->SetDepthStencilSurface(oldDS);

		// Освобождаем временные поверхности
		tempSurface->Release();
		tempDepthSurface->Release();
	}
	else
	{
		// Fallback: если не удалось создать поверхности, просто пропускаем тест
		if (tempSurface)
			tempSurface->Release();
		if (tempDepthSurface)
			tempDepthSurface->Release();
	}

	// Восстанавливаем ВСЕ состояния
	HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, oldColorWriteEnable);
	HW.pDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
	HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
	HW.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlendEnable);

	if (oldRT)
		oldRT->Release();
	if (oldDS)
		oldDS->Release();
}

void CDetailManager::RenderHardwareOcclusionTest(const Fvector& min, const Fvector& max)
{
	if (!m_occlusion_geom)
		return;

	// Проверяем валидность bounding box
	if (!std::isfinite(min.x) || !std::isfinite(min.y) || !std::isfinite(min.z) || !std::isfinite(max.x) ||
		!std::isfinite(max.y) || !std::isfinite(max.z))
	{
		return;
	}

	Fvector size;
	size.sub(max, min);
	if (size.x > 50.0f || size.y > 50.0f || size.z > 50.0f) // Еще больше уменьшили лимит
	{
		return;
	}

	// Тот же подход с временной поверхностью
	IDirect3DSurface9* oldRT = nullptr;
	IDirect3DSurface9* oldDS = nullptr;
	DWORD oldColorWriteEnable, oldZEnable, oldZWriteEnable, oldAlphaBlendEnable;

	HW.pDevice->GetRenderTarget(0, &oldRT);
	HW.pDevice->GetDepthStencilSurface(&oldDS);
	HW.pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);
	HW.pDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
	HW.pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
	HW.pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlendEnable);

	IDirect3DSurface9* tempSurface = nullptr;
	IDirect3DSurface9* tempDepthSurface = nullptr;

	if (SUCCEEDED(HW.pDevice->CreateRenderTarget(1, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &tempSurface,
												 nullptr)) &&
		SUCCEEDED(HW.pDevice->CreateDepthStencilSurface(1, 1, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, FALSE,
														&tempDepthSurface, nullptr)))
	{
		HW.pDevice->SetRenderTarget(0, tempSurface);
		HW.pDevice->SetDepthStencilSurface(tempDepthSurface);
		HW.pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

		HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		HW.pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		HW.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		HW.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		HW.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		HW.pDevice->SetRenderState(D3DRS_CLIPPING, FALSE);

		Fvector center;
		center.add(min, max).mul(0.5f);
		size.sub(max, min).mul(0.5f);

		Fmatrix transform;
		transform.identity();
		transform.scale(size);
		transform.translate_over(center);

		RenderBackend.set_xform_world(transform);
		RenderBackend.set_Shader(Device.m_WireShader);
		RenderBackend.set_Geometry(m_occlusion_geom);

		IDirect3DQuery9* current_query = m_occlusion_queries.back();
		if (current_query)
			current_query->Issue(D3DISSUE_BEGIN);

		HW.pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

		if (current_query)
			current_query->Issue(D3DISSUE_END);

		HW.pDevice->SetRenderTarget(0, oldRT);
		HW.pDevice->SetDepthStencilSurface(oldDS);

		tempSurface->Release();
		tempDepthSurface->Release();
	}
	else
	{
		if (tempSurface)
			tempSurface->Release();
		if (tempDepthSurface)
			tempDepthSurface->Release();
	}

	HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, oldColorWriteEnable);
	HW.pDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
	HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
	HW.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlendEnable);

	if (oldRT)
		oldRT->Release();
	if (oldDS)
		oldDS->Release();
}

////////////////////////////////////////////////////////////////////////////
// Unified Occlusion Interface
////////////////////////////////////////////////////////////////////////////

bool CDetailManager::IsSlotOccluded(Slot* slot)
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return false;

	if (m_primary_occlusion_method == OCCLUSION_HARDWARE_QUERY)
		return IsSlotHardwareOccluded(slot);

	return PerformOcclusionTest(slot);
}

bool CDetailManager::IsCache1Occluded(CacheSlot1* cache1)
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return false;

	if (m_primary_occlusion_method == OCCLUSION_HARDWARE_QUERY)
		return IsCache1HardwareOccluded(cache1);

	return PerformOcclusionTest(cache1);
}

void CDetailManager::UpdateOcclusionTests()
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return;

	m_occlusion_frame++;

	switch (m_primary_occlusion_method)
	{
	case OCCLUSION_HARDWARE_QUERY:
		UpdateHardwareOcclusionTests();
		break;

	case OCCLUSION_RASTER:
		UpdateSoftwareDepthBuffer();
		break;

	default:
		break;
	}
}

void CDetailManager::ProcessOcclusionResults()
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return;

	switch (m_primary_occlusion_method)
	{
	case OCCLUSION_HARDWARE_QUERY:
		ProcessHardwareOcclusionResults();
		break;

	default:
		break;
	}
}

void CDetailManager::RenderOcclusionQueries()
{
	if (!m_occlusion_enabled || !s_use_occlusion_culling)
		return;

	ProcessOcclusionResults();
}

////////////////////////////////////////////////////////////////////////////
// Occlusion System Management
////////////////////////////////////////////////////////////////////////////

void CDetailManager::InitializeOcclusionSystem()
{
	if (!s_use_occlusion_culling)
	{
		m_occlusion_enabled = false;
		return;
	}

	Msg("CDetailManager::InitializeOcclusionSystem() - Initializing software occlusion methods");

	// Загружаем только software методы
	LoadOcclusionGeometry();
	LoadOcclusionPortals();

	// Не создаем hardware resources
	DetectAvailableOcclusionMethods();

	// Принудительно устанавливаем software методы
	if (m_primary_occlusion_method == OCCLUSION_HARDWARE_QUERY)
	{
		m_primary_occlusion_method = OCCLUSION_HYBRID;
	}

	m_occlusion_enabled = true;
	m_occlusion_frame = 0;

	Msg("CDetailManager::InitializeOcclusionSystem() - Using occlusion method: %d", m_primary_occlusion_method);
}

void CDetailManager::CleanupOcclusionSystem()
{
	Msg("CDetailManager::CleanupOcclusionSystem() - Cleaning up multi-method occlusion system");

	// Release hardware queries
	for (int z = 0; z < dm_cache1_line; z++)
	{
		for (int x = 0; x < dm_cache1_line; x++)
		{
			CacheSlot1& cache1 = m_cache_level1[z][x];
			_RELEASE(cache1.occlusion_query);
		}
	}

	for (u32 i = 0; i < dm_cache_size; i++)
	{
		Slot& slot = m_cache_pool[i];
		_RELEASE(slot.occlusion_query);
	}

	// Cleanup software rasterizer
	CleanupSoftwareRasterizer();

	// Cleanup geometry
	m_occlusion_geom.destroy();
	_RELEASE(m_occlusion_ib);
	_RELEASE(m_occlusion_vb);

	// Clear data
	m_occlusion_geometries.clear();
	m_simple_occluders.clear();
	m_occlusion_portals.clear();
	m_occlusion_queries.clear();
	m_occlusion_test_slots.clear();
	m_occlusion_test_cache1.clear();

	Msg("CDetailManager::CleanupOcclusionSystem() - Multi-method occlusion system cleaned up");
}

////////////////////////////////////////////////////////////////////////////
// Visibility Management
////////////////////////////////////////////////////////////////////////////

void CDetailManager::UpdateVisibleM()
{
	if (!g_pGameLevel || !Device.Statistic || !m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	Device.Statistic->RenderDUMP_DT_VIS.Begin();

	ClearVisibleLists();

	UpdateFrustumPlanes();

	if (m_occlusion_enabled && s_use_occlusion_culling)
	{
		UpdateOcclusionTests();
	}

	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* S = m_cache[z][x];
			if (!S || S->empty)
				continue;

			if (!IsSlotVisible(S))
				continue;

			if (IsSlotOccluded(S))
				continue;

			if (Device.dwFrame > S->frame)
			{
				S->frame = Device.dwFrame;

				for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
				{
					SlotPart& sp = S->G[sp_id];
					if (sp.id == DetailSlot::ID_Empty)
						continue;

					for (int lod = 0; lod < 3; lod++)
						sp.r_items[lod].clear();

					for (auto& item_ptr : sp.items)
					{
						SlotItem& Item = *item_ptr;
						Item.scale_calculated = Item.scale;
						sp.r_items[0].push_back(item_ptr);
					}
				}
			}

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

////////////////////////////////////////////////////////////////////////////
// Hardware Rendering Implementation
////////////////////////////////////////////////////////////////////////////

short CDetailManager::QuantConstant(float v)
{
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

	HRESULT hr = HW.pDevice->CreateVertexBuffer(totalVertices * sizeof(vertHW), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED,
												&m_hw_VB, 0);
	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create vertex buffer (HR: 0x%08X)", hr);
		return;
	}

	hr = HW.pDevice->CreateIndexBuffer(totalIndices * sizeof(u16), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED,
									   &m_hw_IB, 0);
	if (FAILED(hr))
	{
		Msg("![ERROR] CDetailManager::hw_Load() - Failed to create index buffer (HR: 0x%08X)", hr);
		_RELEASE(m_hw_VB);
		return;
	}

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
				pVerts->t = QuantConstant((vP.y - D.bv_bb.min.y) * t_scale);
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

	m_hw_Geom.create(GetDetailsVertexDecl(), m_hw_VB, m_hw_IB);
	m_hw_Geom_Instanced.create(GetDetailsInstancedVertexDecl(), m_hw_VB, m_hw_IB);

	Msg("CDetailManager::hw_Load() - Completed successfully");
}

void CDetailManager::hw_Unload()
{
	Msg("CDetailManager::hw_Unload() - Start");

	m_hw_Geom.destroy();
	m_hw_Geom_Instanced.destroy();

	_RELEASE(m_hw_VB_Instances);
	_RELEASE(m_hw_IB);
	_RELEASE(m_hw_VB);

	m_hw_BatchSize = 0;

	Msg("CDetailManager::hw_Unload() - Completed");
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

void CDetailManager::hw_Render()
{
	if (!psDeviceFlags.is(rsDetails))
		return;

	if (!m_hw_Geom_Instanced || !m_loaded || !m_initialized || m_bInLoad || m_bShuttingDown)
		return;

	try
	{
		RenderBackend.set_Geometry(m_hw_Geom_Instanced);
		hw_Render_dump();
	}
	catch (...)
	{
		Msg("![ERROR] CDetailManager::hw_Render() exception caught");
	}
}

void CDetailManager::hw_Render_dump()
{
	Device.Statistic->RenderDUMP_DT_Count = 0;

	Fmatrix mWorld = Fidentity;
	Fmatrix mView = Device.mView;
	Fmatrix mProject = Device.mProject;
	Fmatrix mWorldViewProject = Device.mFullTransform;
	Fmatrix mWorldView;
	mWorldView.mul(mView, mWorld);

	Fvector4 wind_params = GetWindParams();
	Fvector4 wind_turbulence = GetWindTurbulence();

	RenderBackend.get_ConstantCache_Vertex().force_dirty();
	RenderBackend.get_ConstantCache_Pixel().force_dirty();

	for (u32 O = 0; O < m_objects.size(); O++)
	{
		CDetail& Object = *m_objects[O];

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

			RenderBackend.set_Element(Object.shader->E[shader_id]);
			RenderBackend.get_ConstantCache_Vertex().force_dirty();
			RenderBackend.get_ConstantCache_Pixel().force_dirty();

			RenderBackend.set_Constant("m_WorldViewProject", mWorldViewProject);
			RenderBackend.set_Constant("m_WorldView", mWorldView);

			if (!Object.m_Flags.is(DO_NO_WAVING))
			{
				RenderBackend.set_Constant("m_wind_turbulence", wind_turbulence);
				RenderBackend.set_Constant("m_wind_params", wind_params);
			}

			RenderBackend.flush();

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

					HW.pDevice->SetStreamSource(0, m_hw_VB, 0, sizeof(vertHW));
					HW.pDevice->SetStreamSource(1, m_hw_VB_Instances, 0, sizeof(InstanceData));

					HW.pDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | batch_instance_count);
					HW.pDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul);

					hr = HW.pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices,
														  iOffset, Object.number_indices / 3);

					if (SUCCEEDED(hr))
					{
						Device.Statistic->RenderDUMP_DT_Count += batch_instance_count;
					}

					HW.pDevice->SetStreamSourceFreq(0, 1);
					HW.pDevice->SetStreamSourceFreq(1, 1);
				}
			}
		}
	}

	for (u32 lod = 0; lod < 3; lod++)
	{
		for (u32 obj = 0; obj < m_objects.size(); obj++)
		{
			m_render_visibles[lod][obj].clear();
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// Core System Methods
////////////////////////////////////////////////////////////////////////////

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

	for (u32 i = 0; i < 3; ++i)
	{
		m_visibles[i].resize(m_objects.size());
		m_render_visibles[i].resize(m_objects.size());
	}

	cache_Initialize();

	hw_Load();

	if (s_use_occlusion_culling)
	{
		m_occlusion_enabled = true;
		InitializeOcclusionSystem();
	}
	else
	{
		m_occlusion_enabled = false;
	}

	m_initialized = true;
	m_bInLoad = FALSE;

	Msg("CDetailManager::Load() - Internal load completed, loaded %d objects", m_objects.size());
#endif
}

void CDetailManager::Unload()
{
	Msg("CDetailManager::Unload() - Start");

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
	CleanupOcclusionSystem();
	hw_Unload();

	for (u32 i = 0; i < dm_cache_size; i++)
	{
		Slot& slot = m_cache_pool[i];
		for (u32 j = 0; j < dm_obj_in_slot; j++)
		{
			SlotPart& sp = slot.G[j];
			for (auto item : sp.items)
			{
				m_poolSI.destroy(item);
			}
			sp.items.clear();

			for (int lod = 0; lod < 3; lod++)
				sp.r_items[lod].clear();
		}
	}

	for (DetailIt it = m_objects.begin(); it != m_objects.end(); it++)
	{
		(*it)->Unload();
		xr_delete(*it);
	}
	m_objects.clear();

	for (u32 i = 0; i < 3; ++i)
	{
		m_visibles[i].clear();
		m_render_visibles[i].clear();
	}

	m_cache_task.clear();

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

		if (m_occlusion_enabled && s_use_occlusion_culling)
		{
			RenderOcclusionQueries();
		}
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
	CleanupOcclusionSystem();
}

void CDetailManager::OnDeviceResetEnd()
{
	Msg("CDetailManager::OnDeviceResetEnd()");
	if (m_loaded && !m_bShuttingDown)
	{
		hw_Load();
		if (s_use_occlusion_culling)
		{
			InitializeOcclusionSystem();
		}
	}
	m_MT.Leave();
}

////////////////////////////////////////////////////////////////////////////
// Cache System Implementation
////////////////////////////////////////////////////////////////////////////

void CDetailManager::cache_Initialize()
{
	m_cache_cx = 0;
	m_cache_cz = 0;

	Slot* slt = m_cache_pool;
	for (u32 i = 0; i < dm_cache_line; i++)
		for (u32 j = 0; j < dm_cache_line; j++, slt++)
		{
			m_cache[i][j] = slt;
			cache_Task(j, i, slt);
		}

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

	D->empty = (DS.id0 == DetailSlot::ID_Empty) && (DS.id1 == DetailSlot::ID_Empty) &&
			   (DS.id2 == DetailSlot::ID_Empty) && (DS.id3 == DetailSlot::ID_Empty);

	D->type = stReady;
	D->sx = sx;
	D->sz = sz;

	D->vis.box.min.set(sx * dm_slot_size, DS.r_ybase(), sz * dm_slot_size);
	D->vis.box.max.set(D->vis.box.min.x + dm_slot_size, DS.r_ybase() + DS.r_yheight(), D->vis.box.min.z + dm_slot_size);
	D->vis.box.grow(0.05f);
	D->vis.box.getsphere(D->vis.sphere.P, D->vis.sphere.R);

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
			MS.vis.box.invalidate();

			int nullCount = 0;
			int validCount = 0;

			for (int _i = 0; _i < dm_cache1_count * dm_cache1_count; _i++)
			{
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

			if (!MS.vis.box.is_valid())
			{
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

void CDetailManager::UpdateCache()
{
	if (m_bShuttingDown || !m_loaded || !m_initialized || m_bInLoad)
		return;

	Fvector EYE;

	if (g_pGamePersistent)
		EYE = Device.vCameraPosition;
	else
		return;

	if (!std::isfinite(EYE.x) || !std::isfinite(EYE.y) || !std::isfinite(EYE.z) || EYE.square_magnitude() < 0.01f)
		return;

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

	s_x = std::max(-1024, std::min(1024, s_x));
	s_z = std::max(-1024, std::min(1024, s_z));

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

void CDetailManager::cache_Update(int v_x, int v_z, Fvector& view, int limit)
{
	if (m_bShuttingDown || !m_loaded || !m_initialized)
		return;

	v_x = std::max(-1024, std::min(1024, v_x));
	v_z = std::max(-1024, std::min(1024, v_z));

	int dx = v_x - m_cache_cx;
	int dz = v_z - m_cache_cz;

	if (abs(dx) >= dm_cache_line || abs(dz) >= dm_cache_line)
	{
		m_cache_cx = v_x;
		m_cache_cz = v_z;

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

		UpdateCacheLevel1Bounds();
		return;
	}

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
					m_cache[z - 1][x] = m_cache[z][x];
				}
				m_cache[dm_cache_line - 1][x] = S;
				cache_Task(x, dm_cache_line - 1, S);
			}
		}
	}

	if (m_initialized)
	{
		UpdateCacheLevel1Bounds();
	}

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

	Fvector bC, bD;
	D.vis.box.get_CD(bC, bD);

	m_xrc.box_options(CDB::OPT_FULL_TEST);
	m_xrc.box_query(g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);
	u32 triCount = m_xrc.r_count();

	if (0 == triCount)
		return;

	CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
	Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();

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

			Item.scale = r_scale.randF(Dobj->m_fMinScale, Dobj->m_fMaxScale);
			Item.scale_calculated = Item.scale;

			Item.mRotY.identity();
			Item.mRotY.rotateY(r_yaw.randF(0, PI_MUL_2));
			Item.mRotY.translate_over(Item_P);

			Fmatrix mScale, mXform;
			Fbox ItemBB;
			mScale.scale(Item.scale, Item.scale, Item.scale);
			mXform.mul_43(Item.mRotY, mScale);
			ItemBB.xform(Dobj->bv_bb, mXform);
			Bounds.merge(ItemBB);

			Item.c_hemi = GetSlotHemi(DS);
			Item.c_sun = GetSlotSun(DS);
			Item.vis_ID = 0;

			D.G[index].items.push_back(ItemP);
		}
	}

	if (Bounds.is_valid())
	{
		D.vis.box.set(Bounds);
		D.vis.box.grow(0.05f);
		D.vis.box.getsphere(D.vis.sphere.P, D.vis.sphere.R);
	}

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

////////////////////////////////////////////////////////////////////////////
// Update and Utility Methods
////////////////////////////////////////////////////////////////////////////

void __stdcall CDetailManager::Update()
{
	static volatile LONG updateGuard = 0;

	if (m_bShuttingDown)
		return;

	if (InterlockedExchange(&updateGuard, 1) == 1)
		return;

	__try
	{
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

void CDetailManager::SafeUpdate()
{
	Update();
}

////////////////////////////////////////////////////////////////////////////
// Occlusion Control and Statistics
////////////////////////////////////////////////////////////////////////////

void CDetailManager::SetOcclusionMethod(EOcclusionMethod method)
{
	if (method >= OCCLUSION_METHOD_COUNT)
		return;

	s_preferred_occlusion_method = method;

	if (m_occlusion_enabled)
	{
		CleanupOcclusionSystem();
		InitializeOcclusionSystem();
	}
}

u32 CDetailManager::GetOccludedSlotCount() const
{
	u32 count = 0;
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (slot && !slot->empty && !slot->occlusion_visible)
				count++;
		}
	}
	return count;
}

u32 CDetailManager::GetVisibleSlotCount() const
{
	u32 count = 0;
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (slot && !slot->empty && slot->occlusion_visible)
				count++;
		}
	}
	return count;
}

void CDetailManager::DumpOcclusionStats()
{
	u32 total_slots = 0;
	u32 visible_slots = 0;
	u32 occluded_slots = 0;

	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* slot = m_cache[z][x];
			if (slot && !slot->empty)
			{
				total_slots++;
				if (slot->occlusion_visible)
					visible_slots++;
				else
					occluded_slots++;
			}
		}
	}

	Msg("CDetailManager Occlusion Stats:");
	Msg("  Total slots: %d", total_slots);
	Msg("  Visible slots: %d (%.1f%%)", visible_slots, total_slots > 0 ? (visible_slots * 100.0f / total_slots) : 0.0f);
	Msg("  Occluded slots: %d (%.1f%%)", occluded_slots,
		total_slots > 0 ? (occluded_slots * 100.0f / total_slots) : 0.0f);
	Msg("  Occlusion method: %d", m_primary_occlusion_method);
	Msg("  Confidence threshold: %.2f", s_occlusion_confidence_threshold);
}
