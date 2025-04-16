////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Created: 14.10.2023
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "shader_configurator.h"
#include "../xrEngine/ResourceManager.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern u32 ps_r_material_quality;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ConcatAndFindTexture(string_path& ResultPath, string_path AlbedoPath, LPCSTR TexturePrefix)
{
	string_path DummyPath = {0};

	bool SearchResult = false;

	strcpy_s(ResultPath, sizeof(ResultPath), AlbedoPath);
	strconcat(sizeof(ResultPath), ResultPath, ResultPath, TexturePrefix);

	if (FS.exist(DummyPath, "$game_textures$", ResultPath, ".dds"))
		SearchResult = true;

	return SearchResult;
}

float GetFloatValueIfExist(LPCSTR section_name, LPCSTR line_name, float default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_float(section_name, line_name);
	else
		return default_value;
}

Fvector3 GetRGBColorValueIfExist(LPCSTR section_name, LPCSTR line_name, Fvector3 default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_fvector3(section_name, line_name);
	else
		return default_value;
}

Fvector4 GetRGBAColorValueIfExist(LPCSTR section_name, LPCSTR line_name, Fvector4 default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_fvector4(section_name, line_name);
	else
		return default_value;
}

LPCSTR GetStringValueIfExist(LPCSTR section_name, LPCSTR line_name, LPCSTR default_value, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_string(section_name, line_name);
	else
		return default_value;
}

bool GetBoolValueIfExist(LPCSTR section_name, LPCSTR line_name, bool default_state, CInifile* config)
{
	if (config->line_exist(section_name, line_name))
		return config->r_bool(section_name, line_name);
	else
		return default_state;
}

