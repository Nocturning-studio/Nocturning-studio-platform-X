#define _CRT_SECURE_NO_WARNINGS
#include <fcntl.h>
#include <io.h>
#include <cstdio>
#include <DXSDK/d3d9.h>
#include <DXSDK/d3dx9.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <windows.h>
#include <shellapi.h>
#include <cwctype>
#include <fstream>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")

// ... (Шейдеры остаются без изменений) ...
const char* uberPixelShaderSrc = "sampler2D texMain : register(s0);      // Albedo / Normal / Base \n"
								 "sampler2D texAO   : register(s1);      // AO \n"
								 "sampler2D texRough: register(s2);      // Roughness / Gloss \n"
								 "sampler2D texMetal: register(s3);      // Metallic \n"
								 "sampler2D texExtra: register(s4);      // Height / Opacity \n"
								 "\n"
								 "struct PS_INPUT { \n"
								 "    float2 texCoord : TEXCOORD0; \n"
								 "}; \n"
								 "\n"
								 "float4 main(PS_INPUT input) : COLOR { \n"
								 "    float4 result = float4(0, 0, 0, 1); \n"
								 "\n"
								 "#ifdef MODE_ARM \n"
								 "    // r - AO, g - Roughness, b - Metallic \n"
								 "    float ao = tex2D(texAO, input.texCoord).r; \n"
								 "    \n"
								 "    float rough = tex2D(texRough, input.texCoord).r; \n"
								 "    #ifdef USE_GLOSS_INPUT \n"
								 "        rough = 1.0f - rough; // Конвертация Gloss -> Roughness \n"
								 "    #endif \n"
								 "    \n"
								 "    float metal = tex2D(texMetal, input.texCoord).r; \n"
								 "    \n"
								 "    result.r = ao; \n"
								 "    result.g = rough; \n"
								 "    result.b = metal; \n"
								 "    result.a = 1.0f; \n"
								 "    \n"
								 "    #ifdef PACK_HEIGHT_ALPHA \n"
								 "        result.a = tex2D(texExtra, input.texCoord).r; // Displacement в Alpha \n"
								 "    #endif \n"
								 "#endif \n"
								 "\n"
								 "#ifdef MODE_PACKED_NORMAL \n"
								 "    // r - black, g - x, b - black, a - y \n"
								 "    float4 n = tex2D(texMain, input.texCoord); \n"
								 "    // Предполагаем входную нормаль как RGB (x, y, z) \n"
								 "    result = float4(0.0f, n.r, 0.0f, n.g); \n"
								 "#endif \n"
								 "\n"
								 "#ifdef MODE_ALBEDO_OPACITY \n"
								 "    float4 albedo = tex2D(texMain, input.texCoord); \n"
								 "    float opacity = tex2D(texExtra, input.texCoord).r; \n"
								 "    result = float4(albedo.rgb, opacity); \n"
								 "#endif \n"
								 "\n"
								 "#ifdef MODE_PASSTHROUGH \n"
								 "    result = tex2D(texMain, input.texCoord); \n"
								 "#endif \n"
								 "\n"
								 "    return result; \n"
								 "} \n";

const char* simpleVertexShaderSrc = "float4x4 worldViewProj; \n"
									"struct VS_INPUT { \n"
									"    float3 position : POSITION; \n"
									"    float2 texCoord : TEXCOORD0; \n"
									"}; \n"
									"struct VS_OUTPUT { \n"
									"    float4 position : POSITION; \n"
									"    float2 texCoord : TEXCOORD0; \n"
									"}; \n"
									"VS_OUTPUT main(VS_INPUT input) { \n"
									"    VS_OUTPUT output; \n"
									"    output.position = mul(float4(input.position, 1.0), worldViewProj); \n"
									"    output.texCoord = input.texCoord; \n"
									"    return output; \n"
									"} \n";

// ... (Вспомогательные структуры остаются без изменений) ...
struct Vertex
{
	float x, y, z;
	float tu, tv;
};

