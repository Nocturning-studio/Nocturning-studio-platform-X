#include "stdafx.h"
#include "uber_deffer.h"
void fix_texture_name(LPSTR fn);

#include "dxRenderDeviceRender.h"

void uber(CBlender_Compile& C, bool hq, LPCSTR _vspec, LPCSTR _pspec, BOOL _aref, LPCSTR _detail_replace, bool DO_NOT_FINISH)
{
	string256		fname, fnameA, fnameB;
	xr_strcpy(fname, *C.L_textures[0]);	//. andy if (strext(fname)) *strext(fname)=0;
	fix_texture_name(fname);
	ref_texture		_t;		_t.create(fname);

	bool bump = opt(R__USE_BUMP) ? _t.bump_exist() : false;

	if (C.iElement == SE_PLANAR)
		bump = false;

	// detect lmap
	bool			lmap = true;
	if (C.L_textures.size() < 3)	lmap = false;
	else
	{
		pcstr		tex = C.L_textures[2].c_str();
		if (tex[0] == 'l' && tex[1] == 'm' && tex[2] == 'a' && tex[3] == 'p')	lmap = true;
		else															lmap = false;
	}

	string256 ps, vs, dt;
	strconcat(sizeof(vs), vs, "deffer_", _vspec, lmap ? "_lmh" : "");
	strconcat(sizeof(ps), ps, "deffer_", _pspec, lmap ? "_lmh" : "");
	xr_strcpy(dt, sizeof(dt), _detail_replace ? _detail_replace : (C.detail_texture ? C.detail_texture : ""));

	// detect detail bump
	string256		texDetailBump = { '\0' };
	string256		texDetailBumpX = { '\0' };

	bool bHasDetailBump = false;

	if (C.bDetail_Bump && bump)
	{
		LPCSTR detail_bump_texture = DEV->m_textures_description.GetBumpName(dt).c_str();
		//	Detect and use detail bump
		if (detail_bump_texture)
		{
			bHasDetailBump = true;
			xr_strcpy(texDetailBump, sizeof(texDetailBump), detail_bump_texture);
			xr_strcpy(texDetailBumpX, sizeof(texDetailBumpX), detail_bump_texture);
			xr_strcat(texDetailBumpX, "#");
		}
	}

	if (_aref)
	{
		xr_strcat(ps, "_aref");
	}

	if (!bump)
	{
		fnameA[0] = fnameB[0] = 0;
		xr_strcat(vs, "_flat");
		xr_strcat(ps, "_flat");
		if (hq && (C.bDetail_Diffuse || C.bDetail_Bump))
		{
			xr_strcat(vs, "_d");
			xr_strcat(ps, "_d");
		}
	}
	else
	{
		xr_strcpy(fnameA, _t.bump_get().c_str());
		strconcat(sizeof(fnameB), fnameB, fnameA, "#");
		xr_strcat(vs, "_bump");
		if (hq && C.bUseSteepParallax)
		{
			xr_strcat(ps, "_steep");
		}
		else
		{
			xr_strcat(ps, "_bump");
		}
		if (hq && (C.bDetail_Diffuse || C.bDetail_Bump))
		{
			xr_strcat(vs, "_d");
			if (bHasDetailBump)
				xr_strcat(ps, "_db");	//	bump & detail & hq
			else
				xr_strcat(ps, "_d");
		}
	}

	// HQ
	if (bump && hq)
	{
		xr_strcat(vs, "-hq");
		xr_strcat(ps, "-hq");
	}

	// Uber-construct
	u32 tess_object = 0;

	if (hq && bump)
		tess_object = 1;

	// detect 4 detail
	if (!xr_strcmp(_vspec, "impl") && !xr_strcmp(_pspec, "impl"))
	{
		Msg("* terrain tess params:\n\t-tess: %d\n\t-hq: %d", C.TessMethod, hq);
		tess_object = 2;
	}

	if (tess_object && RImplementation.o.tessellation && C.TessMethod != 0)
	{
		char hs[256], ds[256];// = "DX11\\tess", ds[256] = "DX11\\tess";
		char params[256] = "(";

		if (C.TessMethod == CBlender_Compile::TESS_PN || C.TessMethod == CBlender_Compile::TESS_PN_HM)
		{
			RImplementation.addShaderOption("TESS_PN", "1");
			xr_strcat(params, "TESS_PN,");
		}

		if (C.TessMethod == CBlender_Compile::TESS_HM || C.TessMethod == CBlender_Compile::TESS_PN_HM)
		{
			RImplementation.addShaderOption("TESS_HM", "1");
			xr_strcat(params, "TESS_HM,");
		}

		if (lmap)
		{
			RImplementation.addShaderOption("USE_LM_HEMI", "1");
			xr_strcat(params, "USE_LM_HEMI,");
		}

		if (C.bDetail_Diffuse)
		{
			RImplementation.addShaderOption("USE_TDETAIL", "1");
			xr_strcat(params, "USE_TDETAIL,");
		}

		if (C.bDetail_Bump)
		{
			RImplementation.addShaderOption("USE_TDETAIL_BUMP", "1");
			xr_strcat(params, "USE_TDETAIL_BUMP,");
		}

		if (tess_object == 2)
		{
			Log("* Terrain tesselated!");
			RImplementation.addShaderOption("TESS_4_DETAIL", "1");
			xr_strcat(params, "TESS_4_DETAIL,");
		}

		xr_strcat(params, ")");

		if (tess_object == 1)
		{
			strconcat(sizeof(vs), vs, "deffer_", _vspec, "_bump", params);
			strconcat(sizeof(ps), ps, "deffer_", _pspec, _aref ? "_aref" : "", "_bump", params);

			VERIFY(strstr(vs, "bump") != 0);
			VERIFY(strstr(ps, "bump") != 0);
		}
		else if (tess_object == 2)
		{
			xr_strcat(ps, params);
			xr_strcat(vs, params);
		}

		strconcat(sizeof(hs), hs, "tess", params);
		strconcat(sizeof(ds), ds, "tess", params);

		if (C.iElement == SE_ZPREPASS) C.r_TessPass(vs, hs, ds, "null", "zprepass_h", FALSE);
		else C.r_TessPass(vs, hs, ds, "null", ps, FALSE);

		RImplementation.clearAllShaderOptions();

		u32 stage = C.r_dx10Sampler("smp_bump_ds");

		if (stage != -1)
		{
			C.i_dx10Address(stage, D3D_TEXTURE_ADDRESS_WRAP);
			C.i_dx10FilterAnizo(stage, TRUE);
		}

		if (opt(R__DBG_DRAW_WIREFRAME))
			C.R().SetRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

		C.r_dx10Texture("s_tbump", fnameA);
		C.r_dx10Texture("s_tbumpX", fnameB);	// should be before base bump

		if (tess_object == 2)
		{
			string256				mask;
			strconcat(sizeof(mask), mask, C.L_textures[0].c_str(), "_mask");

			C.r_dx10Texture("s_tmask", mask);

			extern string64 oR_Name_hack, oG_Name_hack, oB_Name_hack, oA_Name_hack;

			C.r_dx10Texture("s_tdx_r", strconcat(sizeof(mask), mask, oR_Name_hack, "_bump#"));
			C.r_dx10Texture("s_tdx_g", strconcat(sizeof(mask), mask, oG_Name_hack, "_bump#"));
			C.r_dx10Texture("s_tdx_b", strconcat(sizeof(mask), mask, oB_Name_hack, "_bump#"));
			C.r_dx10Texture("s_tdx_a", strconcat(sizeof(mask), mask, oA_Name_hack, "_bump#"));
		}

		if (bHasDetailBump)
			C.r_dx10Texture("s_tdetailBumpX", texDetailBumpX);
	}
	else
	{
		if (C.iElement == SE_ZPREPASS) C.r_Pass(vs, "zprepass_h", FALSE); 
		else C.r_Pass(vs, ps, FALSE);
	}


	if (::Render->is_sun_static())
	{
		VERIFY(C.L_textures[0].size());

		if (bump)
		{
			VERIFY2(xr_strlen(fnameB), C.L_textures[0].c_str());
			VERIFY2(xr_strlen(fnameA), C.L_textures[0].c_str());
		}

		if (bHasDetailBump)
		{
			VERIFY2(xr_strlen(texDetailBump), C.L_textures[0].c_str());
			VERIFY2(xr_strlen(texDetailBumpX), C.L_textures[0].c_str());
		}
	}

	C.r_Constant("object_id", &RImplementation.m_C_object_id);

	//C.r_Constant("m_VP_prev", &RImplementation.m_C_VP_prev);
	//C.r_Constant("m_P_prev", &RImplementation.m_C_P_prev);
	//C.r_Constant("m_v2w_prev", &RImplementation.m_C_v2w_prev);

	C.r_dx10Texture("s_base", C.L_textures[0]);
	C.r_dx10Texture("s_bumpX", fnameB);	// should be before base bump
	C.r_dx10Texture("s_bump", fnameA);
	C.r_dx10Texture("s_bumpD", dt);
	C.r_dx10Texture("s_detail", dt);

	C.r_dx10Texture("s_position_far", tex_rt_Accumulator);

	if (bHasDetailBump)
	{
		C.r_dx10Texture("s_detailBump", texDetailBump);
		C.r_dx10Texture("s_detailBumpX", texDetailBumpX);
	}

	if (lmap)
	{
		C.r_dx10Texture("s_hemi", C.L_textures[2]);
	}

	C.r_dx10Sampler("smp_base");

	if (!DO_NOT_FINISH)		C.r_End();
}

