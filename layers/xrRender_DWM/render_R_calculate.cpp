#include "stdafx.h"
#include "../xrEngine/customhud.h"

float				g_fSCREEN		;

extern float		r_dtex_range	;
extern float		r_ssaDISCARD	;
extern float		r_ssaDONTSORT	;
extern float		r_ssaLOD_A		;
extern float		r_ssaLOD_B		;
extern float		r_ssaHZBvsTEX	;
extern float		r_ssaGLOD_start,	r_ssaGLOD_end;

void CRender::Calculate		()
{
	// Transfer to global space to avoid deep pointer access
	IRender_Target* T				=	getTarget	();
	float	fov_factor				=	_sqr		(90.f / Device.fFOV);
	g_fSCREEN						=	float(T->get_width()*T->get_height())*fov_factor*(EPS_S+ r__geometry_lod);
	r_ssaDISCARD					=	_sqr(r__ssa_discard)		/g_fSCREEN;
	r_ssaDONTSORT					=	_sqr(r__ssa_dontsort /3)	/g_fSCREEN;
	r_ssaLOD_A						=	_sqr(r__ssa_lod_a /3)		/g_fSCREEN;
	r_ssaLOD_B						=	_sqr(r__ssa_lod_b /3)		/g_fSCREEN;
	r_ssaGLOD_start					=	_sqr(r__ssa_glod_start /3)/g_fSCREEN;
	r_ssaGLOD_end					=	_sqr(r__ssa_glod_end /3)	/g_fSCREEN;
	r_ssaHZBvsTEX					=	_sqr(r__ssa_hzb_vs_tex /3)	/g_fSCREEN;
	r_dtex_range					=	75.f * g_fSCREEN / (1024.f * 768.f);
	
	// Detect camera-sector
	if (!vLastCameraPos.similar(Device.vCameraPosition,EPS_S)) 
	{
		CSector* pSector		= (CSector*)detectSector(Device.vCameraPosition);
		if (pSector && (pSector!=pLastSector))
			g_pGamePersistent->OnSectorChanged( translateSector(pSector) );

		if (0==pSector) pSector = pLastSector;
		pLastSector = pSector;
		vLastCameraPos.set(Device.vCameraPosition);
	}

	// Check if camera is too near to some portal - if so force DualRender
	if (rmPortals) 
	{
		float	eps			= VIEWPORT_NEAR+EPS_L;
		Fvector box_radius; box_radius.set(eps,eps,eps);
		Sectors_xrc.box_options	(CDB::OPT_FULL_TEST);
		Sectors_xrc.box_query	(rmPortals,Device.vCameraPosition,box_radius);
		for (int K=0; K<Sectors_xrc.r_count(); K++)	{
			CPortal*	pPortal		= (CPortal*) Portals[rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
			pPortal->bDualRender	= TRUE;
		}
	}

	//
	Lights.Update();

	// Check if we touch some light even trough portal
	lstRenderables.clear();
	g_SpatialSpace->q_sphere(lstRenderables,0,STYPE_LIGHTSOURCE,Device.vCameraPosition,EPS_L);
	for (u32 _it=0; _it<lstRenderables.size(); _it++)	{
		ISpatial*	spatial		= lstRenderables[_it];		spatial->spatial_updatesector	();
		CSector*	sector		= (CSector*)spatial->spatial.sector;
		if	(0==sector)										continue;	// disassociated from S/P structure

		VERIFY							(spatial->spatial.type & STYPE_LIGHTSOURCE);
		// lightsource
		light*			L				= (light*)	(spatial->dcast_Light());
		VERIFY							(L);
		Lights.add_light				(L);
	}
}