const std::vector<std::wstring> KNOWN_SUFFIXES = {
	L"_ao",		L"_roughness",	  L"_metallic",		  L"_gloss",		 L"_normal", L"_opacity",
	L"_height", L"_displacement", L"_emissive_power", L"_specular_tint", L"_cavity", L"subsurface_power"};

std::wstring ToLower(const std::wstring& str)
{
	std::wstring result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::towlower);
	return result;
}

std::wstring GetFileExtension(const std::wstring& filePath)
{
	size_t lastDot = filePath.find_last_of(L".");
	if (lastDot == std::wstring::npos)
		return L"";
	return ToLower(filePath.substr(lastDot + 1));
}

std::wstring GetDirectory(const std::wstring& filePath)
{
	size_t lastSlash = filePath.find_last_of(L"\\/");
	if (lastSlash == std::wstring::npos)
		return L"";
	return filePath.substr(0, lastSlash + 1);
}

std::wstring GetFileNameNoExt(const std::wstring& filePath)
{
	size_t lastSlash = filePath.find_last_of(L"\\/");
	size_t lastDot = filePath.find_last_of(L".");
	size_t start = (lastSlash == std::wstring::npos) ? 0 : lastSlash + 1;
	if (lastDot == std::wstring::npos || lastDot < start)
		return filePath.substr(start);
	return filePath.substr(start, lastDot - start);
}

std::pair<std::wstring, std::wstring> ParseMaterialName(const std::wstring& filePath)
{
	std::wstring fileName = ToLower(GetFileNameNoExt(filePath));
	std::vector<std::wstring> sortedSuffixes = KNOWN_SUFFIXES;
	std::sort(sortedSuffixes.begin(), sortedSuffixes.end(),
			  [](const std::wstring& a, const std::wstring& b) { return a.length() > b.length(); });

	for (const auto& suffix : sortedSuffixes)
	{
		if (fileName.length() > suffix.length())
		{
			if (fileName.compare(fileName.length() - suffix.length(), suffix.length(), suffix) == 0)
			{
				std::wstring base = fileName.substr(0, fileName.length() - suffix.length());
				return {base, suffix};
			}
		}
	}
	return {fileName, L""};
}