bool StringsIsSimilar(LPCSTR x, LPCSTR y)
{
	bool result = false;

	if (0 == xr_strcmp(x, y))
		result = true;

	return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void configure_shader(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName, LPCSTR PixelShaderName, BOOL bUseAlpha)
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

	string_path Dummy = {0};

	strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Add extension to texture  and chek for null
	Device.Resources->fix_texture_name(AlbedoTexture);

	bool AlbedoOnlyMode = ps_r_shading_flags.test(RFLAG_FLAT_SHADING);

	// Check bump existing
	ref_texture refAlbedoTexture;
	refAlbedoTexture.create(AlbedoTexture);
	bool bUseBump = refAlbedoTexture.bump_exist();

	// Check detail existing and allowing
	bool bDetailTextureExist = C.detail_texture;
	bool bDetailTextureAllowed = C.bDetail;
	bool bUseDetail = bDetailTextureExist && bDetailTextureAllowed;
	bool bUseDetailBump = false;

	bool bUseConfigurator = false;
	CInifile* MaterialConfiguration;
	string_path MaterialConfiguratorSearchPath;
	string_path MaterialConfiguratorRealPath;
	strcpy_s(MaterialConfiguratorSearchPath, sizeof(MaterialConfiguratorSearchPath), AlbedoTexture);
	strconcat(sizeof(MaterialConfiguratorSearchPath), MaterialConfiguratorSearchPath, MaterialConfiguratorSearchPath, "_material_configuration.ltx");
	FS.update_path(MaterialConfiguratorRealPath, "$game_textures$", MaterialConfiguratorSearchPath);

	if (FS.exist(MaterialConfiguratorRealPath))
	{
		MaterialConfiguration = CInifile::Create(MaterialConfiguratorRealPath);
		Msg("Material configuration file finded: %s", MaterialConfiguratorRealPath);
		bUseConfigurator = true;
	}

	// Color space params
	bool bIsSrgbAlbedo = true;

	// Alpha test params
	bool bNeedHashedAlphaTest = true;
	bool bUseAlphaTest = bUseAlpha;

	// Normal map params
	bool bIsOpenGLNormal = false;

	// Wind params
	bool bUseWind = false;
	bool bUseXAxisAsWeight = false;
	bool bUseYAxisAsWeight = false;
	bool bUseBothAxisAsWeight = false;
	bool bInvertWeightAxis = false;
	int WindTypeNum = 1;

	if (bUseConfigurator)
	{
		LPCSTR AlphaTestType = GetStringValueIfExist("material_configuration", "alpha_test_type", "alpha_hashed", MaterialConfiguration);

		if (StringsIsSimilar(AlphaTestType, "alpha_clip"))
			bNeedHashedAlphaTest = false;

		if (StringsIsSimilar(AlphaTestType, "alpha_hashed"))
			bNeedHashedAlphaTest = true;

		if (StringsIsSimilar(AlphaTestType, "none"))
		{
			bNeedHashedAlphaTest = false;
			bUseAlphaTest = false;
		}

		if (bNeedHashedAlphaTest)
			Msg("Hashed alpha test enabled");

		LPCSTR AlbedoColorSpace = GetStringValueIfExist("albedo_configuration", "color_space", "srgb", MaterialConfiguration);

		if (StringsIsSimilar(AlbedoColorSpace, "srgb"))
			bIsSrgbAlbedo = true;

		if (StringsIsSimilar(AlbedoColorSpace, "linear"))
			bIsSrgbAlbedo = false;

		// Check for enabled/disabled bump using
		bUseBump = GetBoolValueIfExist("material_configuration", "use_bump", bUseBump, MaterialConfiguration);

		// Check for details using
		bUseDetail = GetBoolValueIfExist("material_configuration", "use_detail_map", bUseDetail, MaterialConfiguration);

		bUseAlphaTest = GetBoolValueIfExist("material_configuration", "use_alpha_test", bUseAlpha, MaterialConfiguration);

		bIsOpenGLNormal = GetBoolValueIfExist("normal_configuration", "is_opengl_type", bIsOpenGLNormal, MaterialConfiguration);

		// Wind configuration
		bUseWind = GetBoolValueIfExist("wind_configuration", "use_wind", bUseWind, MaterialConfiguration);
		LPCSTR WindType = GetStringValueIfExist("wind_configuration", "wind_type", "trunk", MaterialConfiguration);

		if (StringsIsSimilar(WindType, "legacy"))
			WindTypeNum = 0;
		else if (StringsIsSimilar(WindType, "trunk"))
			WindTypeNum = 1;
		else if (StringsIsSimilar(WindType, "branchcard"))
			WindTypeNum = 2;
		else if (StringsIsSimilar(WindType, "leafcard"))
			WindTypeNum = 3;

		bUseXAxisAsWeight = GetBoolValueIfExist("wind_configuration", "use_x_axis_as_weight", bUseXAxisAsWeight, MaterialConfiguration);
		bUseYAxisAsWeight = GetBoolValueIfExist("wind_configuration", "use_y_axis_as_weight", bUseYAxisAsWeight, MaterialConfiguration);
		bUseBothAxisAsWeight = GetBoolValueIfExist("wind_configuration", "use_both_axis_as_weight", bUseBothAxisAsWeight, MaterialConfiguration);
		bInvertWeightAxis = GetBoolValueIfExist("wind_configuration", "invert_weight_axis", bInvertWeightAxis, MaterialConfiguration);
	}

	C.sh_macro(bNeedHashedAlphaTest, "USE_HASHED_ALPHA_TEST", "1");
	C.sh_macro(bIsSrgbAlbedo, "USE_SRGB_COLOR_CONVERTING", "1");
	C.sh_macro(bIsOpenGLNormal, "IS_OPENGL_NORMAL", "1");

	C.sh_macro(bUseWind, "USE_WIND", "1");
	C.sh_macro(WindTypeNum == 0, "USE_LEGACY_WIND", "1");
	C.sh_macro(WindTypeNum == 1, "USE_TRUNK_WIND", "1");
	C.sh_macro(WindTypeNum == 2, "USE_BRANCHCARD_WIND", "1");
	C.sh_macro(WindTypeNum == 3, "USE_LEAFCARD_WIND", "1");
	C.sh_macro(bUseXAxisAsWeight, "USE_X_AXIS_AS_WEIGHT", "1");
	C.sh_macro(bUseYAxisAsWeight, "USE_Y_AXIS_AS_WEIGHT", "1");
	C.sh_macro(bUseBothAxisAsWeight, "USE_BOTH_AXIS_AS_WEIGHT", "1");
	C.sh_macro(bInvertWeightAxis, "INVERT_WEIGHT_AXIS", "1");

	if (!AlbedoOnlyMode)
	{
		// Get bump map texture
		if (bUseBump)
		{
			strcpy_s(BumpTexture, sizeof(BumpTexture), refAlbedoTexture.bump_get().c_str());
		}
		else
		{
			// If bump not connected - try find unconnected texture with similar name
			if (FS.exist(Dummy, "$game_textures$", BumpTexture, ".dds"))
				bUseBump = ConcatAndFindTexture(BumpTexture, AlbedoTexture, "_bump");
		}

		// Get bump decompression map
		if (bUseBump)
		{
			strcpy_s(BumpCorrectionTexture, sizeof(BumpCorrectionTexture), BumpTexture);
			strconcat(sizeof(BumpCorrectionTexture), BumpCorrectionTexture, BumpCorrectionTexture, "#");
		}
	}

	// Get  for base texture for material
	strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Get detail texture
	if (bUseDetail)
	{
		// Check do we need to use custom shader
		bool bIsSrgbDetail = true;
		if (bUseConfigurator)
		{
			LPCSTR AlbedoColorSpace = GetStringValueIfExist("detail_albedo_configuration", "color_space", "srgb", MaterialConfiguration);

			if (StringsIsSimilar(AlbedoColorSpace, "srgb"))
				bIsSrgbDetail = true;

			if (StringsIsSimilar(AlbedoColorSpace, "linear"))
				bIsSrgbDetail = false;
		}

		C.sh_macro(bIsSrgbDetail, "USE_DETAIL_SRGB_COLOR_CONVERTING", "1");

		strcpy_s(DetailAlbedoTexture, sizeof(DetailAlbedoTexture), C.detail_texture);

		if (!AlbedoOnlyMode)
		{
			// Get bump for detail texture
			bUseDetailBump = ConcatAndFindTexture(DetailBumpTexture, DetailAlbedoTexture, "_bump");

			if (bUseDetailBump)
			{
				// Get bump decompression map for detail texture
				strcpy_s(DetailBumpCorrectionTexture, sizeof(DetailBumpCorrectionTexture), DetailBumpTexture);
				strconcat(sizeof(DetailBumpCorrectionTexture), DetailBumpCorrectionTexture, DetailBumpCorrectionTexture, "#");
			}
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

	// Create lightmapped shader if need
	C.sh_macro(bUseLightMap, "USE_LIGHTMAP", "1");

	// Get opacity texture
	bool bUseOpacity = false;
	string_path OpacityTexture;
	bUseOpacity = ConcatAndFindTexture(OpacityTexture, AlbedoTexture, "_opacity");
	C.sh_macro(bUseOpacity, "USE_CUSTOM_OPACITY", "1");

	// Create shader with alpha testing if need
	C.sh_macro(bUseAlphaTest || bUseOpacity, "USE_ALPHA_TEST", "1");

	// Get BakedAO texture
	bool bUseBakedAO = false;
	string_path BakedAOTexture;
	bUseBakedAO = ConcatAndFindTexture(BakedAOTexture, AlbedoTexture, "_ao") && !AlbedoOnlyMode;
	C.sh_macro(bUseBakedAO, "USE_BAKED_AO", "1");

	// Get normal texture
	bool bUseCustomNormal = false;
	string_path CustomNormalTexture;
	bUseCustomNormal = ConcatAndFindTexture(CustomNormalTexture, AlbedoTexture, "_normal") && !AlbedoOnlyMode;
	C.sh_macro(bUseCustomNormal, "USE_CUSTOM_NORMAL", "1");

	// Get roughness texture
	bool bUseCustomRoughness = false;
	string_path CustomRoughnessTexture;
	bUseCustomRoughness = ConcatAndFindTexture(CustomRoughnessTexture, AlbedoTexture, "_roughness") && !AlbedoOnlyMode;
	C.sh_macro(bUseCustomRoughness, "USE_CUSTOM_ROUGHNESS", "1");

	// Get metallness texture
	bool bUseCustomMetallness = false;
	string_path CustomMetallnessTexture;
	bUseCustomMetallness = ConcatAndFindTexture(CustomMetallnessTexture, AlbedoTexture, "_metallness") && !AlbedoOnlyMode;
	C.sh_macro(bUseCustomMetallness, "USE_CUSTOM_METALLNESS", "1");

	// Get subsurface power texture
	bool bUseCustomSubsurface = false;
	string_path CustomSubsurfaceTexture;
	bUseCustomSubsurface = ConcatAndFindTexture(CustomSubsurfaceTexture, AlbedoTexture, "_subsurface") && !AlbedoOnlyMode;
	C.sh_macro(bUseCustomSubsurface, "USE_CUSTOM_SUBSURFACE", "1");

	// Get emissive power texture
	bool bUseCustomEmissive = false;
	string_path CustomEmissiveTexture;
	bUseCustomEmissive = ConcatAndFindTexture(CustomEmissiveTexture, AlbedoTexture, "_emissive");
	C.sh_macro(bUseCustomEmissive, "USE_CUSTOM_EMISSIVE", "1");

	// Get displacement texture
	bool bUseCustomDisplacement = false;
	string_path CustomDisplacementTexture;
	bUseCustomDisplacement = ConcatAndFindTexture(CustomDisplacementTexture, AlbedoTexture, "_displacement") && !AlbedoOnlyMode;
	C.sh_macro(bUseCustomDisplacement, "USE_CUSTOM_DISPLACEMENT", "1");

	// Get cavity texture
	bool bUseCustomCavity = false;
	string_path CustomCavityTexture;
	bUseCustomCavity = ConcatAndFindTexture(CustomCavityTexture, AlbedoTexture, "_cavity") && !AlbedoOnlyMode;
	C.sh_macro(bUseCustomCavity, "USE_CUSTOM_CAVITY", "1");

	// Get weight texture
	bool bUseCustomWeight = false;
	string_path CustomWeightTexture;
	bUseCustomWeight = ConcatAndFindTexture(CustomWeightTexture, AlbedoTexture, "_weight");
	C.sh_macro(bUseCustomWeight, "USE_WEIGHT_MAP", "1");

	C.sh_macro(bUseBump, "USE_BUMP", "1");

	// Create shader with normal mapping or displacement if need		
	int DisplacementType = 1;
	bool bUseDisplacement = true;
	if (!AlbedoOnlyMode)
	{
		// Configuration file have priority above original parameters
		if (bUseConfigurator && bIsHightQualityGeometry)
		{
			Msg("Configurator used");

			// Check do we realy need for displacement or not
			bUseDisplacement =
				GetBoolValueIfExist("material_configuration", "use_displacement", false, MaterialConfiguration);

			// Check what displacement type we need to set
			if (bUseDisplacement)
			{
				Msg("Displacement enabled");

				LPCSTR displacement_type = GetStringValueIfExist("material_configuration", "displacement_type", "parallax_occlusion_mapping", MaterialConfiguration);

				if (StringsIsSimilar(displacement_type, "none"))
					DisplacementType = 1;
				else if (StringsIsSimilar(displacement_type, "parallax_mapping"))
					DisplacementType = 2;
				else if (StringsIsSimilar(displacement_type, "parallax_occlusion_mapping"))
					DisplacementType = 3;

				Msg("displacement type %d, %s", DisplacementType, AlbedoTexture);
			}
		}
		else
		{
			if (ps_r_material_quality == 1 || !bUseBump)
				DisplacementType = 1; // normal
			else if (ps_r_material_quality == 2 || !C.bSteepParallax)
				DisplacementType = 2; // parallax
			else if ((ps_r_material_quality == 3) && C.bSteepParallax)
				DisplacementType = 3; // steep parallax
		}

		C.sh_macro(DisplacementType == 1, "USE_NORMAL_MAPPING", "1");
		C.sh_macro(DisplacementType == 2, "USE_PARALLAX_MAPPING", "1");
		C.sh_macro(DisplacementType == 3, "USE_PARALLAX_OCCLUSION_MAPPING", "1");
	}

	// Create shader with deatil texture if need
	C.sh_macro(bUseDetail, "USE_TDETAIL", "1");

	C.sh_macro(bUseDetailBump, "USE_DETAIL_BUMP", "1");

	// Check do we need to use custom shader
	if (bUseConfigurator)
	{
		LPCSTR CustomPixelShader = GetStringValueIfExist("material_configuration", "pixel_shader", "default", MaterialConfiguration);
		LPCSTR CustomVertexShader = GetStringValueIfExist("material_configuration", "vertex_shader", "default", MaterialConfiguration);

		if (!StringsIsSimilar(CustomPixelShader, "default"))
			PixelShaderName = CustomPixelShader;
		
		if (!StringsIsSimilar(CustomVertexShader, "default"))
			PixelShaderName = CustomPixelShader;
	}

	// Create shader pass
	strconcat(sizeof(NewPixelShaderName), NewPixelShaderName, "gbuffer_stage_", PixelShaderName);
	strconcat(sizeof(NewVertexShaderName), NewVertexShaderName, "gbuffer_stage_", VertexShaderName);
	C.r_Pass(NewVertexShaderName, NewPixelShaderName, FALSE);

	C.r_Sampler("s_base", AlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
	
	if (bUseOpacity)
		C.r_Sampler("s_custom_opacity", OpacityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseBakedAO)
		C.r_Sampler("s_baked_ao", BakedAOTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomNormal)
		C.r_Sampler("s_custom_normal", CustomNormalTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomRoughness)
		C.r_Sampler("s_custom_roughness", CustomRoughnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomMetallness)
		C.r_Sampler("s_custom_metallness", CustomMetallnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSubsurface)
		C.r_Sampler("s_custom_subsurface", CustomSubsurfaceTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomEmissive)
		C.r_Sampler("s_custom_emissive", CustomEmissiveTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomDisplacement)
		C.r_Sampler("s_custom_displacement", CustomDisplacementTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCavity)
		C.r_Sampler("s_custom_cavity", CustomCavityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomWeight)
		C.r_Sampler("s_custom_weight", CustomWeightTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseBump && !AlbedoOnlyMode)
	{
		C.r_Sampler("s_bumpX", BumpCorrectionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		C.r_Sampler("s_bump", BumpTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
	}

	if (bUseDetail)
	{
		C.r_Sampler("s_detail", DetailAlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);

		if (bUseDetailBump)
		{
			C.r_Sampler("s_detailBump", DetailBumpTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

			C.r_Sampler("s_detailBumpX", DetailBumpCorrectionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		}
	}

	if (bUseLightMap)
	{
		C.r_Sampler("s_hemi", HemisphereLightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_GAUSSIANQUAD);
		C.r_Sampler("s_lmap", LightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_GAUSSIANQUAD);
	}

	jitter(C);

	C.r_End();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void configure_shader_detail_object(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName, LPCSTR PixelShaderName, BOOL bUseAlpha)
{
	// Output shader names
	string_path NewPixelShaderName;
	string_path NewVertexShaderName;

	// Base part of material
	string_path AlbedoTexture;
	strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Add extension to texture  and chek for null
	Device.Resources->fix_texture_name(AlbedoTexture);

	// Get  for base texture for material
	strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	bool bUseConfigurator = false;
	CInifile* MaterialConfiguration;
	string_path MaterialConfiguratorSearchPath;
	string_path MaterialConfiguratorRealPath;
	strcpy_s(MaterialConfiguratorSearchPath, sizeof(MaterialConfiguratorSearchPath), AlbedoTexture);
	strconcat(sizeof(MaterialConfiguratorSearchPath), MaterialConfiguratorSearchPath, MaterialConfiguratorSearchPath,
			  "_material_configuration.ltx");
	FS.update_path(MaterialConfiguratorRealPath, "$game_textures$", MaterialConfiguratorSearchPath);

	if (FS.exist(MaterialConfiguratorRealPath))
	{
		MaterialConfiguration = CInifile::Create(MaterialConfiguratorRealPath);
		Msg("Material configuration file finded: %s", MaterialConfiguratorRealPath);
		bUseConfigurator = true;
	}

	// Check do we need to use custom shader
	bool bIsSrgbAlbedo = true;
	if (bUseConfigurator)
	{
		LPCSTR AlbedoColorSpace = GetStringValueIfExist("albedo_configuration", "color_space", "srgb", MaterialConfiguration);

		if (StringsIsSimilar(AlbedoColorSpace, "srgb"))
			bIsSrgbAlbedo = true;

		if (StringsIsSimilar(AlbedoColorSpace, "linear"))
			bIsSrgbAlbedo = false;
	}

	C.sh_macro(bIsSrgbAlbedo, "USE_SRGB_COLOR_CONVERTING", "1");

	C.sh_macro(true, "USE_NORMAL_MAPPING", "1");

	// Get opacity texture
	bool bUseOpacity = false;
	string_path OpacityTexture;
	bUseOpacity = ConcatAndFindTexture(OpacityTexture, AlbedoTexture, "_opacity");
	C.sh_macro(bUseOpacity, "USE_CUSTOM_OPACITY", "1");

	// Create shader with alpha testing if need
	C.sh_macro(bUseAlpha || bUseOpacity, "USE_ALPHA_TEST", "1");

	// Get BakedAO texture
	bool bUseBakedAO = false;
	string_path BakedAOTexture;
	bUseBakedAO = ConcatAndFindTexture(BakedAOTexture, AlbedoTexture, "_ao");
	C.sh_macro(bUseBakedAO, "USE_BAKED_AO", "1");

	// Get normal texture
	bool bUseCustomNormal = false;
	string_path CustomNormalTexture;
	bUseCustomNormal = ConcatAndFindTexture(CustomNormalTexture, AlbedoTexture, "_normal");
	C.sh_macro(bUseCustomNormal, "USE_CUSTOM_NORMAL", "1");

	// Get roughness texture
	bool bUseCustomRoughness = false;
	string_path CustomRoughnessTexture;
	bUseCustomRoughness = ConcatAndFindTexture(CustomRoughnessTexture, AlbedoTexture, "_roughness");
	C.sh_macro(bUseCustomRoughness, "USE_CUSTOM_ROUGHNESS", "1");

	// Get metallness texture
	bool bUseCustomMetallness = false;
	string_path CustomMetallnessTexture;
	bUseCustomMetallness = ConcatAndFindTexture(CustomMetallnessTexture, AlbedoTexture, "_metallness");
	C.sh_macro(bUseCustomMetallness, "USE_CUSTOM_METALLNESS", "1");

	// Get subsurface power texture
	bool bUseCustomSubsurface = false;
	string_path CustomSubsurfaceTexture;
	bUseCustomSubsurface = ConcatAndFindTexture(CustomSubsurfaceTexture, AlbedoTexture, "_subsurface");
	C.sh_macro(bUseCustomSubsurface, "USE_CUSTOM_SUBSURFACE", "1");

	// Get emissive power texture
	bool bUseCustomEmissive = false;
	string_path CustomEmissiveTexture;
	bUseCustomEmissive = ConcatAndFindTexture(CustomEmissiveTexture, AlbedoTexture, "_emissive");
	C.sh_macro(bUseCustomEmissive, "USE_CUSTOM_EMISSIVE", "1");

	// Get displacement texture
	bool bUseCustomDisplacement = false;
	string_path CustomDisplacementTexture;
	bUseCustomDisplacement = ConcatAndFindTexture(CustomDisplacementTexture, AlbedoTexture, "_displacement");
	C.sh_macro(bUseCustomDisplacement, "USE_CUSTOM_DISPLACEMENT", "1");

	// Get cavity texture
	bool bUseCustomCavity = false;
	string_path CustomCavityTexture;
	bUseCustomCavity = ConcatAndFindTexture(CustomCavityTexture, AlbedoTexture, "_cavity");
	C.sh_macro(bUseCustomCavity, "USE_CUSTOM_CAVITY", "1");

	// Get weight texture
	bool bUseCustomWeight = false;
	string_path CustomWeightTexture;
	bUseCustomWeight = ConcatAndFindTexture(CustomWeightTexture, AlbedoTexture, "_weight");
	C.sh_macro(bUseCustomWeight, "USE_WEIGHT_MAP", "1");

	// Create shader pass
	strconcat(sizeof(NewPixelShaderName), NewPixelShaderName, "gbuffer_stage_", PixelShaderName);
	strconcat(sizeof(NewVertexShaderName), NewVertexShaderName, "gbuffer_stage_", VertexShaderName);
	C.r_Pass(NewVertexShaderName, NewPixelShaderName, FALSE);

	C.r_Sampler("s_base", AlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
				D3DTEXF_ANISOTROPIC, true);

	if (bUseOpacity)
		C.r_Sampler("s_custom_opacity", OpacityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					D3DTEXF_ANISOTROPIC);

	if (bUseBakedAO)
		C.r_Sampler("s_baked_ao", BakedAOTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
					D3DTEXF_ANISOTROPIC);

	if (bUseCustomNormal)
		C.r_Sampler("s_custom_normal", CustomNormalTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomRoughness)
		C.r_Sampler("s_custom_roughness", CustomRoughnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomMetallness)
		C.r_Sampler("s_custom_metallness", CustomMetallnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSubsurface)
		C.r_Sampler("s_custom_subsurface", CustomSubsurfaceTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomEmissive)
		C.r_Sampler("s_custom_emissive", CustomEmissiveTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomDisplacement)
		C.r_Sampler("s_custom_displacement", CustomDisplacementTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCavity)
		C.r_Sampler("s_custom_cavity", CustomCavityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomWeight)
		C.r_Sampler("s_custom_weight", CustomWeightTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC,
					D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	jitter(C);

	C.r_End();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
