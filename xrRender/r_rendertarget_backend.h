///////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////////////////////////
#define DU_SPHERE_NUMVERTEX 92
#define DU_SPHERE_NUMFACES 180
#define DU_CONE_NUMVERTEX 18
#define DU_CONE_NUMFACES 32
///////////////////////////////////////////////////////////////////////////////////
class CRenderTargetBackend
{
  private:
	u32 dwWidth;
	u32 dwHeight;

  public:
	ref_geom g_viewport;

#ifdef DEBUG
	struct dbg_line_t
	{
		Fvector P0, P1;
		u32 color;
	};
	xr_vector<std::pair<Fsphere, Fcolor>> dbg_spheres;
	xr_vector<dbg_line_t> dbg_lines;
	xr_vector<Fplane> dbg_planes;
#endif

  public:
	CRenderTargetBackend();
	~CRenderTargetBackend();

	void u_compute_texgen_screen(Fmatrix& dest);
	void u_compute_texgen_jitter(Fmatrix& dest);

	void set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL);
	void set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2 = NULL, IDirect3DSurface9* _3 = NULL, IDirect3DSurface9* _4 = NULL);
	void set_Depth_Buffer(IDirect3DSurface9* zb);

	void set_viewport_geometry(float w, float h, u32& vOffset);
	void set_viewport_geometry(u32& vOffset);

	void RenderViewportSurface();
	void RenderViewportSurface(const ref_rt& _1);
	void RenderViewportSurface(float w, float h, const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL);

	void RenderViewportSurface(float w, float h, IDirect3DSurface9* _1);
	void RenderViewportSurface(float w, float h, IDirect3DSurface9* _1, IDirect3DSurface9* zb);

	void ClearTexture(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL, u32 color = color_rgba(0, 0, 0, 0));

#ifdef DEBUG
	IC void dbg_addline(Fvector& P0, Fvector& P1, u32 c)
	{
		dbg_lines.push_back(dbg_line_t());
		dbg_lines.back().P0 = P0;
		dbg_lines.back().P1 = P1;
		dbg_lines.back().color = c;
	}
	IC void dbg_addplane(Fplane& P0, u32 c)
	{
		dbg_planes.push_back(P0);
	}
#else
	IC void dbg_addline(Fvector& P0, Fvector& P1, u32 c)
	{
	}
	IC void dbg_addplane(Fplane& P0, u32 c)
	{
	}
#endif
};
///////////////////////////////////////////////////////////////////////////////////
extern CRenderTargetBackend* RenderTargetBackend;
///////////////////////////////////////////////////////////////////////////////////