bool FileExists(const std::wstring& path)
{
	DWORD dwAttrib = GetFileAttributes(path.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// =======================================================================================
// Класс конвертера
// =======================================================================================

class TextureProcessor
{
  private:
	LPDIRECT3DDEVICE9 device;
	LPDIRECT3DVERTEXSHADER9 vertexShader;
	LPD3DXCONSTANTTABLE vertexShaderConstants;
	LPDIRECT3DVERTEXBUFFER9 vertexBuffer;

	LPDIRECT3DTEXTURE9 texWhite;
	LPDIRECT3DTEXTURE9 texBlack;

	std::map<std::wstring, std::wstring> currentMaterialFiles;
	std::wstring currentDirectory;
	std::wstring currentBaseName;

  public:
	TextureProcessor(LPDIRECT3DDEVICE9 dev)
		: device(dev), vertexShader(NULL), vertexShaderConstants(NULL), vertexBuffer(NULL), texWhite(NULL),
		  texBlack(NULL)
	{
		device->AddRef();
		InitializeCommon();
		CreateDummyTextures();
	}

	~TextureProcessor()
	{
		if (vertexShader)
			vertexShader->Release();
		if (vertexShaderConstants)
			vertexShaderConstants->Release();
		if (vertexBuffer)
			vertexBuffer->Release();
		if (texWhite)
			texWhite->Release();
		if (texBlack)
			texBlack->Release();
		device->Release();
	}

	void ProcessMaterial(const std::wstring& directory, const std::wstring& baseName)
	{
		currentDirectory = directory;
		currentBaseName = baseName;
		currentMaterialFiles.clear();

		wprintf(L"------------------------------------------------\n");
		wprintf(L"Processing Material: %s\n", baseName.c_str());

		FindMaterialFiles();

		// --- ARM / ARMD ---
		bool hasAO = currentMaterialFiles.count(L"_ao");
		bool hasRough = currentMaterialFiles.count(L"_roughness");
		bool hasGloss = currentMaterialFiles.count(L"_gloss");
		bool hasMetal = currentMaterialFiles.count(L"_metallic");
		bool hasHeight = currentMaterialFiles.count(L"_height") || currentMaterialFiles.count(L"_displacement");

		if (hasAO || hasRough || hasGloss || hasMetal)
		{
			std::wstring suffix = hasHeight ? L"_armd" : L"_arm";
			wprintf(L"  > Generating %s...\n", suffix.c_str());
			CreateARMTexture(hasHeight);
		}

		// --- Normal Packed ---
		if (currentMaterialFiles.count(L"_normal"))
		{
			wprintf(L"  > Generating _packed_normal...\n");
			CreatePackedNormal();
		}

		// --- Albedo + Opacity ---
		bool hasMain = currentMaterialFiles.count(L"");
		bool hasOpacity = currentMaterialFiles.count(L"_opacity");

		if (hasMain)
		{
			if (hasOpacity)
			{
				wprintf(L"  > Packing Albedo + Opacity...\n");
				CreateAlbedoOpacity();
			}
			else
			{
				wprintf(L"  > Converting Albedo (DXT5)...\n");
				ConvertSimple(L"", L"", D3DFMT_DXT5);
			}
		}

		// --- L8 Textures ---
		std::vector<std::wstring> l8Targets = {L"_emissive_power", L"_specular_tint", L"_cavity", L"subsurface_power"};

		if (hasHeight && (!hasAO && !hasRough && !hasGloss && !hasMetal))
		{
			if (currentMaterialFiles.count(L"_height"))
				ConvertSimple(L"_height", L"_height", D3DFMT_L8);
			else if (currentMaterialFiles.count(L"_displacement"))
				ConvertSimple(L"_displacement", L"_displacement", D3DFMT_L8);
		}

		for (const auto& suff : l8Targets)
		{
			if (currentMaterialFiles.count(suff))
			{
				wprintf(L"  > Converting %s (L8)...\n", suff.c_str());
				ConvertSimple(suff, suff, D3DFMT_L8);
			}
		}
	}

  private:
	// === НОВАЯ ФУНКЦИЯ ДЛЯ ПЕРЕИМЕНОВАНИЯ ===
	void RenameSourceIfDDS(const std::wstring& suffix)
	{
		// Если файла с таким суффиксом нет в списке найденных - выходим
		if (currentMaterialFiles.find(suffix) == currentMaterialFiles.end())
			return;

		std::wstring path = currentMaterialFiles[suffix];

		// Проверяем, что файл заканчивается на .dds (регистронезависимо, но у нас map хранит как нашли)
		// Для надежности проверим последние 4 символа
		if (path.length() >= 4)
		{
			std::wstring ext = ToLower(path.substr(path.length() - 4));
			if (ext == L".dds")
			{
				std::wstring newPath = path.substr(0, path.length() - 4) + L".~dds";

				// Если файл назначения уже есть (от прошлого запуска), удаляем его
				DeleteFileW(newPath.c_str());

				// Переименовываем
				if (MoveFileW(path.c_str(), newPath.c_str()))
				{
					// Логгируем только имя файла
					wprintf(L"    [INFO] Renamed source to .~dds: %s\n", GetFileNameNoExt(path).c_str());
				}
				else
				{
					wprintf(L"    [WARN] Failed to rename source: %s\n", GetFileNameNoExt(path).c_str());
				}
			}
		}
	}

	void InitializeCommon()
	{
		// VS
		LPD3DXBUFFER shaderBuff = NULL;
		D3DXCompileShader(simpleVertexShaderSrc, strlen(simpleVertexShaderSrc), NULL, NULL, "main", "vs_3_0", 0,
						  &shaderBuff, NULL, &vertexShaderConstants);
		device->CreateVertexShader((DWORD*)shaderBuff->GetBufferPointer(), &vertexShader);
		shaderBuff->Release();

		// Quad
		Vertex vertices[] = {
			{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
			{-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},
			{1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f, 0.0f},
		};
		device->CreateVertexBuffer(4 * sizeof(Vertex), 0, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_DEFAULT, &vertexBuffer,
								   NULL);
		void* data;
		vertexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, vertices, sizeof(vertices));
		vertexBuffer->Unlock();
	}

	void CreateDummyTextures()
	{
		device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texWhite, NULL);
		D3DLOCKED_RECT rect;
		texWhite->LockRect(0, &rect, NULL, 0);
		*(DWORD*)rect.pBits = 0xFFFFFFFF;
		texWhite->UnlockRect(0);

		device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texBlack, NULL);
		texBlack->LockRect(0, &rect, NULL, 0);
		*(DWORD*)rect.pBits = 0xFF000000;
		texBlack->UnlockRect(0);
	}

	void FindMaterialFiles()
	{
		TryFindFile(L"");
		for (const auto& suff : KNOWN_SUFFIXES)
		{
			TryFindFile(suff);
		}
	}

	void TryFindFile(const std::wstring& suffix)
	{
		const std::wstring exts[] = {L".tga", L".png", L".jpg", L".jpeg", L".bmp", L".dds"};
		for (const auto& ext : exts)
		{
			std::wstring path = currentDirectory + currentBaseName + suffix + ext;
			if (FileExists(path))
			{
				currentMaterialFiles[suffix] = path;
				return;
			}
		}
	}

	LPDIRECT3DTEXTURE9 LoadTextureSafe(const std::wstring& suffix, LPDIRECT3DTEXTURE9 fallback)
	{
		if (currentMaterialFiles.find(suffix) == currentMaterialFiles.end())
			return fallback;

		LPDIRECT3DTEXTURE9 tex = NULL;
		HRESULT hr = D3DXCreateTextureFromFileExW(device, currentMaterialFiles[suffix].c_str(), D3DX_DEFAULT_NONPOW2,
												  D3DX_DEFAULT_NONPOW2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
												  D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tex);

		if (FAILED(hr))
			return fallback;
		return tex;
	}

	LPDIRECT3DPIXELSHADER9 CompileShader(const D3DXMACRO* macros)
	{
		LPD3DXBUFFER shaderBuff = NULL;
		LPD3DXBUFFER errorBuff = NULL;
		HRESULT hr = D3DXCompileShader(uberPixelShaderSrc, strlen(uberPixelShaderSrc), macros, NULL, "main", "ps_3_0",
									   0, &shaderBuff, &errorBuff, NULL);

		if (FAILED(hr))
		{
			if (errorBuff)
				OutputDebugStringA((char*)errorBuff->GetBufferPointer());
			if (errorBuff)
				errorBuff->Release();
			return NULL;
		}

		LPDIRECT3DPIXELSHADER9 ps = NULL;
		device->CreatePixelShader((DWORD*)shaderBuff->GetBufferPointer(), &ps);
		shaderBuff->Release();
		return ps;
	}

	void RenderAndSave(LPDIRECT3DPIXELSHADER9 ps, UINT width, UINT height, const std::wstring& outputSuffix,
					   D3DFORMAT destFormat)
	{
		HRESULT hr;

		LPDIRECT3DTEXTURE9 rtTex = NULL;
		hr = device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &rtTex,
								   NULL);
		if (FAILED(hr))
		{
			wprintf(L"[ERR] CreateTexture RT failed: 0x%08X\n", hr);
			return;
		}

		LPDIRECT3DSURFACE9 rtSurf = NULL;
		rtTex->GetSurfaceLevel(0, &rtSurf);

		LPDIRECT3DSURFACE9 sysSurf = NULL;
		hr = device->CreateOffscreenPlainSurface(width, height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sysSurf, NULL);
		if (FAILED(hr))
		{
			wprintf(L"[ERR] CreateOffscreenPlainSurface failed: 0x%08X\n", hr);
			rtSurf->Release();
			rtTex->Release();
			return;
		}

		LPDIRECT3DSURFACE9 oldRT = NULL;
		device->GetRenderTarget(0, &oldRT);
		device->SetRenderTarget(0, rtSurf);

		device->SetVertexShader(vertexShader);
		device->SetPixelShader(ps);

		D3DXMATRIX ortho;
		D3DXMatrixOrthoOffCenterLH(&ortho, -1, 1, -1, 1, 0, 1);
		vertexShaderConstants->SetMatrix(device, vertexShaderConstants->GetConstantByName(NULL, "worldViewProj"),
										 &ortho);

		device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
		device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

		device->BeginScene();
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		device->EndScene();

		device->SetRenderTarget(0, oldRT);
		oldRT->Release();

		hr = device->GetRenderTargetData(rtSurf, sysSurf);
		if (FAILED(hr))
		{
			wprintf(L"[ERR] GetRenderTargetData failed: 0x%08X\n", hr);
		}
		else
		{
			ConvertAndSave(sysSurf, outputSuffix, destFormat);
		}

		sysSurf->Release();
		rtSurf->Release();
		rtTex->Release();
	}

	void ConvertAndSave(LPDIRECT3DSURFACE9 srcSurface, const std::wstring& suffix, D3DFORMAT targetFormat)
	{
		std::wstring outPath = currentDirectory + currentBaseName + suffix + L".dds";

		D3DSURFACE_DESC desc;
		srcSurface->GetDesc(&desc);

		UINT width = desc.Width;
		UINT height = desc.Height;

		if (targetFormat == D3DFMT_DXT1 || targetFormat == D3DFMT_DXT2 || targetFormat == D3DFMT_DXT3 ||
			targetFormat == D3DFMT_DXT4 || targetFormat == D3DFMT_DXT5)
		{
			if (width % 4 != 0)
				width = (width + 3) & ~3;
			if (height % 4 != 0)
				height = (height + 3) & ~3;
		}

		LPDIRECT3DTEXTURE9 destTex = NULL;
		HRESULT hr = device->CreateTexture(width, height, 1, 0, targetFormat, D3DPOOL_SCRATCH, &destTex, NULL);

		if (SUCCEEDED(hr))
		{
			LPDIRECT3DSURFACE9 destSurf = NULL;
			destTex->GetSurfaceLevel(0, &destSurf);

			hr = D3DXLoadSurfaceFromSurface(destSurf, NULL, NULL, srcSurface, NULL, NULL, D3DX_FILTER_TRIANGLE, 0);

			if (SUCCEEDED(hr))
			{
				hr = D3DXSaveTextureToFileW(outPath.c_str(), D3DXIFF_DDS, destTex, NULL);
				if (SUCCEEDED(hr))
				{
					wprintf(L"  [OK] Saved: %s%s.dds\n", currentBaseName.c_str(), suffix.c_str());
				}
				else
				{
					wprintf(L"  [ERR] File write error: 0x%08X\n", hr);
				}
			}
			else
			{
				wprintf(L"  [ERR] Conversion/Compression failed: 0x%08X\n", hr);
			}

			destSurf->Release();
			destTex->Release();
		}
		else
		{
			wprintf(L"  [ERR] Dest texture creation failed (Sz:%dx%d Fmt:%d): 0x%08X\n", width, height, targetFormat,
					hr);
		}
	}

	void CreateARMTexture(bool includeHeight)
	{
		// Загрузка исходников или заглушек
		LPDIRECT3DTEXTURE9 tAO = LoadTextureSafe(L"_ao", texWhite);

		bool useGloss = currentMaterialFiles.count(L"_gloss") && !currentMaterialFiles.count(L"_roughness");
		LPDIRECT3DTEXTURE9 tRough = LoadTextureSafe(useGloss ? L"_gloss" : L"_roughness", texWhite);

		LPDIRECT3DTEXTURE9 tMetal = LoadTextureSafe(L"_metallic", texBlack);

		std::wstring heightSuffix = currentMaterialFiles.count(L"_displacement") ? L"_displacement" : L"_height";
		LPDIRECT3DTEXTURE9 tHeight = includeHeight ? LoadTextureSafe(heightSuffix, texWhite) : NULL;

		D3DSURFACE_DESC desc;
		if (tAO != texWhite)
			tAO->GetLevelDesc(0, &desc);
		else if (tRough != texWhite)
			tRough->GetLevelDesc(0, &desc);
		else if (tMetal != texBlack)
			tMetal->GetLevelDesc(0, &desc);
		else
		{
			desc.Width = 1024;
			desc.Height = 1024;
		}

		device->SetTexture(1, tAO);
		device->SetTexture(2, tRough);
		device->SetTexture(3, tMetal);
		if (includeHeight)
			device->SetTexture(4, tHeight);

		std::vector<D3DXMACRO> macros;
		macros.push_back({"MODE_ARM", "1"});
		if (useGloss)
			macros.push_back({"USE_GLOSS_INPUT", "1"});
		if (includeHeight)
			macros.push_back({"PACK_HEIGHT_ALPHA", "1"});
		macros.push_back({NULL, NULL});

		LPDIRECT3DPIXELSHADER9 ps = CompileShader(macros.data());
		bool success = false;
		if (ps)
		{
			RenderAndSave(ps, desc.Width, desc.Height, includeHeight ? L"_armd" : L"_arm", D3DFMT_DXT5);
			ps->Release();
			success = true;
		}

		// ВАЖНО: Сначала освобождаем ресурсы, чтобы снять лок с файлов
		device->SetTexture(1, NULL);
		device->SetTexture(2, NULL);
		device->SetTexture(3, NULL);
		device->SetTexture(4, NULL);

		if (tAO != texWhite)
			tAO->Release();
		if (tRough != texWhite)
			tRough->Release();
		if (tMetal != texBlack)
			tMetal->Release();
		if (tHeight && tHeight != texWhite)
			tHeight->Release();

		// Теперь можно переименовывать исходники, если конвертация прошла успешно (впрочем, можно и всегда)
		if (success)
		{
			RenameSourceIfDDS(L"_ao");
			RenameSourceIfDDS(useGloss ? L"_gloss" : L"_roughness");
			RenameSourceIfDDS(L"_metallic");
			if (includeHeight)
			{
				RenameSourceIfDDS(heightSuffix);
			}
		}
	}

	void CreatePackedNormal()
	{
		LPDIRECT3DTEXTURE9 tNorm = LoadTextureSafe(L"_normal", NULL);
		if (!tNorm)
			return;

		D3DSURFACE_DESC desc;
		tNorm->GetLevelDesc(0, &desc);

		device->SetTexture(0, tNorm);

		D3DXMACRO macros[] = {{"MODE_PACKED_NORMAL", "1"}, {NULL, NULL}};
		LPDIRECT3DPIXELSHADER9 ps = CompileShader(macros);

		bool success = false;
		if (ps)
		{
			RenderAndSave(ps, desc.Width, desc.Height, L"_packed_normal", D3DFMT_DXT5);
			ps->Release();
			success = true;
		}

		device->SetTexture(0, NULL);
		tNorm->Release();

		if (success)
		{
			RenameSourceIfDDS(L"_normal");
		}
	}

	void CreateAlbedoOpacity()
	{
		LPDIRECT3DTEXTURE9 tAlb = LoadTextureSafe(L"", NULL);
		LPDIRECT3DTEXTURE9 tOp = LoadTextureSafe(L"_opacity", texWhite);
		if (!tAlb)
		{
			if (tOp != texWhite)
				tOp->Release();
			return;
		}

		D3DSURFACE_DESC desc;
		tAlb->GetLevelDesc(0, &desc);

		device->SetTexture(0, tAlb);
		device->SetTexture(4, tOp);

		D3DXMACRO macros[] = {{"MODE_ALBEDO_OPACITY", "1"}, {NULL, NULL}};
		LPDIRECT3DPIXELSHADER9 ps = CompileShader(macros);

		bool success = false;
		if (ps)
		{
			RenderAndSave(ps, desc.Width, desc.Height, L"", D3DFMT_DXT5);
			ps->Release();
			success = true;
		}

		device->SetTexture(0, NULL);
		device->SetTexture(4, NULL);

		tAlb->Release();
		if (tOp != texWhite)
			tOp->Release();

		if (success)
		{
			// Если мы упаковали Opacity, значит она больше не нужна отдельным файлом
			RenameSourceIfDDS(L"_opacity");
			// Альбедо мы перезаписали, так что его переименовывать не нужно (оно и так стало .dds)
		}
	}

	void ConvertSimple(const std::wstring& inputSuffix, const std::wstring& outputSuffix, D3DFORMAT format)
	{
		// Простая конвертация обычно не требует переименования исходников в .~dds,
		// так как мы просто сохраняем в новый формат.
		// Если нужно и тут скрывать исходник (например _height.dds -> _height.~dds после конверта в L8),
		// логику можно добавить. Но обычно это делают только для packed текстур.

		LPDIRECT3DTEXTURE9 tTex = LoadTextureSafe(inputSuffix, NULL);
		if (!tTex)
			return;

		D3DSURFACE_DESC desc;
		tTex->GetLevelDesc(0, &desc);

		device->SetTexture(0, tTex);

		D3DXMACRO macros[] = {{"MODE_PASSTHROUGH", "1"}, {NULL, NULL}};
		LPDIRECT3DPIXELSHADER9 ps = CompileShader(macros);

		if (ps)
		{
			RenderAndSave(ps, desc.Width, desc.Height, outputSuffix, format);
			ps->Release();
		}

		device->SetTexture(0, NULL);
		tTex->Release();
	}
};

