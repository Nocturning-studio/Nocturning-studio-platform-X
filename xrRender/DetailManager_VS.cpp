#include "stdafx.h"
#pragma hdrstop

#include "detailmanager.h"

#ifdef _EDITOR
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif

const int quant = 16384;
const int c_hdr = 10;
const int c_size = 4;

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

#pragma pack(push, 1)
struct vertHW
{
	float x, y, z;
	short u, v, t, mid;
};
#pragma pack(pop)

short QC(float v)
{
	int t = iFloor(v * float(quant));
	clamp(t, -32768, 32767);
	return short(t & 0xffff);
}

void CDetailManager::hw_Load()
{
	hw_BatchSize = 512;

	u32 dwVerts = 0;
	u32 dwIndices = 0;
	for (u32 o = 0; o < objects.size(); o++)
	{
		CDetail& D = *objects[o];
		dwVerts += D.number_vertices;
		dwIndices += D.number_indices;
	}

	u32 dwUsage = D3DUSAGE_WRITEONLY;

	R_CHK(HW.pDevice->CreateVertexBuffer(dwVerts * sizeof(vertHW), dwUsage, 0, D3DPOOL_MANAGED, &hw_VB, 0));
	R_CHK(HW.pDevice->CreateIndexBuffer(dwIndices * 2, dwUsage, D3DFMT_INDEX16, D3DPOOL_MANAGED, &hw_IB, 0));
	R_CHK(HW.pDevice->CreateVertexBuffer(hw_BatchSize * sizeof(InstanceData), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
										 D3DPOOL_DEFAULT, &hw_VB_Instances, 0));

	{
		vertHW* pV;
		R_CHK(hw_VB->Lock(0, 0, (void**)&pV, 0));
		for (u32 o = 0; o < objects.size(); o++)
		{
			CDetail& D = *objects[o];
			for (u32 v = 0; v < D.number_vertices; v++)
			{
				Fvector& vP = D.vertices[v].P;
				pV->x = vP.x;
				pV->y = vP.y;
				pV->z = vP.z;
				pV->u = QC(D.vertices[v].u);
				pV->v = QC(D.vertices[v].v);
				pV->t = QC(vP.y / (D.bv_bb.max.y - D.bv_bb.min.y));
				pV->mid = 0;
				pV++;
			}
		}
		R_CHK(hw_VB->Unlock());
	}

	{
		u16* pI;
		R_CHK(hw_IB->Lock(0, 0, (void**)(&pI), 0));
		for (u32 o = 0; o < objects.size(); o++)
		{
			CDetail& D = *objects[o];
			for (u32 i = 0; i < u32(D.number_indices); i++)
				*pI++ = u16(D.indices[i]);
		}
		R_CHK(hw_IB->Unlock());
	}

	hw_Geom.create(dwDecl_Details, hw_VB, hw_IB);
	hw_Geom_Instanced.create(dwDecl_Details_Instanced, hw_VB, hw_IB);
}

void CDetailManager::hw_Unload()
{
	hw_Geom.destroy();
	hw_Geom_Instanced.destroy();
	_RELEASE(hw_VB_Instances);
	_RELEASE(hw_IB);
	_RELEASE(hw_VB);
}

