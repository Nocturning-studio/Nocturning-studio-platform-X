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

bool ConcatAndFindLevelTexture(string_path& ResultPath, string_path AlbedoPath, LPCSTR TexturePrefix)
{
	string_path DummyPath = {0};

	bool SearchResult = false;

	strcpy_s(ResultPath, sizeof(ResultPath), AlbedoPath);
	strconcat(sizeof(ResultPath), ResultPath, ResultPath, TexturePrefix);

	if (FS.exist(DummyPath, "$level$", ResultPath, ".dds"))
		SearchResult = true;

	return SearchResult;
}

bool FindTexture(string_path& ResultPath, LPCSTR TexturePath)
{
	string_path DummyPath = {0};

	bool SearchResult = false;

	strcpy_s(ResultPath, sizeof(ResultPath), TexturePath);

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

bool LineIsExist(LPCSTR section_name, LPCSTR line_name, CInifile* config)
{
	return config->line_exist(section_name, line_name);
}

bool StringsIsSimilar(LPCSTR x, LPCSTR y)
{
	bool result = false;

	if (0 == xr_strcmp(x, y))
		result = true;

	return result;
}

bool CheckAndApplyManualTexturePath(LPCSTR section_name, LPCSTR line_name, string_path& ResultPath, CInifile* config)
{
	bool bUseManualPath = LineIsExist(section_name, line_name, config);
	if (bUseManualPath)
	{
		LPCSTR Path = GetStringValueIfExist(section_name, line_name, "path_is_empty", config);
		bUseManualPath = FindTexture(ResultPath, Path);
	}
	return bUseManualPath;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void configure_shader(CBlender_Compile& C, bool bIsHightQualityGeometry, LPCSTR VertexShaderName, LPCSTR PixelShaderName, BOOL bUseAlpha, BOOL bUseDepthOnly)
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

	bool DisablePBR = !ps_r_shading_flags.test(RFLAG_ENABLE_PBR);

	bool UseAlbedoOnly = bUseDepthOnly;

	bool bUseLightMap = false;

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
	strconcat(sizeof(MaterialConfiguratorSearchPath), MaterialConfiguratorSearchPath, MaterialConfiguratorSearchPath, ".ltx");
	FS.update_path(MaterialConfiguratorRealPath, "$game_textures$", MaterialConfiguratorSearchPath);

	if (FS.exist(MaterialConfiguratorRealPath))
	{
		MaterialConfiguration = CInifile::Create(MaterialConfiguratorRealPath);
		Msg("Material configuration file finded: %s", MaterialConfiguratorRealPath);
		bUseConfigurator = true;
	}

	// Alpha test params
	bool bNeedHashedAlphaTest = true;
	bool bUseAlphaTest = bUseAlpha;
	if (bUseConfigurator)
	{
		LPCSTR AlphaTestType = GetStringValueIfExist("material_configuration", "alpha_test_type", "alpha_hashed", MaterialConfiguration);

		if (StringsIsSimilar(AlphaTestType, "alpha_clip"))
			bNeedHashedAlphaTest = false;

		else if (StringsIsSimilar(AlphaTestType, "none"))
			bUseAlphaTest = false;
	}

	// Get opacity texture
	bool bUseOpacity = false;
	string_path OpacityTexture;
	bUseOpacity = ConcatAndFindTexture(OpacityTexture, AlbedoTexture, "_opacity");
	C.r_Define(bUseOpacity, "USE_CUSTOM_OPACITY", "1");

	// Create shader with alpha testing if need
	C.r_Define(bUseAlphaTest || bUseOpacity, "USE_ALPHA_TEST", "1");
	C.r_Define(bNeedHashedAlphaTest, "USE_HASHED_ALPHA_TEST", "1");

	bool bUseBump = false;

	if (!UseAlbedoOnly)
	{
		// Color space params
		bool bIsSrgbAlbedo = true;
		if (bUseConfigurator)
		{
			LPCSTR AlbedoColorSpace =
				GetStringValueIfExist("albedo_configuration", "color_space", "srgb", MaterialConfiguration);

			if (StringsIsSimilar(AlbedoColorSpace, "linear"))
				bIsSrgbAlbedo = false;
		}

		C.r_Define(bIsSrgbAlbedo, "USE_SRGB_COLOR_CONVERTING", "1");

		// Normal map params
		// Check bump existing
		ref_texture refAlbedoTexture;
		refAlbedoTexture.create(AlbedoTexture);
		bUseBump = refAlbedoTexture.bump_exist();
		bool bIsOpenGLNormal = false;

		if (bUseConfigurator)
		{
			// Check for enabled/disabled bump using
			bUseBump = GetBoolValueIfExist("material_configuration", "use_bump", bUseBump, MaterialConfiguration);

			// Check for details using
			bUseDetail = GetBoolValueIfExist("material_configuration", "use_detail_map", bUseDetail, MaterialConfiguration);

			bUseAlphaTest = GetBoolValueIfExist("material_configuration", "use_alpha_test", bUseAlpha, MaterialConfiguration);

			bIsOpenGLNormal = GetBoolValueIfExist("normal_configuration", "is_opengl_type", bIsOpenGLNormal, MaterialConfiguration);
		}

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

			if (FS.exist(Dummy, "$game_textures$", BumpCorrectionTexture, ".dds") && (ps_r_material_quality > 1))
				C.r_Define("USE_BUMP_DECOMPRESSION", "1");
		}

		C.r_Define(bUseBump, "USE_BUMP", "1");
		C.r_Define(bIsOpenGLNormal, "IS_OPENGL_NORMAL", "1");
	}

	// Wind params
	bool bUseWind = false;
	bool bUseXAxisAsWeight = false;
	bool bUseYAxisAsWeight = false;
	bool bUseBothAxisAsWeight = false;
	bool bInvertWeightAxis = false;
	int WindTypeNum = 1;
	if (bUseConfigurator)
	{
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
	C.r_Define(bUseWind, "USE_WIND", "1");
	C.r_Define(WindTypeNum == 0, "USE_LEGACY_WIND", "1");
	C.r_Define(WindTypeNum == 1, "USE_TRUNK_WIND", "1");
	C.r_Define(WindTypeNum == 2, "USE_BRANCHCARD_WIND", "1");
	C.r_Define(WindTypeNum == 3, "USE_LEAFCARD_WIND", "1");
	C.r_Define(bUseXAxisAsWeight, "USE_X_AXIS_AS_WEIGHT", "1");
	C.r_Define(bUseYAxisAsWeight, "USE_Y_AXIS_AS_WEIGHT", "1");
	C.r_Define(bUseBothAxisAsWeight, "USE_BOTH_AXIS_AS_WEIGHT", "1");
	C.r_Define(bInvertWeightAxis, "INVERT_WEIGHT_AXIS", "1");

	strcpy_s(AlbedoTexture, sizeof(AlbedoTexture), *C.L_textures[0]);

	// Get weight texture
	bool bUseCustomWeight = false;
	string_path CustomWeightTexture;

	if (bUseConfigurator)
	{
		bUseCustomWeight = CheckAndApplyManualTexturePath("wind_configuration", "weight_path", CustomWeightTexture, MaterialConfiguration);
	}

	if (!bUseCustomWeight)
		bUseCustomWeight = ConcatAndFindTexture(CustomWeightTexture, AlbedoTexture, "_weight");

	C.r_Define(bUseCustomWeight, "USE_WEIGHT_MAP", "1");

	// Get AO-Roughness-Metallic texture
	bool bUseARMMap = false;
	string_path ARMTexture;

	// Get Empty-Roughness-Metallic texture
	bool bUseERMMap = false;
	string_path ERMTexture;

	// Get normal texture
	bool bUseCustomNormal = false;
	string_path CustomNormalTexture;

	// Get subsurface_power power texture
	bool bUseCustomSubsurfacePower = false;
	string_path CustomSubsurfacePowerTexture;

	// Get BakedAO texture
	bool bUseBakedAO = false;
	string_path BakedAOTexture;

	// Get emission power texture
	bool bUseCustomEmission = false;
	string_path CustomEmissionTexture;

	// Get roughness texture
	bool bUseCustomRoughness = false;
	string_path CustomRoughnessTexture;

	// Get gloss texture
	bool bUseCustomGloss = false;
	string_path CustomGlossTexture;

	// Get metallic texture
	bool bUseCustomMetallic = false;
	string_path CustomMetallicTexture;

	// Get cavity texture
	bool bUseCustomCavity = false;
	string_path CustomCavityTexture;

	// Get Specular Tint texture
	bool bUseCustomSpecularTint = false;
	string_path CustomSpecularTintTexture;

	// Get displacement texture
	bool bUseCustomDisplacement = false;
	string_path CustomDisplacementTexture;

	// Get sheen intensity texture
	bool bUseCustomSheenIntensity = false;
	string_path CustomSheenIntensityTexture;

	// Get sheen roughness texture
	bool bUseCustomSheenRoughness = false;
	string_path CustomSheenRoughnessTexture;

	// Get coat intensity texture
	bool bUseCustomCoatIntensity = false;
	string_path CustomCoatIntensityTexture;

	// Get coat roughness texture
	bool bUseCustomCoatRoughness = false;
	string_path CustomCoatRoughnessTexture;

	if (!UseAlbedoOnly)
	{
		// Get detail texture
		if (bUseDetail)
		{
			// Check do we need to use custom shader
			bool bIsSrgbDetail = true;
			if (bUseConfigurator)
			{
				LPCSTR AlbedoColorSpace =
					GetStringValueIfExist("detail_albedo_configuration", "color_space", "srgb", MaterialConfiguration);

				if (StringsIsSimilar(AlbedoColorSpace, "linear"))
					bIsSrgbDetail = false;
			}

			C.r_Define(bIsSrgbDetail, "USE_DETAIL_SRGB_COLOR_CONVERTING", "1");

			strcpy_s(DetailAlbedoTexture, sizeof(DetailAlbedoTexture), C.detail_texture);

			// Get bump for detail texture
			bUseDetailBump = ConcatAndFindTexture(DetailBumpTexture, DetailAlbedoTexture, "_bump");

			if (bUseDetailBump)
			{
				// Get bump decompression map for detail texture
				strcpy_s(DetailBumpCorrectionTexture, sizeof(DetailBumpCorrectionTexture), DetailBumpTexture);
				strconcat(sizeof(DetailBumpCorrectionTexture), DetailBumpCorrectionTexture, DetailBumpCorrectionTexture, "#");
			}
		}

		// Check lightmap existing
		if (C.L_textures.size() >= 3)
		{
			pcstr HemisphereLightMapTextureName = C.L_textures[2].c_str();
			pcstr LightMapTextureName = C.L_textures[1].c_str();

			if ((HemisphereLightMapTextureName[0] == 'l' && HemisphereLightMapTextureName[1] == 'm' &&
				 HemisphereLightMapTextureName[2] == 'a' && HemisphereLightMapTextureName[3] == 'p') &&
				(LightMapTextureName[0] == 'l' && LightMapTextureName[1] == 'm' && LightMapTextureName[2] == 'a' &&
				 LightMapTextureName[3] == 'p'))
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
		C.r_Define(bUseLightMap, "USE_LIGHTMAP", "1");

		// Get AO-Roughness-Metallic texture if needed
		bUseARMMap = ConcatAndFindTexture(ARMTexture, AlbedoTexture, "_arm") && !DisablePBR;
		C.r_Define(bUseARMMap, "USE_ARM_MAP", "1");

		// Get Empty-Roughness-Metallic texture if needed
		bUseERMMap = ConcatAndFindTexture(ERMTexture, AlbedoTexture, "_erm") && !DisablePBR;
		C.r_Define(bUseERMMap, "USE_ERM_MAP", "1");

		if (!DisablePBR)
		{
			if (!bUseARMMap && !bUseERMMap)
			{
				if (bUseConfigurator)
				{
					bUseBakedAO = CheckAndApplyManualTexturePath("material_configuration", "ao_path", BakedAOTexture, MaterialConfiguration);

					if (LineIsExist("material_configuration", "gloss_path", MaterialConfiguration))
						bUseCustomGloss = CheckAndApplyManualTexturePath("material_configuration", "gloss_path", CustomGlossTexture, MaterialConfiguration);
					else
						bUseCustomRoughness = CheckAndApplyManualTexturePath("material_configuration", "roughness_path", CustomRoughnessTexture, MaterialConfiguration);

					bUseCustomMetallic = CheckAndApplyManualTexturePath("material_configuration", "metallic_path", CustomMetallicTexture, MaterialConfiguration);
				}

				if (!bUseBakedAO)
					bUseBakedAO = ConcatAndFindTexture(BakedAOTexture, AlbedoTexture, "_ao");

				if (!bUseCustomGloss)
					bUseCustomGloss = ConcatAndFindTexture(CustomGlossTexture, AlbedoTexture, "_gloss");

				if (!bUseCustomRoughness && !bUseCustomGloss)
					bUseCustomRoughness = ConcatAndFindTexture(CustomRoughnessTexture, AlbedoTexture, "_roughness");

				if (!bUseCustomMetallic)
					bUseCustomMetallic = ConcatAndFindTexture(CustomMetallicTexture, AlbedoTexture, "_metallic");

				C.r_Define(bUseBakedAO, "USE_BAKED_AO", "1");
				C.r_Define(bUseCustomRoughness, "USE_CUSTOM_ROUGHNESS", "1");
				C.r_Define(bUseCustomGloss, "USE_CUSTOM_GLOSS", "1");
				C.r_Define(bUseCustomMetallic, "USE_CUSTOM_METALLIC", "1");
			}

			if (bUseConfigurator)
			{
				bUseCustomNormal = CheckAndApplyManualTexturePath("material_configuration", "normal_path", CustomNormalTexture, MaterialConfiguration);
				bUseCustomSubsurfacePower = CheckAndApplyManualTexturePath("material_configuration", "subsurface_power_path", CustomSubsurfacePowerTexture, MaterialConfiguration);
				bUseCustomCavity = CheckAndApplyManualTexturePath("material_configuration", "cavity_path", CustomCavityTexture, MaterialConfiguration);
				bUseCustomSpecularTint = CheckAndApplyManualTexturePath("material_configuration", "specular_tint_path", CustomSpecularTintTexture, MaterialConfiguration);
				bUseCustomSheenIntensity = CheckAndApplyManualTexturePath("material_configuration", "sheen_intensity_path", CustomSheenIntensityTexture, MaterialConfiguration);
				bUseCustomSheenRoughness = CheckAndApplyManualTexturePath("material_configuration", "sheen_roughness_path", CustomSheenRoughnessTexture, MaterialConfiguration);
				bUseCustomCoatIntensity = CheckAndApplyManualTexturePath("material_configuration", "coat_intensity_path", CustomCoatIntensityTexture, MaterialConfiguration);
				bUseCustomCoatRoughness = CheckAndApplyManualTexturePath("material_configuration", "coat_roughness_path", CustomCoatRoughnessTexture, MaterialConfiguration);
			}

			if (!bUseCustomNormal)
				bUseCustomNormal = ConcatAndFindTexture(CustomNormalTexture, AlbedoTexture, "_normal");

			C.r_Define(bUseCustomNormal, "USE_CUSTOM_NORMAL", "1");

			if (!bUseCustomSubsurfacePower)
				bUseCustomSubsurfacePower = ConcatAndFindTexture(CustomSubsurfacePowerTexture, AlbedoTexture, "_subsurface_power");

			C.r_Define(bUseCustomSubsurfacePower, "USE_CUSTOM_SUBSURFACE_POWER", "1");

			if (!bUseCustomCavity)
				bUseCustomCavity = ConcatAndFindTexture(CustomCavityTexture, AlbedoTexture, "_cavity");

			C.r_Define(bUseCustomCavity, "USE_CUSTOM_CAVITY", "1");

			if (!bUseCustomSpecularTint)
				bUseCustomSpecularTint = ConcatAndFindTexture(CustomSpecularTintTexture, AlbedoTexture, "_specular_tint");

			C.r_Define(bUseCustomSpecularTint, "USE_CUSTOM_SPECULAR_TINT", "1");

			if (!bUseCustomSheenIntensity)
				bUseCustomSheenIntensity = ConcatAndFindTexture(CustomSheenIntensityTexture, AlbedoTexture, "_sheen_intensity");

			C.r_Define(bUseCustomSheenIntensity, "USE_CUSTOM_SHEEN_INTENSITY", "1");

			if (!bUseCustomSheenRoughness)
				bUseCustomSheenRoughness = ConcatAndFindTexture(CustomSheenRoughnessTexture, AlbedoTexture, "_sheen_roughness");

			C.r_Define(bUseCustomSheenRoughness, "USE_CUSTOM_SHEEN_ROUGHNESS", "1");

			if (!bUseCustomCoatIntensity)
				bUseCustomCoatIntensity = ConcatAndFindTexture(CustomCoatIntensityTexture, AlbedoTexture, "_coat_intensity");

			C.r_Define(bUseCustomCoatIntensity, "USE_CUSTOM_COAT_INTENSITY", "1");

			if (!bUseCustomCoatRoughness)
				bUseCustomCoatRoughness = ConcatAndFindTexture(CustomCoatRoughnessTexture, AlbedoTexture, "_coat_roughness");

			C.r_Define(bUseCustomCoatRoughness, "USE_CUSTOM_COAT_ROUGHNESS", "1");
		}

		if (bUseConfigurator)
		{
			bUseCustomEmission = CheckAndApplyManualTexturePath("material_configuration", "emission_path",
																CustomEmissionTexture, MaterialConfiguration);
		}

		if (!bUseCustomEmission)
			bUseCustomEmission = ConcatAndFindTexture(CustomEmissionTexture, AlbedoTexture, "_emission");

		C.r_Define(bUseCustomEmission, "USE_CUSTOM_EMISSION", "1");

		// Create shader with normal mapping or displacement if need
		int DisplacementType = 1;
		bool bUseDisplacement = true;

		// Configuration file have priority above original parameters
		if (bUseConfigurator && bIsHightQualityGeometry)
		{
			// Check do we realy need for displacement or not
			bUseDisplacement =
				GetBoolValueIfExist("material_configuration", "use_displacement", false, MaterialConfiguration);

			// Check what displacement type we need to set
			if (bUseDisplacement)
			{
				LPCSTR displacement_type = GetStringValueIfExist("material_configuration", "displacement_type", "parallax_occlusion_mapping", MaterialConfiguration);

				if (StringsIsSimilar(displacement_type, "normal_mapping"))
					DisplacementType = 1;
				else if (StringsIsSimilar(displacement_type, "parallax_mapping"))
					DisplacementType = 2;
				else if (StringsIsSimilar(displacement_type, "parallax_occlusion_mapping"))
					DisplacementType = 3;
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

		C.r_Define(DisplacementType == 1, "USE_NORMAL_MAPPING", "1");
		C.r_Define(DisplacementType == 2, "USE_PARALLAX_MAPPING", "1");
		C.r_Define(DisplacementType == 3, "USE_PARALLAX_OCCLUSION_MAPPING", "1");

		// Get displacement texture
		if (bUseConfigurator)
			bUseCustomDisplacement = CheckAndApplyManualTexturePath("material_configuration", "displacement_path", CustomDisplacementTexture, MaterialConfiguration);

		if (!bUseCustomDisplacement)
			bUseCustomDisplacement = ConcatAndFindTexture(CustomDisplacementTexture, AlbedoTexture, "_displacement");

		C.r_Define(bUseCustomDisplacement, "USE_CUSTOM_DISPLACEMENT", "1");

		// Create shader with deatil texture if need
		C.r_Define(bUseDetail, "USE_TDETAIL", "1");

		C.r_Define(bUseDetailBump, "USE_DETAIL_BUMP", "1");

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
	}
	// Create shader pass
	strconcat(sizeof(NewPixelShaderName), NewPixelShaderName, "gbuffer_stage_", PixelShaderName);
	strconcat(sizeof(NewVertexShaderName), NewVertexShaderName, "gbuffer_stage_", VertexShaderName);
	C.r_Pass(NewVertexShaderName, NewPixelShaderName, FALSE);

	C.r_Sampler("s_base", AlbedoTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC, true);
	
	if (bUseOpacity)
		C.r_Sampler("s_custom_opacity", OpacityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseARMMap)
	{
		C.r_Sampler("s_arm_map", ARMTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
	}
	else
	{
		if (bUseERMMap)
		{
			C.r_Sampler("s_erm_map", ERMTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		}

		if (bUseBakedAO)
			C.r_Sampler("s_baked_ao", BakedAOTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		if (bUseCustomRoughness)
			C.r_Sampler("s_custom_roughness", CustomRoughnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		if (bUseCustomGloss)
			C.r_Sampler("s_custom_gloss", CustomGlossTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

		if (bUseCustomMetallic)
			C.r_Sampler("s_custom_metallic", CustomMetallicTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
	}

	if (bUseCustomNormal)
		C.r_Sampler("s_custom_normal", CustomNormalTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSubsurfacePower)
		C.r_Sampler("s_custom_subsurface_power", CustomSubsurfacePowerTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomEmission)
		C.r_Sampler("s_custom_emission", CustomEmissionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomDisplacement)
		C.r_Sampler("s_custom_displacement", CustomDisplacementTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCavity)
		C.r_Sampler("s_custom_cavity", CustomCavityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSpecularTint)
		C.r_Sampler("s_custom_specular_tint", CustomSpecularTintTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSheenIntensity)
		C.r_Sampler("s_custom_sheen_intensity", CustomSheenIntensityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSheenRoughness)
		C.r_Sampler("s_custom_sheen_roughness", CustomSheenRoughnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCoatIntensity)
		C.r_Sampler("s_custom_coat_intensity", CustomCoatIntensityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCoatRoughness)
		C.r_Sampler("s_custom_coat_roughness", CustomCoatRoughnessTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomWeight)
		C.r_Sampler("s_custom_weight", CustomWeightTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseBump)
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
		C.r_Sampler("s_hemi", HemisphereLightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
		C.r_Sampler("s_lmap", LightMapTexture, false, D3DTADDRESS_CLAMP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
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

	bool DisablePBR = !ps_r_shading_flags.test(RFLAG_ENABLE_PBR);

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

	// Check do we need to use custom shader
	bool bIsSrgbAlbedo = true;
	if (bUseConfigurator)
	{
		LPCSTR AlbedoColorSpace = GetStringValueIfExist("albedo_configuration", "color_space", "srgb", MaterialConfiguration);

		if (StringsIsSimilar(AlbedoColorSpace, "linear"))
			bIsSrgbAlbedo = false;
	}

	C.r_Define(bIsSrgbAlbedo, "USE_SRGB_COLOR_CONVERTING", "1");

	C.r_Define(true, "USE_NORMAL_MAPPING", "1");

	// Get opacity texture
	bool bUseOpacity = false;
	string_path OpacityTexture;
	bUseOpacity = ConcatAndFindLevelTexture(OpacityTexture, AlbedoTexture, "_opacity");
	C.r_Define(bUseOpacity, "USE_CUSTOM_OPACITY", "1");

	// Create shader with alpha testing if need
	C.r_Define(bUseAlpha || bUseOpacity, "USE_ALPHA_TEST", "1");

	// Get BakedAO texture
	bool bUseBakedAO = false;
	string_path BakedAOTexture;
	bUseBakedAO = ConcatAndFindLevelTexture(BakedAOTexture, AlbedoTexture, "_ao") && !DisablePBR;
	C.r_Define(bUseBakedAO, "USE_BAKED_AO", "1");

	// Get normal texture
	bool bUseCustomNormal = false;
	string_path CustomNormalTexture;
	bUseCustomNormal = ConcatAndFindLevelTexture(CustomNormalTexture, AlbedoTexture, "_normal") && !DisablePBR;
	C.r_Define(bUseCustomNormal, "USE_CUSTOM_NORMAL", "1");

	// Get roughness texture
	bool bUseCustomRoughness = false;
	string_path CustomRoughnessTexture;
	bUseCustomRoughness = ConcatAndFindLevelTexture(CustomRoughnessTexture, AlbedoTexture, "_roughness") && !DisablePBR;
	C.r_Define(bUseCustomRoughness, "USE_CUSTOM_ROUGHNESS", "1");

	// Get metallic texture
	bool bUseCustomMetallic = false;
	string_path CustomMetallicTexture;
	bUseCustomMetallic = ConcatAndFindLevelTexture(CustomMetallicTexture, AlbedoTexture, "_metallic") && !DisablePBR;
	C.r_Define(bUseCustomMetallic, "USE_CUSTOM_METALLIC", "1");

	// Get subsurface_power power texture
	bool bUseCustomSubsurfacePower = false;
	string_path CustomSubsurfacePowerTexture;
	bUseCustomSubsurfacePower = ConcatAndFindLevelTexture(CustomSubsurfacePowerTexture, AlbedoTexture, "_subsurface_power") && !DisablePBR;
	C.r_Define(bUseCustomSubsurfacePower, "USE_CUSTOM_SUBSURFACE_POWER", "1");
	Msg("CustomSubsurfacePowerTexture %s", CustomSubsurfacePowerTexture);

	// Get emission power texture
	bool bUseCustomEmission = false;
	string_path CustomEmissionTexture;
	bUseCustomEmission = ConcatAndFindLevelTexture(CustomEmissionTexture, AlbedoTexture, "_emission");
	C.r_Define(bUseCustomEmission, "USE_CUSTOM_EMISSION", "1");

	// Get displacement texture
	bool bUseCustomDisplacement = false;
	string_path CustomDisplacementTexture;
	bUseCustomDisplacement = ConcatAndFindLevelTexture(CustomDisplacementTexture, AlbedoTexture, "_displacement") && !DisablePBR;
	C.r_Define(bUseCustomDisplacement, "USE_CUSTOM_DISPLACEMENT", "1");

	// Get cavity texture
	bool bUseCustomCavity = false;
	string_path CustomCavityTexture;
	bUseCustomCavity = ConcatAndFindLevelTexture(CustomCavityTexture, AlbedoTexture, "_cavity") && !DisablePBR;
	C.r_Define(bUseCustomCavity, "USE_CUSTOM_CAVITY", "1");

	// Get weight texture
	bool bUseCustomWeight = false;
	string_path CustomWeightTexture;
	bUseCustomWeight = ConcatAndFindLevelTexture(CustomWeightTexture, AlbedoTexture, "_weight");
	C.r_Define(bUseCustomWeight, "USE_WEIGHT_MAP", "1");

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

	if (bUseCustomMetallic)
		C.r_Sampler("s_custom_metallic", CustomMetallicTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomSubsurfacePower)
		C.r_Sampler("s_custom_subsurface_power", CustomSubsurfacePowerTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomEmission)
		C.r_Sampler("s_custom_emission", CustomEmissionTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomDisplacement)
		C.r_Sampler("s_custom_displacement", CustomDisplacementTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomCavity)
		C.r_Sampler("s_custom_cavity", CustomCavityTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	if (bUseCustomWeight)
		C.r_Sampler("s_custom_weight", CustomWeightTexture, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);

	jitter(C);

	C.r_End();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
