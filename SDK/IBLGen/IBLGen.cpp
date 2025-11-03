#include <DXSDK/d3d9.h>
#include <DXSDK/d3dx9.h>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <string>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

// Константы для максимального качества
const int MAX_SAMPLES_SPECULAR = 65536;
const int MAX_SAMPLES_IRRADIANCE = 131072;
const int MANUAL_FILTER_SAMPLES = 256;

// Простой вершинный шейдер
const char* simpleVertexShader = "float4x4 worldViewProj; \n"
								 "struct VS_INPUT { \n"
								 "    float3 position : POSITION; \n"
								 "    float3 normal : NORMAL; \n"
								 "}; \n"
								 "struct VS_OUTPUT { \n"
								 "    float4 position : POSITION; \n"
								 "    float3 worldPos : TEXCOORD0; \n"
								 "    float3 normal : TEXCOORD1; \n"
								 "}; \n"
								 "VS_OUTPUT main(VS_INPUT input) { \n"
								 "    VS_OUTPUT output; \n"
								 "    output.position = mul(float4(input.position, 1.0), worldViewProj); \n"
								 "    output.worldPos = input.position; \n"
								 "    output.normal = input.normal; \n"
								 "    return output; \n"
								 "} \n";

const char* qualitySpecularPixelShader =
	"#define PI 3.14159265359 \n"
	"#define TWO_PI 6.28318530718 \n"
	" \n"
	"float roughness; \n"
	"float resolution; \n"
	" \n"
	"// Улучшенная функция сэмплирования кубмапы с правильной межгранной фильтрацией \n"
	"float3 manualCubeSampleHighQuality(float3 dir, float currentRoughness) \n"
	"{ \n"
	"    // Используем встроенную фильтрацию Direct3D для межгранных переходов \n"
	"    // При высоких уровнях roughness увеличиваем уровень mip для исходной текстуры \n"
	"    float mipLevel = currentRoughness * 8.0; // 8.0 - максимальный уровень размытия \n"
	"    return texCUBElod(s0, float4(dir, mipLevel)).rgb; \n"
	"} \n"
	" \n"
	"// Функция Hammersley для квази-случайных последовательностей \n"
	"float2 hammersley(uint i, uint N) { \n"
	"    uint bits = (i << 16u) | (i >> 16u); \n"
	"    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u); \n"
	"    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u); \n"
	"    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u); \n"
	"    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u); \n"
	"    return float2(float(i) / float(N), float(bits) * 2.3283064365386963e-10); \n"
	"} \n"
	" \n"
	"// Функция GGX NDF \n"
	"float D_GGX(float NdotH, float roughness) { \n"
	"    float a = roughness * roughness; \n"
	"    float a2 = a * a; \n"
	"    float NdotH2 = NdotH * NdotH; \n"
	"    float denom = (NdotH2 * (a2 - 1.0) + 1.0); \n"
	"    denom = PI * denom * denom; \n"
	"    return a2 / max(denom, 0.0000001); \n"
	"} \n"
	" \n"
	"// Функция для importance sampling GGX \n"
	"float3 importanceSampleGGX(float2 Xi, float roughness, float3 N) { \n"
	"    float a = roughness * roughness; \n"
	"    \n"
	"    float phi = 2.0 * PI * Xi.x; \n"
	"    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y)); \n"
	"    float sinTheta = sqrt(1.0 - cosTheta * cosTheta); \n"
	"    \n"
	"    float3 H; \n"
	"    H.x = cos(phi) * sinTheta; \n"
	"    H.y = sin(phi) * sinTheta; \n"
	"    H.z = cosTheta; \n"
	"    \n"
	"    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0); \n"
	"    float3 tangent = normalize(cross(up, N)); \n"
	"    float3 bitangent = cross(N, tangent); \n"
	"    \n"
	"    return tangent * H.x + bitangent * H.y + N * H.z; \n"
	"} \n"
	" \n"
	"float4 main(float3 worldPos : TEXCOORD0, float3 normal : TEXCOORD1) : COLOR { \n"
	"    float3 N = normalize(normal); \n"
	"    float3 R = N; \n"
	"    float3 V = R; \n"
	"    \n"
	"    float totalWeight = 0.0; \n"
	"    float3 prefilteredColor = float3(0.0, 0.0, 0.0); \n"
	"    \n"
	"    // Адаптивное количество сэмплов в зависимости от roughness \n"
	"    uint numSamples = uint(64.0 + (1.0 - roughness) * 64.0); \n"
	"    numSamples = min(numSamples, 128u); \n"
	"    \n"
	"    for(uint i = 0u; i < numSamples; i++) { \n"
	"        float2 Xi = hammersley(i, numSamples); \n"
	"        \n"
	"        float3 H = importanceSampleGGX(Xi, roughness, N); \n"
	"        float3 L = normalize(2.0 * dot(V, H) * H - V); \n"
	"        \n"
	"        float NdotL = max(dot(N, L), 0.0); \n"
	"        \n"
	"        if(NdotL > 0.0) { \n"
	"            // Используем улучшенную функцию сэмплирования \n"
	"            float3 sampleColor = manualCubeSampleHighQuality(L, roughness); \n"
	"            \n"
	"            prefilteredColor += sampleColor * NdotL; \n"
	"            totalWeight += NdotL; \n"
	"        } \n"
	"    } \n"
	"    \n"
	"    prefilteredColor = totalWeight > 0.0 ? prefilteredColor / totalWeight : float3(0.0, 0.0, 0.0); \n"
	"    return float4(prefilteredColor, 1.0); \n"
	"} \n";