void CDetailManager::hw_Render()
{
	if (!psDeviceFlags.is(rsDetails))
		return;

	RenderBackend.set_Geometry(hw_Geom_Instanced);
	hw_Render_dump_instanced_combined();
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

void CDetailManager::hw_Render_dump_instanced_combined()
{
	Device.Statistic->RenderDUMP_DT_Count = 0;

    // Устанавливаем глобальные константы один раз
	Fmatrix mWorld = RenderBackend.get_xform_world();
	Fmatrix mView = Device.mView;
	Fmatrix mProject = Device.mProject;
	Fmatrix mFullTransform = Device.mFullTransform;

	Fvector4 wind_params = GetWindParams();
	Fvector4 wind_turbulence = GetWindTurbulence();

	// ВАЖНО: Правильные матрицы для инстансинга
	// Для view space позиции нам нужна WorldView матрица
	Fmatrix mWorldView;
	mWorldView.mul(mView, mWorld); // World * View

	// Для проекции нужна полная матрица WorldViewProject
	Fmatrix mWorldViewProject;
	mWorldViewProject.mul(mProject, mWorldView); // World * View * Project

	// ОСНОВНОЙ ЦИКЛ РЕНДЕРИНГА
	for (u32 O = 0; O < objects.size(); O++)
	{
		CDetail& Object = *objects[O];

		// ВЫЧИСЛЯЕМ СМЕЩЕНИЯ ДЛЯ КАЖДОГО ОБЪЕКТА
		u32 vOffset = 0;
		u32 iOffset = 0;
		for (u32 i = 0; i < O; i++)
		{
			vOffset += objects[i]->number_vertices;
			iOffset += objects[i]->number_indices;
		}

		// Рендерим все LOD уровни для этого объекта
		for (u32 lod_id = 0; lod_id < 3; lod_id++)
		{
			vis_list& list = m_visibles[lod_id];
			xr_vector<SlotItemVec*>& vis = list[O];

			if (vis.empty())
				continue;

			// Выбираем правильный шейдер
			int shader_id;
			if (Object.m_Flags.is(DO_NO_WAVING))
			{
				shader_id = SE_DETAIL_NORMAL_STATIC;
			}
			else
			{
				shader_id = SE_DETAIL_NORMAL_ANIMATED;
			}

			if (!Object.shader || !Object.shader->E[shader_id])
			{
				vis.clear();
				continue;
			}

			// ВРЕМЕННЫЙ ВЕКТОР для инстансов
			xr_vector<InstanceData> instances;
			instances.reserve(512);

			xr_vector<SlotItemVec*>::iterator _vI = vis.begin();
			xr_vector<SlotItemVec*>::iterator _vE = vis.end();
			for (; _vI != _vE; _vI++)
			{
				SlotItemVec* items = *_vI;
				SlotItemVecIt _iI = items->begin();
				SlotItemVecIt _iE = items->end();
				for (; _iI != _iE; _iI++)
				{
					SlotItem& Instance = **_iI;

					InstanceData inst{};
					float scale = Instance.scale_calculated;
					Fmatrix& M = Instance.mRotY;

					// Матрица инстанса уже включает позицию и вращение
					inst.mat0.set(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
					inst.mat1.set(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
					inst.mat2.set(M._13 * scale, M._23 * scale, M._33 * scale, M._43);

					float h = Instance.c_hemi;
					float s = Instance.c_sun;
					inst.color.set(s, s, s, h);
					inst.lod_id = (float)lod_id;

					instances.push_back(inst);
				}
			}

			if (instances.empty())
			{
				vis.clear();
				continue;
			}

			RenderBackend.set_Element(Object.shader->E[shader_id]);

			// Устанавливаем матрицы через регистры
			RenderBackend.set_Constant("mat_WorldView", mWorldView);
			RenderBackend.set_Constant("mat_WorldViewProject", mWorldViewProject);

			// Константы ветра
			if (!Object.m_Flags.is(DO_NO_WAVING))
			{
				RenderBackend.set_Constant_Register_VS(20, wind_turbulence);
				RenderBackend.set_Constant_Register_VS(21, wind_params);
			}

			u32 total_instances = instances.size();
			u32 batch_count = (total_instances + hw_BatchSize - 1) / hw_BatchSize;

			for (u32 batch = 0; batch < batch_count; batch++)
			{
				u32 start_instance = batch * hw_BatchSize;
				u32 end_instance = std::min(start_instance + hw_BatchSize, total_instances);
				u32 batch_instance_count = end_instance - start_instance;

				InstanceData* pInstances;
				R_CHK(hw_VB_Instances->Lock(0, batch_instance_count * sizeof(InstanceData), (void**)&pInstances,
											D3DLOCK_DISCARD));

				CopyMemory(pInstances, &instances[start_instance], batch_instance_count * sizeof(InstanceData));
				R_CHK(hw_VB_Instances->Unlock());

				HW.pDevice->SetStreamSource(0, hw_VB, 0, sizeof(vertHW));
				HW.pDevice->SetStreamSource(1, hw_VB_Instances, 0, sizeof(InstanceData));

				HW.pDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | batch_instance_count);
				HW.pDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul);

				HRESULT hr = HW.pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices,
															  iOffset, Object.number_indices / 3);

				HW.pDevice->SetStreamSourceFreq(0, 1);
				HW.pDevice->SetStreamSourceFreq(1, 1);

				Device.Statistic->RenderDUMP_DT_Count += batch_instance_count;
				RenderBackend.stat.r.s_details.add(Object.number_vertices * batch_instance_count);
			}

			vis.clear();
		}
	}
}
