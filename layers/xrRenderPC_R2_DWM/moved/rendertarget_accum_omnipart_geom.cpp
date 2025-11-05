#include "stdafx.h"
#include "du_sphere_part.h"


#include "BufferUtils.h"


void CRenderTarget::accum_omnip_geom_create		()
{
	// vertices
	{
		u32		vCount		= DU_SPHERE_PART_NUMVERTEX;
		u32		vSize		= 3*4;

		R_CHK(BufferUtils::CreateBuffer( &g_accum_omnip_vb, du_sphere_part_vertices, vCount*vSize, D3D_BIND_VERTEX_BUFFER));

	}

	// Indices
	{
		u32		iCount		= DU_SPHERE_PART_NUMFACES*3;

		R_CHK(BufferUtils::CreateBuffer( &g_accum_omnip_ib, du_sphere_part_faces, iCount*2, D3D_BIND_INDEX_BUFFER ));

	}
}

void CRenderTarget::accum_omnip_geom_destroy()
{
#ifdef DEBUG
	_SHOW_REF("g_accum_omnip_ib", g_accum_omnip_ib);
#endif // DEBUG

	_RELEASE(g_accum_omnip_ib);
#ifdef DEBUG
	_SHOW_REF("g_accum_omnip_vb", g_accum_omnip_vb);
#endif // DEBUG

	_RELEASE(g_accum_omnip_vb);
}