void uber_shadow(CBlender_Compile& C, LPCSTR _vspec)
{
	// Uber-parse
	string256		fname, fnameA, fnameB;
	xr_strcpy(fname, *C.L_textures[0]);	//. andy if (strext(fname)) *strext(fname)=0;
	fix_texture_name(fname);
	ref_texture		_t;		_t.create(fname);

	bool bump;

	if (::Render->is_sun_static())
	{
		BOOL need_bump = opt(R__USE_BUMP);
		bump = need_bump ? _t.bump_exist() : false;
	}
	else
	{
		bump = _t.bump_exist();
	}

	// detect lmap
	bool lmap = true;

	if (C.L_textures.size() < 3)
		lmap = false;
	else
	{
		pcstr		tex = C.L_textures[2].c_str();
		if (tex[0] == 'l' && tex[1] == 'm' && tex[2] == 'a' && tex[3] == 'p')	lmap = true;
		else															lmap = false;
	}


	string256		vs, dt;
	xr_strcpy(dt, sizeof(dt), C.detail_texture ? C.detail_texture : "");

	// detect detail bump
	string256		texDetailBump = { '\0' };
	string256		texDetailBumpX = { '\0' };
	bool			bHasDetailBump = false;

	if (C.bDetail_Bump && bump)
	{
		LPCSTR detail_bump_texture = DEV->m_textures_description.GetBumpName(dt).c_str();
		//	Detect and use detail bump
		if (detail_bump_texture)
		{
			bHasDetailBump = true;
			xr_strcpy(texDetailBump, sizeof(texDetailBump), detail_bump_texture);
			xr_strcpy(texDetailBumpX, sizeof(texDetailBumpX), detail_bump_texture);
			xr_strcat(texDetailBumpX, "#");
		}
	}

	if (!bump)
		fnameA[0] = fnameB[0] = 0;
	else
	{
		xr_strcpy(fnameA, _t.bump_get().c_str());
		strconcat(sizeof(fnameB), fnameB, fnameA, "#");
	}

	if (bump && RImplementation.o.tessellation && C.TessMethod != 0)
	{
		char hs[256], ds[256];
		char params[256] = "(";

		if (C.TessMethod == CBlender_Compile::TESS_PN || C.TessMethod == CBlender_Compile::TESS_PN_HM)
		{
			RImplementation.addShaderOption("TESS_PN", "1");
			xr_strcat(params, "TESS_PN,");
		}

		if (C.TessMethod == CBlender_Compile::TESS_HM || C.TessMethod == CBlender_Compile::TESS_PN_HM)
		{
			RImplementation.addShaderOption("TESS_HM", "1");
			xr_strcat(params, "TESS_HM,");
		}

		if (lmap)
		{
			RImplementation.addShaderOption("USE_LM_HEMI", "1");
			xr_strcat(params, "USE_LM_HEMI,");
		}

		if (C.bDetail_Diffuse)
		{
			RImplementation.addShaderOption("USE_TDETAIL", "1");
			xr_strcat(params, "USE_TDETAIL,");
		}

		if (C.bDetail_Bump)
		{
			RImplementation.addShaderOption("USE_TDETAIL_BUMP", "1");
			xr_strcat(params, "USE_TDETAIL_BUMP,");
		}

		xr_strcat(params, ")");

		strconcat(sizeof(vs), vs, "deffer_", _vspec, "_bump", params);
		strconcat(sizeof(hs), hs, "tess", params);
		strconcat(sizeof(ds), ds, "tess_shadow", params);

		if (C.iElement == SE_ZPREPASS)
			C.r_TessPass(vs, hs, ds, "null", "zprepass_h", FALSE, TRUE, TRUE, FALSE);
		else
			C.r_TessPass(vs, hs, ds, "null", "shadow_direct_base", FALSE, TRUE, TRUE, FALSE);

		RImplementation.clearAllShaderOptions();
		C.r_dx10Texture("s_base", C.L_textures[0]);
		C.r_dx10Texture("s_bumpX", fnameB);	// should be before base bump
		C.r_dx10Texture("s_bump", fnameA);
		if (bHasDetailBump)
		{
			C.r_dx10Texture("s_detailBump", texDetailBump);
			C.r_dx10Texture("s_detailBumpX", texDetailBumpX);
		}
		u32 stage = C.r_dx10Sampler("smp_bump_ds");
		if (stage != -1)
		{
			C.i_dx10Address(stage, D3D_TEXTURE_ADDRESS_WRAP);
			C.i_dx10FilterAnizo(stage, TRUE);
		}
		if (opt(R__DBG_DRAW_WIREFRAME))
			C.R().SetRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	}
	else
		if (C.iElement == SE_ZPREPASS)
			C.r_Pass("shadow_direct_base_multisampled", "zprepass", FALSE, TRUE, TRUE, FALSE);
		else
			C.r_Pass("shadow_direct_base", "shadow_direct_base", FALSE, TRUE, TRUE, FALSE);

	C.r_ColorWriteEnable(true, true, false, false);
}

