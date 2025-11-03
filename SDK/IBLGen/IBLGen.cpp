#include <DXSDK/d3d9.h>
#include <DXSDK/d3dx9.h>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

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

// Упрощенный, но максимально качественный шейдер для Specular IBL
const char* qualitySpecularPixelShader =
	"#define PI 3.14159265359 \n"
	"#define TWO_PI 6.28318530718 \n"
	" \n"
	"float roughness; \n"
	"float resolution; \n"
	" \n"
	"// Детектирование грани кубмапы по направлению \n"
	"int getFaceIndex(float3 dir) \n"
	"{ \n"
	"    float3 absDir = abs(dir); \n"
	"    \n"
	"    if (absDir.x >= absDir.y && absDir.x >= absDir.z) \n"
	"        return dir.x > 0.0 ? 0 : 1; \n"
	"    else if (absDir.y >= absDir.x && absDir.y >= absDir.z) \n"
	"        return dir.y > 0.0 ? 2 : 3; \n"
	"    else \n"
	"        return dir.z > 0.0 ? 4 : 5; \n"
	"} \n"
	" \n"
	"// Преобразование направления в UV координаты для конкретной грани \n"
	"float2 directionToFaceUV(float3 dir, int faceIndex) \n"
	"{ \n"
	"    float3 absDir = abs(dir); \n"
	"    float ma; \n"
	"    float2 uv; \n"
	"    \n"
	"    switch(faceIndex) { \n"
	"        case 0: // +X \n"
	"            ma = 0.5 / absDir.x; \n"
	"            uv = float2(-dir.z, -dir.y); \n"
	"            break; \n"
	"        case 1: // -X \n"
	"            ma = 0.5 / absDir.x; \n"
	"            uv = float2(dir.z, -dir.y); \n"
	"            break; \n"
	"        case 2: // +Y \n"
	"            ma = 0.5 / absDir.y; \n"
	"            uv = float2(dir.x, dir.z); \n"
	"            break; \n"
	"        case 3: // -Y \n"
	"            ma = 0.5 / absDir.y; \n"
	"            uv = float2(dir.x, -dir.z); \n"
	"            break; \n"
	"        case 4: // +Z \n"
	"            ma = 0.5 / absDir.z; \n"
	"            uv = float2(dir.x, -dir.y); \n"
	"            break; \n"
	"        case 5: // -Z \n"
	"            ma = 0.5 / absDir.z; \n"
	"            uv = float2(-dir.x, -dir.y); \n"
	"            break; \n"
	"    } \n"
	"    \n"
	"    return uv * ma + 0.5; \n"
	"} \n"
	" \n"
	"// Ручное сэмплирование исходной кубмапы с межгранной фильтрацией \n"
	"float3 manualCubeSampleHighQuality(float3 dir, float currentRoughness) \n"
	"{ \n"
	"    // ВАЖНО: Всегда сэмплируем исходную текстуру (Mip 0) \n"
	"    float3 result = float3(0.0, 0.0, 0.0); \n"
	"    float totalWeight = 0.0; \n"
	"    \n"
	"    // Определяем основную грань \n"
	"    int mainFace = getFaceIndex(dir); \n"
	"    \n"
	"    // Вычисляем уровень размытия на основе roughness \n"
	"    float blurAmount = currentRoughness * 0.1; \n"
	"    \n"
	"    // Многократное сэмплирование для smooth переходов \n"
	"    for (int i = 0; i < 256; i++) \n"
	"    { \n"
	"        // Генерируем случайное смещение \n"
	"        float2 randomOffset = float2( \n"
	"            (fmod(float(i) * 1.618034, 1.0) - 0.5) * 2.0, \n"
	"            (fmod(float(i) * 2.678234, 1.0) - 0.5) * 2.0 \n"
	"        ) * blurAmount; \n"
	"        \n"
	"        float3 sampleDir = normalize(dir + float3(randomOffset.x, randomOffset.y, 0.0)); \n"
	"        \n"
	"        // Всегда сэмплируем Mip 0 исходной текстуры \n"
	"        result += texCUBE(s0, sampleDir).rgb; \n"
	"        totalWeight += 1.0; \n"
	"    } \n"
	"    \n"
	"    return result / totalWeight; \n"
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
	"    for(uint i = 0u; i < 65536; i++) { \n"
	"        float2 Xi = hammersley(i, 65536); \n"
	"        \n"
	"        float3 H = importanceSampleGGX(Xi, roughness, N); \n"
	"        float3 L = normalize(2.0 * dot(V, H) * H - V); \n"
	"        \n"
	"        float NdotL = max(dot(N, L), 0.0); \n"
	"        \n"
	"        if(NdotL > 0.0) { \n"
	"            // ВАЖНОЕ ИЗМЕНЕНИЕ: Всегда сэмплируем исходную текстуру с ручной фильтрацией \n"
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

