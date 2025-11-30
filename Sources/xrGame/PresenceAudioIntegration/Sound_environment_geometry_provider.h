/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment_geometry_provider.h
====================================================================================================
  Author: NSDeathman & Gemini 3
  Description: Реализация интерфейса IGeometryProvider.
  Этот класс выступает мостом между Presence Audio SDK и физическим движком X-Ray (CDB/ObjectSpace).
====================================================================================================

  Copyright (c) 2025 Nocturning Studio, NSDeathman & Gemini 3

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  1. The above copyright notice and this permission notice shall be included in all
	 copies or substantial portions of the Software.

  2. Any project (commercial, free, open-source, or closed-source) using this Software
	 must include attribution to "Presence Audio SDK by Nocturning Studio" in its
	 documentation, credits, or about screen.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

====================================================================================================
  Developed by: NSDeathman (Architecture & Core), Gemini 3 (Optimization & Math)
  Organization: Nocturning Studio
====================================================================================================
*/
#pragma once
#include "stdafx.h"

// Подключение API библиотеки звука
#include <PresenceAudioSDK/PresenceAudioAPI.h>

// Подключение заголовков движка X-Ray для доступа к геометрии и материалам
#include "../xrEngine/igame_level.h"
#include "../xrEngine/xr_area.h"
#include "..\xrGame\GameMtlLib.h"

// =================================================================================================
// ADAPTER CLASS (X-Ray -> Presence)
// =================================================================================================
class XRayGeometryAdapter : public Presence::IGeometryProvider
{
  public:
	// ---------------------------------------------------------------------------------------------
	// Кэш материалов (Material Cache)
	// ---------------------------------------------------------------------------------------------
	// В движке X-Ray материалы хранятся как строки (LPCSTR).
	// Сравнивать строки (strstr) при каждом трассировке луча — экстремально медленно.
	// Поэтому мы создаем массив (Lookup Table), где индекс соответствует ID материала (u16),
	// а значение — это наш enum Presence::MaterialType.
	// Это превращает поиск материала из O(N) в O(1).
	// ---------------------------------------------------------------------------------------------
	xr_vector<Presence::MaterialType> m_MaterialCache;
	bool m_bCacheBuilt;

	XRayGeometryAdapter() : m_bCacheBuilt(false)
	{
	}

	// ---------------------------------------------------------------------------------------------
	// Построение кэша (Build Cache)
	// ---------------------------------------------------------------------------------------------
	// Анализирует все материалы, загруженные в GMLib, и сопоставляет их с типами Presence.
	// Вызывается один раз при первом обращении (Lazy Initialization).
	// ---------------------------------------------------------------------------------------------
	void BuildMaterialCache()
	{
		if (m_bCacheBuilt)
			return;

		Msg("[Presence EAX] Building material cache...");

		// Получаем общее количество материалов в игре
		u32 mtlCount = GMLib.CountMaterial();
		m_MaterialCache.reserve(mtlCount);

		for (u32 i = 0; i < mtlCount; i++)
		{
			SGameMtl* mtl = GMLib.GetMaterialByIdx(i);
			Presence::MaterialType type = Presence::MaterialType::Stone; // Дефолтный материал (Бетон)

			if (mtl)
			{
				// Эвристический анализ имени материала.
				// Имена в X-Ray обычно имеют формат "materials\wood_plank", "materials\concrete_wall" и т.д.
				LPCSTR name = mtl->m_Name.c_str();

				if (strstr(name, "wood") || strstr(name, "trees") || strstr(name, "plank"))
					type = Presence::MaterialType::Wood;
				else if (strstr(name, "metal") || strstr(name, "grate") || strstr(name, "tin") || strstr(name, "pipe"))
					type = Presence::MaterialType::Metal;
				else if (strstr(name, "glass") || strstr(name, "ice") || strstr(name, "window"))
					type = Presence::MaterialType::Glass;
				else if (strstr(name, "earth") || strstr(name, "grass") || strstr(name, "bush") ||
						 strstr(name, "water") || strstr(name, "cloth") || strstr(name, "fabric"))
					type = Presence::MaterialType::Soft;
				// Проверка на специальный материал "Абсорбер" (Студийная звукоизоляция)
				else if (strstr(name, "absorber") || strstr(name, "foam") || strstr(name, "padding"))
					type = Presence::MaterialType::Absorber;
				else if (strstr(name, "stone") || strstr(name, "brick") || strstr(name, "concrete") ||
						 strstr(name, "asphalt"))
					type = Presence::MaterialType::Stone;
			}

			// Индекс в векторе соответствует ID материала
			m_MaterialCache.push_back(type);
		}

		m_bCacheBuilt = true;
		Msg("[Presence EAX] Material cache built. Total mapped materials: %d", m_MaterialCache.size());
	}

