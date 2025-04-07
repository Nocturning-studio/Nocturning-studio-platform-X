#ifndef r_backendH
#define r_backendH
#pragma once

// #define RBackend_PGO

#ifdef RBackend_PGO
#define PGO(a) a
#else
#define PGO(a)
#endif

#include "R_Backend_Data_Streams.h"
#include "r_constants_cache.h"
#include "r_backend_xform.h"
#include "r_backend_hemi.h"
#include "r_backend_tree.h"
#include "fvf.h"

const u32 CULL_CCW = D3DCULL_CCW;
const u32 CULL_CW = D3DCULL_CW;
const u32 CULL_NONE = D3DCULL_NONE;

///		detailed statistic
struct R_statistics_element
{
	u32 verts, dips;
	ICF void add(u32 _verts)
	{
		verts += _verts;
		dips++;
	}
};
struct R_statistics
{
	R_statistics_element s_static;
	R_statistics_element s_flora;
	R_statistics_element s_flora_lods;
	R_statistics_element s_details;
	R_statistics_element s_ui;
	R_statistics_element s_dynamic;
	R_statistics_element s_dynamic_sw;
	R_statistics_element s_dynamic_inst;
	R_statistics_element s_dynamic_1B;
	R_statistics_element s_dynamic_2B;
};

class ENGINE_API CBackend
{
  public:
	// Dynamic geometry streams
	VertexStream Vertex;
	IndexStream Index;
	IDirect3DIndexBuffer9* QuadIB;
	IDirect3DIndexBuffer9* old_QuadIB;
	R_xforms xforms;
	R_hemi hemi;
	R_tree tree;

	ref_geom g_viewport;

  private:
	// Render-targets
	IDirect3DSurface9* pRT[4];
	IDirect3DSurface9* pZB;

	// Vertices/Indices/etc
	IDirect3DVertexDeclaration9* decl;
	IDirect3DVertexBuffer9* vb;
	IDirect3DIndexBuffer9* ib;
	u32 vb_stride;

	// Pixel/Vertex constants
	ALIGN(16) R_constants constants;
	R_constant_table* ctable;

	// Shaders/State
	IDirect3DStateBlock9* state;
	IDirect3DPixelShader9* ps;
	IDirect3DVertexShader9* vs;
#ifdef DEBUG
	LPCSTR ps_name;
	LPCSTR vs_name;
#endif
	u32 stencil_enable;
	u32 stencil_func;
	u32 stencil_ref;
	u32 stencil_mask;
	u32 stencil_writemask;
	u32 stencil_fail;
	u32 stencil_pass;
	u32 stencil_zfail;
	u32 colorwrite_mask;
	u32 cull_mode;
	bool zwrite;

	// Lists
	STextureList* T;
	SMatrixList* M;
	SConstantList* C;

	// Lists-expanded
	CTexture* textures_ps[16]; // stages
	CTexture* textures_vs[5];  // dmap + 4 vs
#ifdef _EDITOR
	CMatrix* matrices[8]; // matrices are supported only for FFP
#endif

	void Invalidate();

  public:
	struct _stats
	{
		u32 polys;
		u32 verts;
		u32 calls;
		u32 vs;
		u32 ps;
#ifdef DEBUG
		u32 decl;
		u32 vb;
		u32 ib;
		u32 states;	   // Number of times the shader-state changes
		u32 textures;  // Number of times the shader-tex changes
		u32 matrices;  // Number of times the shader-xform changes
		u32 constants; // Number of times the shader-consts changes
#endif
		u32 xforms;
		u32 target_rt;
		u32 target_zb;

		R_statistics r;
	} stat;

  public:
	IC CTexture* get_ActiveTexture(u32 stage)
	{
		if (stage >= 256)
			return textures_vs[stage - 256];
		else
			return textures_ps[stage];
	}
	IC R_constant_array& get_ConstantCache_Vertex()
	{
		return constants.a_vertex;
	}
	IC R_constant_array& get_ConstantCache_Pixel()
	{
		return constants.a_pixel;
	}

	// API
	IC void set_xform(u32 ID, const Fmatrix& M);
	IC void set_xform_world(const Fmatrix& M);
	IC void set_xform_view(const Fmatrix& M);
	IC void set_xform_project(const Fmatrix& M);
	IC const Fmatrix& get_xform_world();
	IC const Fmatrix& get_xform_view();
	IC const Fmatrix& get_xform_project();

	IC void setRenderTarget(IDirect3DSurface9* RT, u32 ID = 0);
	IC void setDepthBuffer(IDirect3DSurface9* ZB);

	IC void set_Constants(R_constant_table* C);
	IC void set_Constants(ref_ctable& CTable)
	{
		set_Constants(&*CTable);
	}

	void set_Textures(STextureList* T);
	IC void set_Textures(ref_texture_list& TexList)
	{
		set_Textures(&*TexList);
	}

#ifdef _EDITOR
	IC void set_Matrices(SMatrixList* M);
	IC void set_Matrices(ref_matrix_list& M)
	{
		set_Matrices(&*M);
	}
#endif