void uber_planar(CBlender_Compile& C, LPCSTR _vspec, BOOL _aref)
{
	string256 vs, ps;
	xr_sprintf(vs, "%s%s", "planar_", _vspec);
	xr_sprintf(ps, "%s%s", "planar_common", _aref ? "_aref" : "");

	C.r_Pass(vs, ps, FALSE, TRUE, TRUE, TRUE, D3D_BLEND_INV_SRC_ALPHA, D3D_BLEND_SRC_ALPHA);

	C.r_Constant("planar_env",			&RImplementation.m_C_planar_env);
	C.r_Constant("planar_amb",			&RImplementation.m_C_planar_amb);
	C.r_Constant("planar_bias",			&RImplementation.m_C_planar_bias);
	C.r_Constant("planar_shadow",		&RImplementation.m_C_planar_shadow);
	C.r_Constant("planar_vp_camera",	&RImplementation.m_C_planar_vp_camera);

	C.r_dx10Texture("s_base", C.L_textures[0]);
	C.r_dx10Texture("env_s0", tex_t_envmap_0);
	C.r_dx10Texture("env_s1", tex_t_envmap_1);
	C.r_dx10Texture("s_smap", tex_rt_Planar_shadow);
	C.r_dx10Texture("s_lmap", TEX_TORCH_ATT2);

	C.r_dx10Sampler("smp_nofilter");
	C.r_dx10Sampler("smp_base");
	C.r_dx10Sampler("smp_smap");

	C.r_ColorWriteEnable(true, true, true, false);

	C.r_End();
}