// Исправленный, максимально качественный шейдер для Irradiance IBL
const char* qualityIrradiancePixelShader =
	"#define PI 3.14159265359 \n"
	" \n"
	"// Упрощенная функция сэмплирования для irradiance \n"
	"float3 manualCubeSampleHighQuality(float3 dir) \n"
	"{ \n"
	"    return texCUBE(s0, dir).rgb; \n"
	"} \n"
	" \n"
	"// Функция Hammersley для квази-случайных последовательностей \n"
	"float2 hammersley(uint i, uint N) { \n"
	"    uint bits = (i << 16u) | (i >> 16u); \n"
	"    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u); \n"
	"    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u); \n"
	"    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u); \n"
	"    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u); \n"
	"    return float2(float(i) / float(N), float(bits) * 2.3283064365386963e-10); \n"
	"} \n"
	" \n"
	"float4 main(float3 worldPos : TEXCOORD0, float3 normal : TEXCOORD1) : COLOR { \n"
	"    float3 N = normalize(normal); \n"
	"    \n"
	"    float3 irradiance = float3(0.0, 0.0, 0.0); \n"
	"    float totalWeight = 0.0; \n"
	"    \n"
	"    // Уменьшаем количество сэмплов для irradiance \n"
	"    for(uint i = 0u; i < 1024u; i++) { \n"
	"        float2 Xi = hammersley(i, 1024); \n"
	"        \n"
	"        // Косинусное распределение для диффузного освещения \n"
	"        float phi = 2.0 * PI * Xi.x; \n"
	"        float cosTheta = sqrt(Xi.y); \n"
	"        float sinTheta = sqrt(1.0 - cosTheta * cosTheta); \n"
	"        \n"
	"        float3 L = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta); \n"
	"        \n"
	"        float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0); \n"
	"        float3 tangent = normalize(cross(up, N)); \n"
	"        float3 bitangent = cross(N, tangent); \n"
	"        L = tangent * L.x + bitangent * L.y + N * L.z; \n"
	"        \n"
	"        float NdotL = max(dot(N, L), 0.0); \n"
	"        \n"
	"        if(NdotL > 0.0) { \n"
	"            float3 sampleColor = manualCubeSampleHighQuality(L); \n"
	"            irradiance += sampleColor * NdotL; \n"
	"            totalWeight += NdotL; \n"
	"        } \n"
	"    } \n"
	"    \n"
	"    irradiance = totalWeight > 0.0 ? irradiance / totalWeight : float3(0.0, 0.0, 0.0); \n"
	"    return float4(irradiance, 1.0); \n"
	"} \n";