	IC void set_Element(ShaderElement* S, u32 pass = 0);
	IC void set_Element(ref_selement& S, u32 pass = 0)
	{
		set_Element(&*S, pass);
	}

	IC void set_Shader(Shader* S, u32 pass = 0);
	IC void set_Shader(ref_shader& S, u32 pass = 0)
	{
		set_Shader(&*S, pass);
	}

	ICF void set_States(IDirect3DStateBlock9* _state);
	ICF void set_States(ref_state& _state)
	{
		set_States(_state->state);
	}

	ICF void set_Format(IDirect3DVertexDeclaration9* _decl);

	ICF void set_Pixel_Shader(IDirect3DPixelShader9* _ps, LPCSTR _n = 0);
	ICF void set_Pixel_Shader(ref_ps& _ps)
	{
		set_Pixel_Shader(_ps->sh, _ps->cName.c_str());
	}

	ICF void set_Vertex_Shader(IDirect3DVertexShader9* _vs, LPCSTR _n = 0);
	ICF void set_Vertex_Shader(ref_vs& _vs)
	{
		set_Vertex_Shader(_vs->sh, _vs->cName.c_str());
	}

	ICF void set_Vertices(IDirect3DVertexBuffer9* _vb, u32 _vb_stride);
	ICF void set_Indices(IDirect3DIndexBuffer9* _ib);
	ICF void set_Geometry(SGeometry* _geom);
	ICF void set_Geometry(ref_geom& _geom)
	{
		set_Geometry(&*_geom);
	}
	IC void set_Stencil(u32 _enable, u32 _func = D3DCMP_ALWAYS, u32 _ref = 0x00, u32 _mask = 0x00,
						u32 _writemask = 0x00, u32 _fail = D3DSTENCILOP_KEEP, u32 _pass = D3DSTENCILOP_KEEP,
						u32 _zfail = D3DSTENCILOP_KEEP);
	IC void set_ColorWriteEnable(u32 _mask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
											 D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
	IC void set_ZWriteEnable(bool state);
	IC void set_CullMode(u32 _mode);
	IC u32 get_CullMode()
	{
		return cull_mode;
	}
	void set_ClipPlanes(u32 _enable, Fplane* _planes = NULL, u32 count = 0);
	void set_ClipPlanes(u32 _enable, Fmatrix* _xform = NULL, u32 fmask = 0xff);
	IC void set_Scissor(Irect* rect = NULL);

	void enable_anisotropy_filtering();
	void disable_anisotropy_filtering();
	void set_anisotropy_filtering(int max_anisothropy);

	// constants
	ICF ref_constant get_Constant(LPCSTR n)
	{
		if (ctable)
			return ctable->get(n);
		else
			return 0;
	}
	ICF ref_constant get_Constant(shared_str& n)
	{
		if (ctable)
			return ctable->get(n);
		else
			return 0;
	}

	// constants - direct (fast)
	ICF void set_Constant(R_constant* Const, const Fmatrix& A)
	{
		if (Const)
			constants.set(Const, A);
	}
	ICF void set_Constant(R_constant* Const, const Fvector4& A)
	{
		if (Const)
			constants.set(Const, A);
	}
	ICF void set_Constant(R_constant* Const, float x, float y = NULL, float z = NULL, float w = NULL)
	{
		if (Const)
			constants.set(Const, x, y, z, w);
	}
	ICF void set_Array_Constant(R_constant* Const, u32 e, const Fmatrix& A)
	{
		if (Const)
			constants.seta(Const, e, A);
	}
	ICF void set_Array_Constant(R_constant* Const, u32 e, const Fvector4& A)
	{
		if (Const)
			constants.seta(Const, e, A);
	}
	ICF void set_Array_Constant(R_constant* Const, u32 e, float x, float y, float z, float w)
	{
		if (Const)
			constants.seta(Const, e, x, y, z, w);
	}

	// constants - LPCSTR (slow)
	ICF void set_Constant(LPCSTR n, const Fmatrix& A)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), A);
	}
	ICF void set_Constant(LPCSTR n, const Fvector4& A)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), A);
	}
	ICF void set_Constant(LPCSTR n, float x)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), x, 0, 0, 0);
	}
	ICF void set_Constant(LPCSTR n, float x, float y)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), x, y, 0, 0);
	}
	ICF void set_Constant(LPCSTR n, float x, float y, float z)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), x, y, z, 0);
	}
	ICF void set_Constant(LPCSTR n, float x, float y, float z, float w)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), x, y, z, w);
	}
	ICF void set_Array_Constant(LPCSTR n, u32 e, const Fmatrix& A)
	{
		if (ctable)
			set_Array_Constant(&*ctable->get(n), e, A);
	}
	ICF void set_Array_Constant(LPCSTR n, u32 e, const Fvector4& A)
	{
		if (ctable)
			set_Array_Constant(&*ctable->get(n), e, A);
	}
	ICF void set_Array_Constant(LPCSTR n, u32 e, float x, float y, float z, float w)
	{
		if (ctable)
			set_Array_Constant(&*ctable->get(n), e, x, y, z, w);
	}

	// constants - shared_str (average)
	ICF void set_Constant(shared_str& n, const Fmatrix& A)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), A);
	}
	ICF void set_Constant(shared_str& n, const Fvector4& A)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), A);
	}
	ICF void set_Constant(shared_str& n, float x, float y, float z, float w)
	{
		if (ctable)
			set_Constant(&*ctable->get(n), x, y, z, w);
	}
	ICF void set_Array_Constant(shared_str& n, u32 e, const Fmatrix& A)
	{
		if (ctable)
			set_Array_Constant(&*ctable->get(n), e, A);
	}
	ICF void set_Array_Constant(shared_str& n, u32 e, const Fvector4& A)
	{
		if (ctable)
			set_Array_Constant(&*ctable->get(n), e, A);
	}
	ICF void set_Array_Constant(shared_str& n, u32 e, float x, float y, float z, float w)
	{
		if (ctable)
			set_Array_Constant(&*ctable->get(n), e, x, y, z, w);
	}

	ICF void Render(D3DPRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC);
	ICF void Render(D3DPRIMITIVETYPE T, u32 startV, u32 PC);

	// Device create / destroy / frame signaling
	void CreateQuadIB();
	void OnFrameBegin();
	void OnFrameEnd();
	void OnDeviceCreate();
	void OnDeviceDestroy();
	void DeleteResources();

	// Debug render
	void dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc);
	void dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC);
	IC void dbg_SetRS(D3DRENDERSTATETYPE p1, u32 p2)
	{
		CHK_DX(HW.pDevice->SetRenderState(p1, p2));
	}
	IC void dbg_SetSS(u32 sampler, D3DSAMPLERSTATETYPE type, u32 value)
	{
		CHK_DX(HW.pDevice->SetSamplerState(sampler, type, value));
	}
