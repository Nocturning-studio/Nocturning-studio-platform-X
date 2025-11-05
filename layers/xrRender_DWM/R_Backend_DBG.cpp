#include "stdafx.h"
#pragma hdrstop

void CBackend::dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc)
{
	RCache.set_Geometry		(geom);
	RCache.Render			(pt,vBase,pc);
}

void CBackend::dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
	RCache.set_Geometry		(geom);
	RCache.Render			(pt,baseV,startV,countV,startI,PC);
}

#ifdef DEBUG

void CBackend::dbg_Draw			(D3DPRIMITIVETYPE T, FVF::L* pVerts, int vcnt, u16* pIdx, int pcnt)
{

	//	TODO: DX10: implement
	//VERIFY(!"CBackend::dbg_Draw not implemented.");

}
void CBackend::dbg_Draw			(D3DPRIMITIVETYPE T, FVF::L* pVerts, int pcnt)
{

	//	TODO: DX10: implement
	//VERIFY(!"CBackend::dbg_Draw not implemented.");

}

#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
void CBackend::dbg_DrawOBB		(Fmatrix& T, Fvector& half_dim, u32 C)
{
	Fmatrix mL2W_Transform,mScaleTransform;

	mScaleTransform.scale(half_dim);
	mL2W_Transform.mul_43(T,mScaleTransform);

	FVF::L  aabb[8];
	aabb[0].set( -1, -1, -1, C ); // 0
	aabb[1].set( -1, +1, -1, C ); // 1
	aabb[2].set( +1, +1, -1, C ); // 2
	aabb[3].set( +1, -1, -1, C ); // 3
	aabb[4].set( -1, -1, +1, C ); // 4
	aabb[5].set( -1, +1, +1, C ); // 5
	aabb[6].set( +1, +1, +1, C ); // 6
	aabb[7].set( +1, -1, +1, C ); // 7

	u16		aabb_id[12*2] = {
		0,1,  1,2,  2,3,  3,0,  4,5,  5,6,  6,7,  7,4,  1,5,  2,6,  3,7,  0,4
	};
	set_xform_world	(mL2W_Transform);
	dbg_Draw(D3DPT_LINELIST,aabb,8,aabb_id,12);
}
void CBackend::dbg_DrawTRI	(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C)
{
	FVF::L	tri[3];
	tri[0].p = p1; tri[0].color = C;
	tri[1].p = p2; tri[1].color = C;
	tri[2].p = p3; tri[2].color = C;

	set_xform_world	(T);
	dbg_Draw(D3DPT_TRIANGLESTRIP,tri,1);
}
void CBackend::dbg_DrawLINE(Fmatrix& T, Fvector& p1, Fvector& p2, u32 C)
{
	FVF::L	line[2];
	line[0].p = p1; line[0].color = C;
	line[1].p = p2; line[1].color = C;

	set_xform_world	(T);
	dbg_Draw(D3DPT_LINELIST,line,1);
}
void CBackend::dbg_DrawEllipse(Fmatrix& T, u32 C)
{
	float gVertices[] =
	{
		0.0000f,0.0000f,1.0000f,  0.0000f,0.3827f,0.9239f,  -0.1464f,0.3536f,0.9239f,
			-0.2706f,0.2706f,0.9239f,  -0.3536f,0.1464f,0.9239f,  -0.3827f,0.0000f,0.9239f,
			-0.3536f,-0.1464f,0.9239f,  -0.2706f,-0.2706f,0.9239f,  -0.1464f,-0.3536f,0.9239f,
			0.0000f,-0.3827f,0.9239f,  0.1464f,-0.3536f,0.9239f,  0.2706f,-0.2706f,0.9239f,
			0.3536f,-0.1464f,0.9239f,  0.3827f,0.0000f,0.9239f,  0.3536f,0.1464f,0.9239f,
			0.2706f,0.2706f,0.9239f,  0.1464f,0.3536f,0.9239f,  0.0000f,0.7071f,0.7071f,
			-0.2706f,0.6533f,0.7071f,  -0.5000f,0.5000f,0.7071f,  -0.6533f,0.2706f,0.7071f,
			-0.7071f,0.0000f,0.7071f,  -0.6533f,-0.2706f,0.7071f,  -0.5000f,-0.5000f,0.7071f,
			-0.2706f,-0.6533f,0.7071f,  0.0000f,-0.7071f,0.7071f,  0.2706f,-0.6533f,0.7071f,
			0.5000f,-0.5000f,0.7071f,  0.6533f,-0.2706f,0.7071f,  0.7071f,0.0000f,0.7071f,
			0.6533f,0.2706f,0.7071f,  0.5000f,0.5000f,0.7071f,  0.2706f,0.6533f,0.7071f,
			0.0000f,0.9239f,0.3827f,  -0.3536f,0.8536f,0.3827f,  -0.6533f,0.6533f,0.3827f,
			-0.8536f,0.3536f,0.3827f,  -0.9239f,0.0000f,0.3827f,  -0.8536f,-0.3536f,0.3827f,
			-0.6533f,-0.6533f,0.3827f,  -0.3536f,-0.8536f,0.3827f,  0.0000f,-0.9239f,0.3827f,
			0.3536f,-0.8536f,0.3827f,  0.6533f,-0.6533f,0.3827f,  0.8536f,-0.3536f,0.3827f,
			0.9239f,0.0000f,0.3827f,  0.8536f,0.3536f,0.3827f,  0.6533f,0.6533f,0.3827f,
			0.3536f,0.8536f,0.3827f,  0.0000f,1.0000f,0.0000f,  -0.3827f,0.9239f,0.0000f,
			-0.7071f,0.7071f,0.0000f,  -0.9239f,0.3827f,0.0000f,  -1.0000f,0.0000f,0.0000f,
			-0.9239f,-0.3827f,0.0000f,  -0.7071f,-0.7071f,0.0000f,  -0.3827f,-0.9239f,0.0000f,
			0.0000f,-1.0000f,0.0000f,  0.3827f,-0.9239f,0.0000f,  0.7071f,-0.7071f,0.0000f,
			0.9239f,-0.3827f,0.0000f,  1.0000f,0.0000f,0.0000f,  0.9239f,0.3827f,0.0000f,
			0.7071f,0.7071f,0.0000f,  0.3827f,0.9239f,0.0000f,  0.0000f,0.9239f,-0.3827f,
			-0.3536f,0.8536f,-0.3827f,  -0.6533f,0.6533f,-0.3827f,  -0.8536f,0.3536f,-0.3827f,
			-0.9239f,0.0000f,-0.3827f,  -0.8536f,-0.3536f,-0.3827f,  -0.6533f,-0.6533f,-0.3827f,
			-0.3536f,-0.8536f,-0.3827f,  0.0000f,-0.9239f,-0.3827f,  0.3536f,-0.8536f,-0.3827f,
			0.6533f,-0.6533f,-0.3827f,  0.8536f,-0.3536f,-0.3827f,  0.9239f,0.0000f,-0.3827f,
			0.8536f,0.3536f,-0.3827f,  0.6533f,0.6533f,-0.3827f,  0.3536f,0.8536f,-0.3827f,
			0.0000f,0.7071f,-0.7071f,  -0.2706f,0.6533f,-0.7071f,  -0.5000f,0.5000f,-0.7071f,
			-0.6533f,0.2706f,-0.7071f,  -0.7071f,0.0000f,-0.7071f,  -0.6533f,-0.2706f,-0.7071f,
			-0.5000f,-0.5000f,-0.7071f,  -0.2706f,-0.6533f,-0.7071f,  0.0000f,-0.7071f,-0.7071f,
			0.2706f,-0.6533f,-0.7071f,  0.5000f,-0.5000f,-0.7071f,  0.6533f,-0.2706f,-0.7071f,
			0.7071f,0.0000f,-0.7071f,  0.6533f,0.2706f,-0.7071f,  0.5000f,0.5000f,-0.7071f,
			0.2706f,0.6533f,-0.7071f,  0.0000f,0.3827f,-0.9239f,  -0.1464f,0.3536f,-0.9239f,
			-0.2706f,0.2706f,-0.9239f,  -0.3536f,0.1464f,-0.9239f,  -0.3827f,0.0000f,-0.9239f,
			-0.3536f,-0.1464f,-0.9239f,  -0.2706f,-0.2706f,-0.9239f,  -0.1464f,-0.3536f,-0.9239f,
			0.0000f,-0.3827f,-0.9239f,  0.1464f,-0.3536f,-0.9239f,  0.2706f,-0.2706f,-0.9239f,
			0.3536f,-0.1464f,-0.9239f,  0.3827f,0.0000f,-0.9239f,  0.3536f,0.1464f,-0.9239f,
			0.2706f,0.2706f,-0.9239f,  0.1464f,0.3536f,-0.9239f,  0.0000f,0.0000f,-1.0000f
	};


	const int vcnt = sizeof(gVertices)/(sizeof(float)*3);
	FVF::L  verts[vcnt];
	for (int i=0; i<vcnt; i++) {
		int k=i*3;
		verts[i].set(gVertices[k],gVertices[k+1],gVertices[k+2],C);
	}

	set_xform_world				(T);


	//	TODO: DX10: implement
	//VERIFY(!"CBackend::dbg_Draw not implemented.");
	//dbg_Draw(D3DPT_TRIANGLELIST,verts,vcnt,gFaces,224);

}

#endif