struct Vertex
{
	float x, y, z;
	float nx, ny, nz;
};

class IBLGenerator
{
  private:
	LPDIRECT3DDEVICE9 device;
	LPDIRECT3DCUBETEXTURE9 sourceCubeMap;

	// Шейдеры
	LPDIRECT3DVERTEXSHADER9 vertexShader;
	LPDIRECT3DPIXELSHADER9 specularPixelShader;
	LPDIRECT3DPIXELSHADER9 irradiancePixelShader;
	LPD3DXCONSTANTTABLE vertexShaderConstants;
	LPD3DXCONSTANTTABLE specularPixelShaderConstants;
	LPD3DXCONSTANTTABLE irradiancePixelShaderConstants;

	// ПРАВИЛЬНЫЕ матрицы обзора для 6 граней куба (поменяны X и Y)
	D3DXMATRIX viewMatrices[6];

	void InitializeViewMatrices()
	{
		D3DXVECTOR3 eye(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);

		// -X (левая грань) - теперь индекс 0
		D3DXVECTOR3 atXNeg(-1.0f, 0.0f, 0.0f);
		D3DXMatrixLookAtLH(&viewMatrices[0], &eye, &atXNeg, &up);

		// +X (правая грань) - теперь индекс 1
		D3DXVECTOR3 atXPos(1.0f, 0.0f, 0.0f);
		D3DXMatrixLookAtLH(&viewMatrices[1], &eye, &atXPos, &up);

		// -Y (нижняя грань) - теперь индекс 2
		D3DXVECTOR3 atYNeg(0.0f, -1.0f, 0.0f);
		D3DXVECTOR3 upYNeg(0.0f, 0.0f, 1.0f);
		D3DXMatrixLookAtLH(&viewMatrices[2], &eye, &atYNeg, &upYNeg);

		// +Y (верхняя грань) - теперь индекс 3
		D3DXVECTOR3 atYPos(0.0f, 1.0f, 0.0f);
		D3DXVECTOR3 upY(0.0f, 0.0f, -1.0f);
		D3DXMatrixLookAtLH(&viewMatrices[3], &eye, &atYPos, &upY);

		// +Z (передняя грань)
		D3DXVECTOR3 atZPos(0.0f, 0.0f, 1.0f);
		D3DXMatrixLookAtLH(&viewMatrices[4], &eye, &atZPos, &up);

		// -Z (задняя грань)
		D3DXVECTOR3 atZNeg(0.0f, 0.0f, -1.0f);
		D3DXMatrixLookAtLH(&viewMatrices[5], &eye, &atZNeg, &up);

		// Инвертируем матрицы
		for (int i = 0; i < 6; i++)
		{
			D3DXMatrixInverse(&viewMatrices[i], NULL, &viewMatrices[i]);
		}
	}

