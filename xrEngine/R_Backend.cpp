#include "stdafx.h"
#pragma hdrstop

ENGINE_API CBackend RenderBackend;

// Create Quad-IB
void CBackend::CreateQuadIB()
{
	OPTICK_EVENT("CBackend::CreateQuadIB");

	const u32 dwTriCount = 4 * 1024;
	const u32 dwIdxCount = dwTriCount * 2 * 3;
	u16* Indices = 0;
	u32 dwUsage = D3DUSAGE_WRITEONLY;
	if (HW.Caps.geometry.bSoftware)
		dwUsage |= D3DUSAGE_SOFTWAREPROCESSING;
	R_CHK(HW.pDevice->CreateIndexBuffer(dwIdxCount * 2, dwUsage, D3DFMT_INDEX16, D3DPOOL_MANAGED, &QuadIB, NULL));
	R_CHK(QuadIB->Lock(0, 0, (void**)&Indices, 0));
	{
		int Cnt = 0;
		int ICnt = 0;
		for (int i = 0; i < dwTriCount; i++)
		{
			Indices[ICnt++] = u16(Cnt + 0);
			Indices[ICnt++] = u16(Cnt + 1);
			Indices[ICnt++] = u16(Cnt + 2);

			Indices[ICnt++] = u16(Cnt + 3);
			Indices[ICnt++] = u16(Cnt + 2);
			Indices[ICnt++] = u16(Cnt + 1);

			Cnt += 4;
		}
	}
	R_CHK(QuadIB->Unlock());
}

// Device dependance
void CBackend::OnDeviceCreate()
{
	OPTICK_EVENT("CBackend::OnDeviceCreate");

	CreateQuadIB();

	// streams
	Vertex.Create();
	Index.Create();

	// invalidate caching
	Invalidate();

	g_viewport.create(FVF::F_TL, Vertex.Buffer(), QuadIB);
}

void CBackend::OnDeviceDestroy()
{
	OPTICK_EVENT("CBackend::OnDeviceDestroy");

	// streams
	Index.Destroy();
	Vertex.Destroy();

	// Quad
	_RELEASE(QuadIB);
}

void CBackend::DeleteResources()
{
	g_viewport.destroy();
}