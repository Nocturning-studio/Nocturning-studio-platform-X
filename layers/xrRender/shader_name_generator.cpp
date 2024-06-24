////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2023
// Author: Deathman
// Nocturning studio for NS Project X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "shader_name_generator.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern u32 ps_r2_material_quality;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fix_texture_name(LPSTR fn);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void generate_shader_name(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName,
						  LPCSTR PixelShaderName, BOOL bUseAlpha)
{
	// Output shader names
	string_path NewPixelShaderName;
	string_path NewVertexShaderName;

	// Base part of material
	string_path AlbedoTexture;
	string_path BumpTexture;
	string_path BumpCorrectionTexture;

	// Detail part of material
	string_path DetailAlbedoTexture;
	string_path DetailBumpTexture;
	string_path DetailBumpCorrectionTexture;

	// Other textures
	string_path HemisphereLightMapTexture;
	string_path LightMapTexture;
	string_path BakedAOTexture;

	string_path Dummy = {0};

	strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Add extension to texture  and chek for null
	fix_texture_name(AlbedoTexture);

	// Check bump existing
	ref_texture refAlbedoTexture;
	refAlbedoTexture.create(AlbedoTexture);
	bool bUseBump = refAlbedoTexture.bump_exist();

	// Get bump map texture
	if (bUseBump)
	{
		strcpy_s(BumpTexture, sizeof(BumpTexture), refAlbedoTexture.bump_get().c_str());
	}
	else
	{
		//If bump not connected - try find unconnected texture with similar name
		strcpy_s(BumpTexture, sizeof(BumpTexture), AlbedoTexture);
		strconcat(sizeof(BumpTexture), BumpTexture, BumpTexture, "_bump");

		if (FS.exist(Dummy, "$game_textures$", BumpTexture, ".dds"))
			bUseBump = true;
	}

	// Get bump decompression map
	strcpy_s(BumpCorrectionTexture, sizeof(BumpCorrectionTexture), BumpTexture);
	strconcat(sizeof(BumpCorrectionTexture), BumpCorrectionTexture, BumpCorrectionTexture, "#");

	// Check detail existing and allowing
	bool bDetailTextureExist = C.detail_texture;
	bool bDetailTextureAllowed = C.bDetail;
	bool bUseDetail = bDetailTextureExist && bDetailTextureAllowed;
	bool bUseDetailBump = false;

	// Get  for base texture for material
	strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Get detail texture
	if (bUseDetail)
	{
		strcpy_s(DetailAlbedoTexture, sizeof(DetailAlbedoTexture), C.detail_texture);

		// Get bump for detail texture
		strcpy_s(DetailBumpTexture, sizeof(DetailBumpTexture), DetailAlbedoTexture);

		// Checking for existance
		if (FS.exist(Dummy, "$game_textures$", DetailBumpTexture, ".dds"))
		{
			bUseDetailBump = true;

			strconcat(sizeof(DetailBumpTexture), DetailBumpTexture, DetailBumpTexture, "_bump");

			// Get bump decompression map for detail texture
			strcpy_s(DetailBumpCorrectionTexture, sizeof(DetailBumpCorrectionTexture), DetailBumpTexture);
			strconcat(sizeof(DetailBumpCorrectionTexture), DetailBumpCorrectionTexture, DetailBumpCorrectionTexture,"#");
		}
	}

	// Check lightmap existing
	bool bUseLightMap = false;
	if (C.L_textures.size() >= 3)
	{
		pcstr HemisphereLightMapTextureName = C.L_textures[2].c_str();
		pcstr LightMapTextureName = C.L_textures[1].c_str();

		if ((HemisphereLightMapTextureName[0] == 'l' && HemisphereLightMapTextureName[1] == 'm' &&
			 HemisphereLightMapTextureName[2] == 'a' && HemisphereLightMapTextureName[3] == 'p') &&
			(LightMapTextureName[0] == 'l' && LightMapTextureName[1] == 'm' &&
			 LightMapTextureName[2] == 'a' && LightMapTextureName[3] == 'p'))
		{
			bUseLightMap = true;

			// Get LightMap texture
			strcpy_s(HemisphereLightMapTexture, sizeof(HemisphereLightMapTexture), *C.L_textures[2]);
			strcpy_s(LightMapTexture, sizeof(LightMapTexture), *C.L_textures[1]);
		}
		else
		{
			bUseLightMap = false;
		}
	}

	// Get BakedAO texture
	bool bUseBakedAO = false;
	strcpy_s(BakedAOTexture, sizeof(BakedAOTexture), AlbedoTexture);
	strconcat(sizeof(BakedAOTexture), BakedAOTexture, BakedAOTexture, "_ao");
	if (FS.exist(Dummy, "$game_textures$", BakedAOTexture, ".dds"))
		bUseBakedAO = true;

	// Starting generate shader name
	strconcat(sizeof(NewPixelShaderName), NewPixelShaderName, "gbuffer_stage_", PixelShaderName);
	strconcat(sizeof(NewVertexShaderName), NewVertexShaderName, "gbuffer_stage_", VertexShaderName);

	// Create lightmapped shader if need
	C.sh_macro(bUseLightMap, "USE_LIGHTMAP", "1");

	// Create shader with alpha testing if need
	C.sh_macro(bUseAlpha, "USE_ALPHA_TEST", "1");

	C.sh_macro(bUseBump, "USE_BUMP", "1");

	// Create shader with normal mapping or displacement if need
	int BumpType = 0;
	if (bIsHightQualityGeometry)
	{
		if (ps_r2_material_quality == 1 || !bUseBump)
			BumpType = 1; // normal
		else if (ps_r2_material_quality == 2 || !C.bSteepParallax)
			BumpType = 2; // parallax
		else if ((ps_r2_material_quality == 3 || ps_r2_material_quality == 4) && C.bSteepParallax)
			BumpType = 3; // steep parallax
		else 
			BumpType = 2; // parallax
	}

	C.sh_macro(BumpType == 1, "USE_NORMAL_MAPPING", "1");
	C.sh_macro(BumpType == 2, "USE_PARALLAX_MAPPING", "1");
	C.sh_macro(BumpType == 3, "USE_STEEP_PARALLAX_MAPPING", "1");

	// Create shader with deatil texture if need
	C.sh_macro(bUseDetail, "USE_TDETAIL", "1");

	C.sh_macro(bUseDetailBump, "USE_DETAIL_BUMP", "1");

	C.sh_macro(bUseBakedAO, "USE_BAKED_AO", "1");

	// Create shader pass
	C.r_Pass(NewVertexShaderName, NewPixelShaderName, FALSE);

	C.r_Sampler("s_base", AlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_POINT, D3DTEXF_ANISOTROPIC);
	
	if (bUseBakedAO)
	{
		C.r_Sampler("s_baked_ao", BakedAOTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
	}

	if (bUseBump)
	{
		C.r_Sampler("s_bumpX", BumpCorrectionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		C.r_Sampler("s_bump", BumpTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
	}

	if (bUseDetail)
	{
		C.r_Sampler("s_detail", DetailAlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		if (bUseDetailBump)
		{
			C.r_Sampler("s_detailBump", DetailBumpTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

			C.r_Sampler("s_detailBumpX", DetailBumpCorrectionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		}
	}

	if (bUseLightMap)
	{
		C.r_Sampler("s_hemi", HemisphereLightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_NONE, D3DTEXF_LINEAR);
		C.r_Sampler("s_lmap", LightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_GAUSSIANQUAD, D3DTEXF_NONE, D3DTEXF_LINEAR);
	}

	jitter(C);

	C.r_End();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
