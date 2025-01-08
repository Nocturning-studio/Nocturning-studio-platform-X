#ifndef R_BACKEND_RUNTIMEH
#define R_BACKEND_RUNTIMEH
#pragma once

#include <optick/optick.h>
#include "sh_texture.h"
#include "sh_matrix.h"
#include "sh_constant.h"
#include "sh_rt.h"

IC void R_xforms::set_c_w(R_constant* C)
{
	OPTICK_EVENT("R_xforms::set_c_w");

	c_w = C;
	RCache.set_c(C, m_w);
};
IC void R_xforms::set_c_invw(R_constant* C)
{
	OPTICK_EVENT("R_xforms::set_c_invw");

	c_invw = C;
	apply_invw();
};
IC void R_xforms::set_c_v(R_constant* C)
{
	OPTICK_EVENT("R_xforms::set_c_v");

	c_v = C;
	RCache.set_c(C, m_v);
};
IC void R_xforms::set_c_p(R_constant* C)
{
	OPTICK_EVENT("R_xforms::set_c_p");

	c_p = C;
	RCache.set_c(C, m_p);
};
IC void R_xforms::set_c_wv(R_constant* C)
{
	OPTICK_EVENT("R_xforms::set_c_wv");

	c_wv = C;
	RCache.set_c(C, m_wv);
};
IC void R_xforms::set_c_vp(R_constant* C)
{
	OPTICK_EVENT("R_xforms::set_c_vp");

	c_vp = C;
	RCache.set_c(C, m_vp);
};
IC void R_xforms::set_c_wvp(R_constant* C)
{
	OPTICK_EVENT("R_xforms::set_c_wvp");

	c_wvp = C;
	RCache.set_c(C, m_wvp);
};

IC void CBackend::set_xform(u32 ID, const Fmatrix& Matrix)
{
	OPTICK_EVENT("CBackend::set_xform");

	stat.xforms++;
	CHK_DX(HW.pDevice->SetTransform((D3DTRANSFORMSTATETYPE)ID, (D3DMATRIX*)&Matrix));
}
IC void CBackend::set_xform_world(const Fmatrix& Matrix)
{
	OPTICK_EVENT("CBackend::set_xform_world");

	xforms.set_W(Matrix);
}
IC void CBackend::set_xform_view(const Fmatrix& Matrix)
{
	OPTICK_EVENT("CBackend::set_xform_view");

	xforms.set_V(Matrix);
}
IC void CBackend::set_xform_project(const Fmatrix& Matrix)
{
	OPTICK_EVENT("CBackend::set_xform_project");

	xforms.set_P(Matrix);
}
IC const Fmatrix& CBackend::get_xform_world()
{
	OPTICK_EVENT("CBackend::get_xform_world");

	return xforms.get_W();
}
IC const Fmatrix& CBackend::get_xform_view()
{
	OPTICK_EVENT("CBackend::get_xform_view");

	return xforms.get_V();
}
IC const Fmatrix& CBackend::get_xform_project()
{
	OPTICK_EVENT("CBackend::get_xform_project");

	return xforms.get_P();
}

IC void CBackend::set_RT(IDirect3DSurface9* RT, u32 ID)
{		
	OPTICK_EVENT("CBackend:setRT");

	if (RT != pRT[ID])
	{
		stat.target_rt++;
		pRT[ID] = RT;
		CHK_DX(HW.pDevice->SetRenderTarget(ID, RT));
	}
}

IC void CBackend::set_ZB(IDirect3DSurface9* ZB)
{
	OPTICK_EVENT("CBackend:setZB");

	if (ZB != pZB)
	{
		stat.target_zb++;
		pZB = ZB;
		CHK_DX(HW.pDevice->SetDepthStencilSurface(ZB));
	}
}

ICF void CBackend::set_States(IDirect3DStateBlock9* _state)
{
	OPTICK_EVENT("CBackend:set_States");

	if (state != _state)
	{
#ifdef DEBUG
		stat.states++;
#endif
		state = _state;
		state->Apply();
	}
}