	// ---------------------------------------------------------------------------------------------
	// Трассировка луча (Ray Casting Implementation)
	// ---------------------------------------------------------------------------------------------
	// Главный метод, вызываемый библиотекой PresenceAudioSDK.
	// Переводит запрос из формата SDK в формат X-Ray и возвращает физические данные.
	// ---------------------------------------------------------------------------------------------
	virtual Presence::RayHit CastRay(const Presence::float3& start, const Presence::float3& dir, float maxDist) override
	{
		// Ленивая инициализация кэша, если он еще не построен
		if (!m_bCacheBuilt)
			BuildMaterialCache();

		Presence::RayHit result;
		result.isHit = false;
		result.distance = maxDist;

		// Защита от вызова, когда уровень не загружен
		if (!g_pGameLevel)
			return result;

		// 1. Конвертация типов данных (Presence Vector -> X-Ray Vector)
		Fvector xStart, xDir;
		xStart.set(start.x, start.y, start.z);
		xDir.set(dir.x, dir.y, dir.z);

		collide::rq_result rq;

		// 2. Выполнение запроса к физическому движку (ObjectSpace)
		// rqtStatic - нас интересует только статическая геометрия (стены, пол, террейн).
		// Динамические объекты (ящики, бочки) обычно игнорируются для глобальной акустики.
		// Также учитываем особенности хрея - для статики используется BVH, а для динамики нет
		// поэтому запрос к ней будет дороже
		BOOL hit = g_pGameLevel->ObjectSpace.RayPick(xStart, xDir, maxDist, collide::rqtStatic, rq, NULL);

		if (hit)
		{
			result.isHit = true;
			result.distance = rq.range;

			// 3. Получение информации о треугольнике
			// rq.element - это индекс треугольника в глобальном массиве статики
			CDB::TRI* tri = g_pGameLevel->ObjectSpace.GetStaticTris() + rq.element;

			// 4. Расчет нормали поверхности
			// Нормаль нужна для расчета отскоков звука (Reflection).
			// Берем 3 вершины треугольника и считаем перпендикуляр.
			Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
			Fvector xNorm;
			xNorm.mknormal(verts[tri->verts[0]], verts[tri->verts[1]], verts[tri->verts[2]]);

			// Конвертация обратно в Presence Vector
			result.normal = Presence::float3(xNorm.x, xNorm.y, xNorm.z);

			// 5. Определение материала
			// tri->material хранит ID материала (u16).
			u16 mtl_idx = (u16)tri->material;

			if (mtl_idx < m_MaterialCache.size())
			{
				// Быстрый поиск в кэше O(1)
				result.material = m_MaterialCache[mtl_idx];
			}
			else
			{
				// Fallback (Запасной вариант): Если ID материала некорректен.
				// Используем простую геометрическую эвристику:
				// Если стена вертикальная (dot product с вектором взгляда) - скорее всего камень.
				float cos_angle = _abs(xDir.dotproduct(xNorm));
				if (rq.range < 2.0f && cos_angle > 0.6f)
					result.material = Presence::MaterialType::Stone;
				else
					result.material = Presence::MaterialType::Stone;
			}
		}

		return result;
	}
};