	bool InitializeShaders()
	{
		LPD3DXBUFFER shaderBuffer = NULL;
		LPD3DXBUFFER errorBuffer = NULL;

		// Компиляция вершинного шейдера
		HRESULT hr = D3DXCompileShader(simpleVertexShader, strlen(simpleVertexShader), NULL, NULL, "main", "vs_3_0", 0,
									   &shaderBuffer, &errorBuffer, &vertexShaderConstants);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
				errorBuffer->Release();
			}
			return false;
		}

		hr = device->CreateVertexShader((DWORD*)shaderBuffer->GetBufferPointer(), &vertexShader);
		shaderBuffer->Release();
		if (FAILED(hr))
			return false;

		// Компиляция пиксельного шейдера для Specular
		hr = D3DXCompileShader(qualitySpecularPixelShader, strlen(qualitySpecularPixelShader), NULL, NULL, "main",
							   "ps_3_0", 0, &shaderBuffer, &errorBuffer, &specularPixelShaderConstants);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
				errorBuffer->Release();
			}
			return false;
		}

		hr = device->CreatePixelShader((DWORD*)shaderBuffer->GetBufferPointer(), &specularPixelShader);
		shaderBuffer->Release();
		if (FAILED(hr))
			return false;

		// Компиляция пиксельного шейдера для Irradiance
		hr = D3DXCompileShader(qualityIrradiancePixelShader, strlen(qualityIrradiancePixelShader), NULL, NULL, "main",
							   "ps_3_0", 0, &shaderBuffer, &errorBuffer, &irradiancePixelShaderConstants);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
				errorBuffer->Release();
			}
			return false;
		}

		hr = device->CreatePixelShader((DWORD*)shaderBuffer->GetBufferPointer(), &irradiancePixelShader);
		if (shaderBuffer)
			shaderBuffer->Release();
		if (errorBuffer)
			errorBuffer->Release();

		return SUCCEEDED(hr);
	}

  public:
	IBLGenerator(LPDIRECT3DDEVICE9 dev)
		: device(dev), vertexShader(NULL), specularPixelShader(NULL), irradiancePixelShader(NULL),
		  vertexShaderConstants(NULL), specularPixelShaderConstants(NULL), irradiancePixelShaderConstants(NULL)
	{
		device->AddRef();
		InitializeViewMatrices();
		InitializeShaders();
	}

	~IBLGenerator()
	{
		if (sourceCubeMap)
			sourceCubeMap->Release();
		if (vertexShader)
			vertexShader->Release();
		if (specularPixelShader)
			specularPixelShader->Release();
		if (irradiancePixelShader)
			irradiancePixelShader->Release();
		if (vertexShaderConstants)
			vertexShaderConstants->Release();
		if (specularPixelShaderConstants)
			specularPixelShaderConstants->Release();
		if (irradiancePixelShaderConstants)
			irradiancePixelShaderConstants->Release();
		device->Release();
	}

	HRESULT LoadSourceCubeMap(const wchar_t* filename)
	{
		if (sourceCubeMap)
			sourceCubeMap->Release();
		return D3DXCreateCubeTextureFromFile(device, filename, &sourceCubeMap);
	}

	HRESULT GenerateSpecularIBL(LPDIRECT3DCUBETEXTURE9& specularCubeMap, UINT outputSize, UINT mipLevels)
	{
		HRESULT hr = device->CreateCubeTexture(outputSize, mipLevels, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F,
											   D3DPOOL_DEFAULT, &specularCubeMap, NULL);
		if (FAILED(hr))
			return hr;

		LPDIRECT3DSURFACE9 oldRenderTarget;
		device->GetRenderTarget(0, &oldRenderTarget);

		LPDIRECT3DVERTEXBUFFER9 vb;
		CreateCubeVertexBuffer(vb);

		// Устанавливаем шейдеры
		device->SetVertexShader(vertexShader);
		device->SetPixelShader(specularPixelShader);

		// Настройка состояний рендеринга
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// ВАЖНО: Настраиваем сэмплер для максимального качества
		device->SetTexture(0, sourceCubeMap);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 16);

		for (UINT mip = 0; mip < mipLevels; ++mip)
		{
			for (UINT face = 0; face < 6; ++face)
			{
				LPDIRECT3DSURFACE9 surface;
				specularCubeMap->GetCubeMapSurface((D3DCUBEMAP_FACES)face, mip, &surface);

				device->SetRenderTarget(0, surface);
				device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

				RenderSpecularFace(face, mip, mipLevels, outputSize);

				surface->Release();
			}
		}

		// Восстанавливаем состояния
		device->SetVertexShader(NULL);
		device->SetPixelShader(NULL);
		device->SetRenderTarget(0, oldRenderTarget);
		vb->Release();
		oldRenderTarget->Release();
		return S_OK;
	}

	HRESULT GenerateIrradianceIBL(LPDIRECT3DCUBETEXTURE9& irradianceCubeMap, UINT outputSize)
	{
		HRESULT hr = device->CreateCubeTexture(outputSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F,
											   D3DPOOL_DEFAULT, &irradianceCubeMap, NULL);
		if (FAILED(hr))
			return hr;

		LPDIRECT3DSURFACE9 oldRenderTarget;
		device->GetRenderTarget(0, &oldRenderTarget);

		LPDIRECT3DVERTEXBUFFER9 vb;
		CreateCubeVertexBuffer(vb);

		// Устанавливаем шейдеры
		device->SetVertexShader(vertexShader);
		device->SetPixelShader(irradiancePixelShader);

		// Настройка состояний рендеринга
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// ВАЖНО: Настраиваем сэмплер для максимального качества
		device->SetTexture(0, sourceCubeMap);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 16);

		for (UINT face = 0; face < 6; ++face)
		{
			LPDIRECT3DSURFACE9 surface;
			irradianceCubeMap->GetCubeMapSurface((D3DCUBEMAP_FACES)face, 0, &surface);

			device->SetRenderTarget(0, surface);
			device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

			RenderIrradianceFace(face);

			surface->Release();
		}

		// Восстанавливаем состояния
		device->SetVertexShader(NULL);
		device->SetPixelShader(NULL);
		device->SetRenderTarget(0, oldRenderTarget);
		vb->Release();
		oldRenderTarget->Release();
		return S_OK;
	}

  private:
	void CreateCubeVertexBuffer(LPDIRECT3DVERTEXBUFFER9& vb)
	{
		// ПРАВИЛЬНЫЙ порядок вершин куба (поменяны X и Y)
		Vertex vertices[] = {// -X face (левая грань) - теперь индекс 0
							 {-1, -1, 1, -1, 0, 0},
							 {-1, 1, 1, -1, 0, 0},
							 {-1, 1, -1, -1, 0, 0},
							 {-1, 1, -1, -1, 0, 0},
							 {-1, -1, -1, -1, 0, 0},
							 {-1, -1, 1, -1, 0, 0},

							 // +X face (правая грань) - теперь индекс 1
							 {1, -1, -1, 1, 0, 0},
							 {1, 1, -1, 1, 0, 0},
							 {1, 1, 1, 1, 0, 0},
							 {1, 1, 1, 1, 0, 0},
							 {1, -1, 1, 1, 0, 0},
							 {1, -1, -1, 1, 0, 0},

							 // -Y face (нижняя грань) - теперь индекс 2
							 {-1, -1, -1, 0, -1, 0},
							 {1, -1, -1, 0, -1, 0},
							 {1, -1, 1, 0, -1, 0},
							 {1, -1, 1, 0, -1, 0},
							 {-1, -1, 1, 0, -1, 0},
							 {-1, -1, -1, 0, -1, 0},

							 // +Y face (верхняя грань) - теперь индекс 3
							 {-1, 1, 1, 0, 1, 0},
							 {1, 1, 1, 0, 1, 0},
							 {1, 1, -1, 0, 1, 0},
							 {1, 1, -1, 0, 1, 0},
							 {-1, 1, -1, 0, 1, 0},
							 {-1, 1, 1, 0, 1, 0},

							 // +Z face (передняя грань)
							 {-1, -1, 1, 0, 0, 1},
							 {-1, 1, 1, 0, 0, 1},
							 {1, 1, 1, 0, 0, 1},
							 {1, 1, 1, 0, 0, 1},
							 {1, -1, 1, 0, 0, 1},
							 {-1, -1, 1, 0, 0, 1},

							 // -Z face (задняя грань)
							 {1, -1, -1, 0, 0, -1},
							 {1, 1, -1, 0, 0, -1},
							 {-1, 1, -1, 0, 0, -1},
							 {-1, 1, -1, 0, 0, -1},
							 {-1, -1, -1, 0, 0, -1},
							 {1, -1, -1, 0, 0, -1}};

		device->CreateVertexBuffer(36 * sizeof(Vertex), 0, D3DFVF_XYZ | D3DFVF_NORMAL, D3DPOOL_DEFAULT, &vb, NULL);

		void* data;
		vb->Lock(0, 0, &data, 0);
		memcpy(data, vertices, sizeof(vertices));
		vb->Unlock();

		device->SetStreamSource(0, vb, 0, sizeof(Vertex));
		device->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL);
	}

	void RenderSpecularFace(UINT face, UINT mip, UINT totalMips, UINT resolution)
	{
		device->BeginScene();

		// Устанавливаем матрицы
		D3DXMATRIX proj;
		D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 2, 1.0f, 0.1f, 10.0f);

		D3DXMATRIX view = viewMatrices[face];
		D3DXMATRIX world;
		D3DXMatrixIdentity(&world);

		D3DXMATRIX worldViewProj = world * view * proj;

		// Устанавливаем константы для вершинного шейдера
		if (vertexShaderConstants)
		{
			D3DXHANDLE handle = vertexShaderConstants->GetConstantByName(NULL, "worldViewProj");
			vertexShaderConstants->SetMatrix(device, handle, &worldViewProj);
		}

		// Устанавливаем параметры для пиксельного шейдера
		if (specularPixelShaderConstants)
		{
			float roughness = (float)mip / (float)(totalMips - 1);
			D3DXHANDLE roughnessHandle = specularPixelShaderConstants->GetConstantByName(NULL, "roughness");
			specularPixelShaderConstants->SetFloat(device, roughnessHandle, roughness);

			D3DXHANDLE resolutionHandle = specularPixelShaderConstants->GetConstantByName(NULL, "resolution");
			specularPixelShaderConstants->SetFloat(device, resolutionHandle, (float)resolution);
		}

		// Рендерим куб
		device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12);

		device->EndScene();
	}

	void RenderIrradianceFace(UINT face)
	{
		device->BeginScene();

		// Устанавливаем матрицы
		D3DXMATRIX proj;
		D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 2, 1.0f, 0.1f, 10.0f);

		D3DXMATRIX view = viewMatrices[face];
		D3DXMATRIX world;
		D3DXMatrixIdentity(&world);

		D3DXMATRIX worldViewProj = world * view * proj;

		// Устанавливаем константы для вершинного шейдера
		if (vertexShaderConstants)
		{
			D3DXHANDLE handle = vertexShaderConstants->GetConstantByName(NULL, "worldViewProj");
			vertexShaderConstants->SetMatrix(device, handle, &worldViewProj);
		}

		// Рендерим куб
		device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12);

		device->EndScene();
	}
};