#ifdef _EDITOR
IC void CBackend::set_Matrices(SMatrixList* _M)
{
	if (M != _M)
	{
		M = _M;
		if (M)
		{
			for (u32 it = 0; it < M->size(); it++)
			{
				CMatrix* mat = &*((*M)[it]);
				if (mat && matrices[it] != mat)
				{
					matrices[it] = mat;
					mat->Calculate();
					set_xform(D3DTS_TEXTURE0 + it, mat->xform);
					stat.matrices++;
				}
			}
		}
	}
}
#endif

IC void CBackend::set_Constants(R_constant_table* ConstTable)
{
	OPTICK_EVENT("CBackend::set_Constants");

	// caching
	if (ctable == ConstTable)
		return;

	ctable = ConstTable;
	xforms.unmap();

	if (0 == ConstTable)
		return;

	// process constant-loaders
	R_constant_table::c_table::iterator it = ConstTable->table.begin();
	R_constant_table::c_table::iterator end = ConstTable->table.end();
	for (; it != end; it++)
	{
		R_constant* Constant = &**it;
		if (Constant->handler)
			Constant->handler->setup(Constant);
	}
}

IC void CBackend::set_Element(ShaderElement* S, u32 pass)
{
	OPTICK_EVENT("CBackend::set_Element");

	SPass& P = *(S->passes[pass]);
	set_States(P.state);
	set_PS(P.ps);
	set_VS(P.vs);
	set_Constants(P.constants);
	set_Textures(P.T);
#ifdef _EDITOR
	set_Matrices(P.M);
#endif
}

ICF void CBackend::set_Format(IDirect3DVertexDeclaration9* _decl)
{
	OPTICK_EVENT("CBackend::set_Format");

	if (decl != _decl)
	{
#ifdef DEBUG
		stat.decl++;
#endif
		decl = _decl;
		CHK_DX(HW.pDevice->SetVertexDeclaration(decl));
	}
}

ICF void CBackend::set_PS(IDirect3DPixelShader9* _ps, LPCSTR _n)
{
	OPTICK_EVENT("CBackend::set_PS");

	if (ps != _ps)
	{
		//shared_str str;
		//str.sprintf("CBackend::set_PS: %s", _n);
		//OPTICK_EVENT_DYNAMIC(str.c_str());

		stat.ps++;
		ps = _ps;
		CHK_DX(HW.pDevice->SetPixelShader(ps));
#ifdef DEBUG
		ps_name = _n;
#endif
	}
}

ICF void CBackend::set_VS(IDirect3DVertexShader9* _vs, LPCSTR _n)
{
	OPTICK_EVENT("CBackend::set_VS");

	if (vs != _vs)
	{
		//shared_str str;
		//str.sprintf("CBackend::set_VS: %s", _n);
		//OPTICK_EVENT_DYNAMIC(str.c_str());

		stat.vs++;
		vs = _vs;
		CHK_DX(HW.pDevice->SetVertexShader(vs));
#ifdef DEBUG
		vs_name = _n;
#endif
	}
}

ICF void CBackend::set_Vertices(IDirect3DVertexBuffer9* _vb, u32 _vb_stride)
{
	OPTICK_EVENT("CBackend::set_Vertices");

	if ((vb != _vb) || (vb_stride != _vb_stride))
	{
		//LPSTR event_name;
		//sprintf(event_name, "CBackend::set_Vertices:%x,%d", _vb, _vb_stride);
		//OPTICK_EVENT(event_name);
#ifdef DEBUG
		stat.vb++;
#endif
		vb = _vb;
		vb_stride = _vb_stride;
		CHK_DX(HW.pDevice->SetStreamSource(0, vb, 0, vb_stride));
	}
}

ICF void CBackend::set_Indices(IDirect3DIndexBuffer9* _ib)
{
	OPTICK_EVENT("CBackend::set_Indices");

	if (ib != _ib)
	{
		//LPSTR event_name;
		//sprintf(event_name, "CBackend::set_Indices:%x", _ib);
		//OPTICK_EVENT(event_name);

#ifdef DEBUG
		stat.ib++;
#endif
		ib = _ib;
		CHK_DX(HW.pDevice->SetIndices(ib));
	}
}