#ifdef DEBUG
	void dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int vcnt, u16* pIdx, int pcnt);
	void dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int pcnt);
	IC void dbg_DrawAABB(Fvector& T, float sx, float sy, float sz, u32 C)
	{
		Fvector half_dim;
		half_dim.set(sx, sy, sz);
		Fmatrix TM;
		TM.translate(T);
		dbg_DrawOBB(TM, half_dim, C);
	}
	void dbg_DrawOBB(Fmatrix& T, Fvector& half_dim, u32 C);
	IC void dbg_DrawTRI(Fmatrix& T, Fvector* p, u32 C)
	{
		dbg_DrawTRI(T, p[0], p[1], p[2], C);
	}
	void dbg_DrawTRI(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C);
	void dbg_DrawLINE(Fmatrix& T, Fvector& p1, Fvector& p2, u32 C);
	void dbg_DrawEllipse(Fmatrix& T, u32 C);
#endif

	CBackend()
	{
		Invalidate();
	};

	void u_compute_texgen_screen(Fmatrix& dest);
	void set_viewport_geometry(float w, float h, ref_geom geometry, u32& vOffset);

	void set_Render_Target_Surface(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL);
	void set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* _1, IDirect3DSurface9* _2 = NULL, IDirect3DSurface9* _3 = NULL, IDirect3DSurface9* _4 = NULL);
	void set_Depth_Buffer(IDirect3DSurface9* zb);

	void set_viewport_geometry(float w, float h, u32& vOffset);
	void set_viewport_geometry(ref_geom geometry, u32& vOffset);
	void set_viewport_geometry(u32& vOffset);

	void RenderViewportSurface();
	void RenderViewportSurface(const ref_rt& _1);
	void RenderViewportSurface(const ref_rt& _1, IDirect3DSurface9* zb);
	void RenderViewportSurface(float w, float h, const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL);

	void RenderViewportSurface(float w, float h, IDirect3DSurface9* _1);
	void RenderViewportSurface(float w, float h, IDirect3DSurface9* _1, IDirect3DSurface9* zb);

	void ClearTexture(const ref_rt& _1, u32 color = color_rgba(0, 0, 0, 0));
	void ClearTexture(const ref_rt& _1, const ref_rt& _2 = NULL, u32 color = color_rgba(0, 0, 0, 0));
	void ClearTexture(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, u32 color = color_rgba(0, 0, 0, 0));
	void ClearTexture(const ref_rt& _1, const ref_rt& _2 = NULL, const ref_rt& _3 = NULL, const ref_rt& _4 = NULL, u32 color = color_rgba(0, 0, 0, 0));

};

extern ENGINE_API CBackend RenderBackend;

#ifndef _EDITOR
#include "D3DUtils.h"
#endif

#endif
