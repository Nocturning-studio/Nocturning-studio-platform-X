#include "stdafx.h"
#include "uistaticitem.h"
#include "hudmanager.h"

ref_geom hGeom_fan = NULL;

void CreateUIGeom()
{
	hGeom_fan.create(FVF::F_TL, RenderBackend.Vertex.Buffer(), 0);
}

void DestroyUIGeom()
{
	hGeom_fan = NULL;
}

ref_geom GetUIGeom()
{
	return hGeom_fan;
}

CUIStaticItem::CUIStaticItem()
{
	dwColor = 0xffffffff;
	iTileX = 1;
	iTileY = 1;
	iRemX = 0.0f;
	iRemY = 0.0f;
	alpha_ref = -1;
	hShader = NULL;
#ifdef DEBUG
	dbg_tex_name = NULL;
#endif
}

CUIStaticItem::~CUIStaticItem()
{
}

void CUIStaticItem::CreateShader(LPCSTR tex, LPCSTR sh)
{
	hShader.create(sh, tex);

#ifdef DEBUG
	dbg_tex_name = tex;
#endif
	uFlags &= !flValidRect;
}

void CUIStaticItem::SetShader(const ref_shader& sh)
{
	hShader = sh;
}

void CUIStaticItem::Init(LPCSTR tex, LPCSTR sh, float left, float top, u32 align)
{
	uFlags &= !flValidRect;

	CreateShader(tex, sh);
	SetPos(left, top);
	SetAlign(align);
}

void CUIStaticItem::Render()
{
	OPTICK_EVENT("CUIStaticItem::Render");

	VERIFY(g_bRendering);
	// ���������� ����������� ����� ������� CustomItem::Render() !!!
	VERIFY(hShader);
	RenderBackend.set_Shader(hShader);
	if (alpha_ref != -1)
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAREF, alpha_ref));
	// convert&set pos
	Fvector2 bp;
	UI()->ClientToScreenScaled(bp, float(iPos.x), float(iPos.y));
	bp.x = (float)iFloor(bp.x);
	bp.y = (float)iFloor(bp.y);

	// actual rendering
	u32 vOffset;
	Fvector2 pos;
	Fvector2 f_len;
	UI()->ClientToScreenScaled(f_len, iVisRect.x2, iVisRect.y2);

	int tile_x = fis_zero(iRemX) ? iTileX : iTileX + 1;
	int tile_y = fis_zero(iRemY) ? iTileY : iTileY + 1;
	int x, y;
	if (!(tile_x && tile_y))
		return;
	// render
	FVF::TL* start_pv = (FVF::TL*)RenderBackend.Vertex.Lock(8 * tile_x * tile_y, hGeom_fan.stride(), vOffset);
	FVF::TL* pv = start_pv;
	for (x = 0; x < tile_x; ++x)
	{
		for (y = 0; y < tile_y; ++y)
		{
			pos.set(bp.x + f_len.x * x, bp.y + f_len.y * y);
			inherited::Render(pv, pos, dwColor);
		}
	}
	std::ptrdiff_t p_cnt = (pv - start_pv) / 3;
	VERIFY((pv - start_pv) <= 8 * tile_x * tile_y);
	RenderBackend.Vertex.Unlock(u32(pv - start_pv), hGeom_fan.stride());
	// set scissor
	Frect clip_rect = {iPos.x, iPos.y, iPos.x + iVisRect.x2 * iTileX + iRemX, iPos.y + iVisRect.y2 * iTileY + iRemY};
	UI()->PushScissor(clip_rect);
	// set geom
	RenderBackend.set_Geometry(hGeom_fan);
	if (p_cnt != 0)
		RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, u32(p_cnt));
	if (alpha_ref != -1)
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAREF, 0));
	UI()->PopScissor();
}

void CUIStaticItem::Render(float angle)
{
	OPTICK_EVENT("CUIStaticItem::Render");

	VERIFY(g_bRendering);
	// ���������� ����������� ����� ������� CustomItem::Render() !!!
	VERIFY(hShader);
	RenderBackend.set_Shader(hShader);
	if (alpha_ref != -1)
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAREF, alpha_ref));
	// convert&set pos
	Fvector2 bp_ns;
	bp_ns.set(iPos);

	// actual rendering
	u32 vOffset;
	FVF::TL* start_pv = (FVF::TL*)RenderBackend.Vertex.Lock(32, hGeom_fan.stride(), vOffset);
	FVF::TL* pv = start_pv;
	inherited::Render(pv, bp_ns, dwColor, angle);
	// unlock VB and Render it as triangle LIST
	std::ptrdiff_t p_cnt = pv - start_pv;
	RenderBackend.Vertex.Unlock(u32(p_cnt), hGeom_fan.stride());
	RenderBackend.set_Geometry(hGeom_fan);
	if (p_cnt > 2)
		RenderBackend.Render(D3DPT_TRIANGLEFAN, vOffset, u32(p_cnt - 2));
	if (alpha_ref != -1)
		CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAREF, 0));
}