ICF void CBackend::Render(D3DPRIMITIVETYPE PrimitiveType, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
	OPTICK_EVENT("CBackend::Render");

	stat.calls++;
	stat.verts += countV;
	stat.polys += PC;
	constants.flush();
	CHK_DX(HW.pDevice->DrawIndexedPrimitive(PrimitiveType, baseV, startV, countV, startI, PC));

	//LPSTR event_name;
	//sprintf(event_name, "CBackend::Render:%dv/%df", countV, PC);
	//OPTICK_EVENT(event_name);
}

ICF void CBackend::Render(D3DPRIMITIVETYPE PrimitiveType, u32 startV, u32 PC)
{
	OPTICK_EVENT("CBackend::Render");

	stat.calls++;
	stat.verts += 3 * PC;
	stat.polys += PC;
	constants.flush();
	CHK_DX(HW.pDevice->DrawPrimitive(PrimitiveType, startV, PC));

	//LPSTR event_name;
	//sprintf(event_name, "CBackend::Render:%dv/%df", 3 * PC, PC);
	//OPTICK_EVENT(event_name);
}

ICF void CBackend::set_Shader(Shader* S, u32 pass)
{
	OPTICK_EVENT("CBackend::set_Shader");

	set_Element(S->E[0], pass);
}

IC void CBackend::set_Geometry(SGeometry* _geom)
{
	OPTICK_EVENT("CBackend::set_Geometry");

	set_Format(_geom->dcl._get()->dcl);
	set_Vertices(_geom->vb, _geom->vb_stride);
	set_Indices(_geom->ib);
}

IC void CBackend::set_Scissor(Irect* R)
{
	OPTICK_EVENT("CBackend::set_Scissor");

	if (R)
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE));
		RECT* clip = (RECT*)R;
		CHK_DX(HW.pDevice->SetScissorRect(clip));
	}
	else
	{
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE));
	}
}

IC void CBackend::set_Stencil(u32 _enable, u32 _func, u32 _ref, u32 _mask, u32 _writemask, u32 _fail, u32 _pass,
							  u32 _zfail)
{
	OPTICK_EVENT("CBackend::set_Stencil");

	// Simple filter
	if (stencil_enable != _enable)
	{
		stencil_enable = _enable;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, _enable));
	}
	if (!stencil_enable)
		return;
	if (stencil_func != _func)
	{
		stencil_func = _func;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, _func));
	}
	if (stencil_ref != _ref)
	{
		stencil_ref = _ref;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, _ref));
	}
	if (stencil_mask != _mask)
	{
		stencil_mask = _mask;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, _mask));
	}
	if (stencil_writemask != _writemask)
	{
		stencil_writemask = _writemask;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, _writemask));
	}
	if (stencil_fail != _fail)
	{
		stencil_fail = _fail;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, _fail));
	}
	if (stencil_pass != _pass)
	{
		stencil_pass = _pass;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, _pass));
	}
	if (stencil_zfail != _zfail)
	{
		stencil_zfail = _zfail;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, _zfail));
	}
}
IC void CBackend::set_ColorWriteEnable(u32 _mask)
{
	OPTICK_EVENT("CBackend::set_ColorWriteEnable");

	if (colorwrite_mask != _mask)
	{
		colorwrite_mask = _mask;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, _mask));
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, _mask));
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, _mask));
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, _mask));
	}
}
IC void CBackend::set_ZWriteEnable(bool write_state)
{
	OPTICK_EVENT("CBackend::set_ZWriteEnable");

	if (zwrite != write_state)
	{
		zwrite = write_state;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, write_state));
	}
}
ICF void CBackend::set_CullMode(u32 _mode)
{
	OPTICK_EVENT("CBackend::set_CullMode");

	if (cull_mode != _mode)
	{
		cull_mode = _mode;
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_CULLMODE, _mode));
	}
}

#endif