// =======================================================================================
// Windows Boilerplate
// =======================================================================================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT rect;
		GetClientRect(hwnd, &rect);

		SetBkMode(hdc, TRANSPARENT);
		HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
								 CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
		SelectObject(hdc, hFont);

		const wchar_t* text = L"ПЕРЕТАЩИТЕ ТЕКСТУРЫ СЮДА\n\n"
							  L"Система автоматически найдет связанные файлы материала.\n"
							  L"Например, перетащите 'wall_ao.tga', и программа найдет\n"
							  L"'wall_roughness', 'wall_metallic' и т.д.\n\n"
							  L"Результат конвертации появится в папке с исходниками.\n"
							  L"Исходные .dds файлы будут переименованы в .~dds\n\n"
							  L"Смотрите подробный лог в черном окне консоли ->";

		DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

		DeleteObject(hFont);
		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_DROPFILES: {
		HDROP hDrop = (HDROP)wParam;
		UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
		TextureProcessor* processor = (TextureProcessor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (processor && fileCount > 0)
		{
			std::set<std::wstring> processedMaterials;

			wprintf(L"\n>>> Начало обработки Drop-события (%d файлов) <<<\n", fileCount);

			for (UINT i = 0; i < fileCount; i++)
			{
				wchar_t filePath[MAX_PATH];
				DragQueryFile(hDrop, i, filePath, MAX_PATH);

				std::wstring fullPath = filePath;
				std::wstring directory = GetDirectory(fullPath);

				std::pair<std::wstring, std::wstring> parsed = ParseMaterialName(GetFileNameNoExt(fullPath));
				std::wstring baseName = parsed.first;

				std::wstring materialKey = directory + baseName;

				if (processedMaterials.find(materialKey) == processedMaterials.end())
				{
					processor->ProcessMaterial(directory, baseName);
					processedMaterials.insert(materialKey);
				}
			}
			wprintf(L"\n>>> Обработка завершена! <<<\n");
			MessageBeep(MB_ICONASTERISK);
		}
		DragFinish(hDrop);
		return 0;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	AllocConsole();

	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"TexConverterClass";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, L"TexConverterClass", L"Material Batch Converter", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							   100, 100, 800, 600, NULL, NULL, hInstance, NULL);

	DragAcceptFiles(hwnd, TRUE);

	LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d)
	{
		printf("CRITICAL: Direct3D9 SDK not found.\n");
		return -1;
	}

	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;

	LPDIRECT3DDEVICE9 device;
	HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
								   &d3dpp, &device);

	if (FAILED(hr))
	{
		hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp,
							   &device);
	}

	if (!device)
	{
		printf("CRITICAL: Could not create D3D Device. Error: 0x%08X\n", hr);
		return -1;
	}

	TextureProcessor* processor = new TextureProcessor(device);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)processor);

	printf("================================================\n");
	printf("       PBR TEXTURE CONVERTER READY              \n");
	printf("================================================\n");
	printf("Drag & Drop your textures here.\n\n");

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	delete processor;
	device->Release();
	d3d->Release();
	return 0;
}