// Функция для извлечения пути и имени файла
std::wstring GetFileNameWithoutExtension(const std::wstring& filePath)
{
	size_t lastSlash = filePath.find_last_of(L"\\/");
	size_t lastDot = filePath.find_last_of(L".");

	if (lastDot == std::wstring::npos || lastDot < lastSlash)
		return filePath.substr(lastSlash + 1);

	return filePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
}

std::wstring GetDirectory(const std::wstring& filePath)
{
	size_t lastSlash = filePath.find_last_of(L"\\/");
	if (lastSlash == std::wstring::npos)
		return L"";
	return filePath.substr(0, lastSlash + 1);
}

// Функция для обработки одной cubemap текстуры
bool ProcessCubeMapFile(const std::wstring& filePath, IBLGenerator& generator)
{
	wprintf(L"Обработка файла: %s\n", filePath.c_str());

	// Загрузка исходной Cubemap текстуры
	if (FAILED(generator.LoadSourceCubeMap(filePath.c_str())))
	{
		wprintf(L"Ошибка: Не удалось загрузить cubemap текстуру: %s\n", filePath.c_str());
		return false;
	}

	LPDIRECT3DCUBETEXTURE9 specularCubeMap = nullptr;
	LPDIRECT3DCUBETEXTURE9 irradianceCubeMap = nullptr;

	// Генерация Specular IBL с mip-уровнями
	wprintf(L"Генерация specular IBL...\n");
	if (FAILED(generator.GenerateSpecularIBL(specularCubeMap, 512, 9)))
	{
		wprintf(L"Ошибка: Не удалось сгенерировать specular IBL для: %s\n", filePath.c_str());
		return false;
	}

	// Генерация Irradiance IBL без mip-уровней
	wprintf(L"Генерация irradiance IBL...\n");
	if (FAILED(generator.GenerateIrradianceIBL(irradianceCubeMap, 32)))
	{
		wprintf(L"Ошибка: Не удалось сгенерировать irradiance IBL для: %s\n", filePath.c_str());
		specularCubeMap->Release();
		return false;
	}

	// Формирование путей для сохранения
	std::wstring baseName = GetFileNameWithoutExtension(filePath);
	std::wstring directory = GetDirectory(filePath);

	std::wstring specularPath = directory + baseName + L".dds";
	std::wstring irradiancePath = directory + baseName + L"#irradiance.dds";

	// Сохранение результатов
	wprintf(L"Сохранение specular IBL: %s\n", specularPath.c_str());
	if (FAILED(D3DXSaveTextureToFile(specularPath.c_str(), D3DXIFF_DDS, specularCubeMap, NULL)))
	{
		wprintf(L"Ошибка: Не удалось сохранить specular IBL: %s\n", specularPath.c_str());
	}

	wprintf(L"Сохранение irradiance IBL: %s\n", irradiancePath.c_str());
	if (FAILED(D3DXSaveTextureToFile(irradiancePath.c_str(), D3DXIFF_DDS, irradianceCubeMap, NULL)))
	{
		wprintf(L"Ошибка: Не удалось сохранить irradiance IBL: %s\n", irradiancePath.c_str());
	}

	specularCubeMap->Release();
	irradianceCubeMap->Release();

	wprintf(L"Успешно обработан файл: %s\n\n", filePath.c_str());
	return true;
}