void uber_rsm(CBlender_Compile& C, LPCSTR _vspec, BOOL _aref)
{
	string256 vs, ps;
	xr_sprintf(vs, "%s%s", "rsm_direct_", _vspec);
	xr_sprintf(ps, "%s%s", "rsm_direct_common", _aref ? "_aref" : "");

	C.r_Pass(vs, ps, FALSE, TRUE, TRUE, FALSE);
	C.r_CullMode(D3D_CULL_BACK);

	C.r_dx10Texture("s_base", C.L_textures[0]);
	C.r_dx10Sampler("smp_base");

	C.r_ColorWriteEnable(true, true, true, true);

	C.r_End();
}

void uber_zprepass(CBlender_Compile& C, LPCSTR _vspec, BOOL _aref, BOOL _tess)
{
	string256 vs, ps;

	BOOL need_aref = _aref;

	xr_sprintf(vs, "%s%s%s", "shadow_direct_", _vspec, need_aref ? "_aref" : "");
	xr_sprintf(ps, "%s%s", "zprepass", need_aref ? "_aref" : "");

	if (_tess)
		uber_shadow(C, "base"); // auto multisampled & tesselated, or shadow_direct_base_multisampled
	else
		C.r_Pass(vs, ps, FALSE, TRUE, TRUE, FALSE);

	C.r_dx10Texture("s_base", C.L_textures[0]);
	C.r_dx10Sampler("smp_base");
	C.r_dx10Sampler("smp_linear");

	C.r_ColorWriteEnable(true, false, false, false);

	C.r_End();
}