// Упрощенный, но максимально качественный шейдер для Irradiance IBL
const char* qualityIrradiancePixelShader =
	"#define PI 3.14159265359 \n"
	" \n"
	"// Ручное сэмплирование исходной кубмапы с межгранной фильтрацией \n"
	"float3 manualCubeSampleHighQuality(float3 dir) \n"
	"{ \n"
	"    float3 result = float3(0.0, 0.0, 0.0); \n"
	"    float totalWeight = 0.0; \n"
	"    \n"
	"    // Многократное сэмплирование для smooth переходов \n"
	"    for (int i = 0; i < 256; i++) \n"
	"    { \n"
	"        float2 randomOffset = float2( \n"
	"            (fmod(float(i) * 1.618034, 1.0) - 0.5) * 2.0 * 0.02, \n"
	"            (fmod(float(i) * 2.678234, 1.0) - 0.5) * 2.0 * 0.02 \n"
	"        ); \n"
	"        \n"
	"        float3 sampleDir = normalize(dir + float3(randomOffset.x, randomOffset.y, 0.0)); \n"
	"        result += texCUBE(s0, sampleDir).rgb; \n"
	"        totalWeight += 1.0; \n"
	"    } \n"
	"    \n"
	"    return result / totalWeight; \n"
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
	"    for(uint i = 0u; i < 131072; i++) { \n"
	"        float2 Xi = hammersley(i, 131072); \n"
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
	"            // ВАЖНОЕ ИЗМЕНЕНИЕ: Всегда сэмплируем исходную текстуру с ручной фильтрацией \n"
	"            float3 sampleColor = manualCubeSampleHighQuality(L); \n"
	"            \n"
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

int main()
{
	// Инициализация Direct3D
	LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferWidth = 1;
	d3dpp.BackBufferHeight = 1;

	LPDIRECT3DDEVICE9 device;
	d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetConsoleWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING,
					  &d3dpp, &device);

	// Создание генератора IBL
	IBLGenerator generator(device);

	// Загрузка исходной Cubemap текстуры
	if (SUCCEEDED(generator.LoadSourceCubeMap(L"skybox.dds")))
	{
		LPDIRECT3DCUBETEXTURE9 specularCubeMap, irradianceCubeMap;

		// Генерация Specular IBL с mip-уровнями
		generator.GenerateSpecularIBL(specularCubeMap, 512, 9);

		// Генерация Irradiance IBL без mip-уровней
		generator.GenerateIrradianceIBL(irradianceCubeMap, 32);

		// Сохранение результатов
		D3DXSaveTextureToFile(L"specular_ibl.dds", D3DXIFF_DDS, specularCubeMap, NULL);
		D3DXSaveTextureToFile(L"irradiance_ibl.dds", D3DXIFF_DDS, irradianceCubeMap, NULL);

		specularCubeMap->Release();
		irradianceCubeMap->Release();
	}

	device->Release();
	d3d->Release();
	return 0;
}