// Оконная процедура для обработки сообщений drag and drop
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_DROPFILES: {
		HDROP hDrop = (HDROP)wParam;
		UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

		// Получаем устройство Direct3D из пользовательских данных окна
		IBLGenerator* generator = (IBLGenerator*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (generator && fileCount > 0)
		{
			wprintf(L"Обнаружено %d файлов для обработки:\n", fileCount);

			for (UINT i = 0; i < fileCount; i++)
			{
				wchar_t filePath[MAX_PATH];
				DragQueryFile(hDrop, i, filePath, MAX_PATH);
				ProcessCubeMapFile(filePath, *generator);
			}

			wprintf(L"Обработка всех файлов завершена!\n");
		}

		DragFinish(hDrop);
		return 0;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// Выводим инструкцию для пользователя
		RECT rect;
		GetClientRect(hwnd, &rect);
		DrawText(hdc, L"Перетащите cubemap текстуры (.dds) в это окно для обработки", -1, &rect,
				 DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		EndPaint(hwnd, &ps);
		return 0;
	}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Регистрация класса окна
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"IBLGeneratorWindow";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClass(&wc);

	// Создание окна
	HWND hwnd =
		CreateWindowEx(0, L"IBLGeneratorWindow", L"IBL Generator - Drag & Drop Tool", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
					   CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, NULL, NULL, hInstance, NULL);

	if (!hwnd)
		return -1;

	// Включаем поддержку drag and drop
	DragAcceptFiles(hwnd, TRUE);

	// Инициализация Direct3D
	LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d)
	{
		MessageBox(hwnd, L"Не удалось инициализировать Direct3D 9", L"Ошибка", MB_ICONERROR);
		return -1;
	}

	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferWidth = 1;
	d3dpp.BackBufferHeight = 1;
	d3dpp.hDeviceWindow = hwnd;

	LPDIRECT3DDEVICE9 device;
	HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
								   &d3dpp, &device);

	if (FAILED(hr))
	{
		MessageBox(hwnd, L"Не удалось создать устройство Direct3D", L"Ошибка", MB_ICONERROR);
		d3d->Release();
		return -1;
	}

	// Создание генератора IBL и сохранение указателя в пользовательских данных окна
	IBLGenerator* generator = new IBLGenerator(device);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)generator);

	// Вывод инструкции
	wprintf(L"IBL Generator запущен!\n");
	wprintf(L"Перетащите cubemap текстуры (.dds) в окно для обработки.\n");
	wprintf(L"Для каждой текстуры будут созданы:\n");
	wprintf(L"  - Оригинальное_имя.dds (specular IBL с mip-уровнями)\n");
	wprintf(L"  - Оригинальное_имя#irradiance.dds (irradiance IBL)\n\n");

	// Цикл сообщений
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Очистка ресурсов
	delete generator;
	device->Release();
	d3d->Release();

	return 0;
}
