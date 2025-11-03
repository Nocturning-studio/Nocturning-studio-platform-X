#include <DXSDK/d3d9.h>
#include <DXSDK/d3dx9.h>
#include <vector>
#include <cmath>
#include <random>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// Усовершенствованные шейдеры с использованием HLSL Shader Model 3.0
const char* advancedVertexShader = "float4x4 worldViewProj; \n"
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

// Продвинутый шейдер для Specular IBL с GGX и importance sampling
const char* advancedSpecularPixelShader =
	"#define PI 3.14159265359 \n"
	"#define TWO_PI 6.28318530718 \n"
	"#define SAMPLE_COUNT 1024 \n"
	" \n"
	"float roughness; \n"
	"float resolution; \n"
	" \n"
	"// Функция для генерации случайных чисел \n"
	"float random(float2 uv) { \n"
	"    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453); \n"
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
	"    // Распределение в локальной системе координат \n"
	"    float phi = 2.0 * PI * Xi.x; \n"
	"    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y)); \n"
	"    float sinTheta = sqrt(1.0 - cosTheta * cosTheta); \n"
	"    \n"
	"    // Преобразование из сферических в декартовы координаты \n"
	"    float3 H; \n"
	"    H.x = cos(phi) * sinTheta; \n"
	"    H.y = sin(phi) * sinTheta; \n"
	"    H.z = cosTheta; \n"
	"    \n"
	"    // Преобразование в мировую систему координат \n"
	"    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0); \n"
	"    float3 tangent = normalize(cross(up, N)); \n"
	"    float3 bitangent = cross(N, tangent); \n"
	"    \n"
	"    return tangent * H.x + bitangent * H.y + N * H.z; \n"
	"} \n"
	" \n"
	"// Функция геометрии Smith для GGX \n"
	"float geometrySchlickGGX(float NdotV, float roughness) { \n"
	"    float r = (roughness + 1.0); \n"
	"    float k = (r * r) / 8.0; \n"
	"    float denom = NdotV * (1.0 - k) + k; \n"
	"    return NdotV / denom; \n"
	"} \n"
	" \n"
	"float geometrySmith(float NdotV, float NdotL, float roughness) { \n"
	"    float ggx2 = geometrySchlickGGX(NdotV, roughness); \n"
	"    float ggx1 = geometrySchlickGGX(NdotL, roughness); \n"
	"    return ggx1 * ggx2; \n"
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
	"    for(uint i = 0u; i < SAMPLE_COUNT; i++) { \n"
	"        // Генерация квази-случайной точки \n"
	"        float2 Xi = hammersley(i, SAMPLE_COUNT); \n"
	"        \n"
	"        // Importance sampling на основе GGX \n"
	"        float3 H = importanceSampleGGX(Xi, roughness, N); \n"
	"        float3 L = normalize(2.0 * dot(V, H) * H - V); \n"
	"        \n"
	"        float NdotL = max(dot(N, L), 0.0); \n"
	"        float NdotH = max(dot(N, H), 0.0); \n"
	"        float VdotH = max(dot(V, H), 0.0); \n"
	"        \n"
	"        if(NdotL > 0.0) { \n"
	"            // Вычисление D_GGX \n"
	"            float D = D_GGX(NdotH, roughness); \n"
	"            float pdf = D * NdotH / (4.0 * VdotH) + 0.0001; \n"
	"            \n"
	"            // Разрешение сэмпла для mip уровня \n"
	"            float saTexel = 4.0 * PI / (6.0 * resolution * resolution); \n"
	"            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001); \n"
	"            \n"
	"            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); \n"
	"            \n"
	"            // Сэмплирование cubemap с соответствующим mip уровнем \n"
	"            float3 sampleColor = texCUBElod(s0, float4(L, mipLevel)).rgb; \n"
	"            \n"
	"            prefilteredColor += sampleColor * NdotL; \n"
	"            totalWeight += NdotL; \n"
	"        } \n"
	"    } \n"
	"    \n"
	"    prefilteredColor = prefilteredColor / totalWeight; \n"
	"    return float4(prefilteredColor, 1.0); \n"
	"} \n";

// Продвинутый шейдер для Irradiance IBL со сферическими гармониками и Disney диффузом
const char* advancedIrradiancePixelShader =
	"#define PI 3.14159265359 \n"
	"#define SAMPLE_COUNT 2048 \n"
	" \n"
	"// Коэффициенты сферических гармоник (SH) \n"
	"float3 SHCoeffs[9]; \n"
	" \n"
	"// Функция для проекции на SH базис \n"
	"void projectToSH(float3 direction, float3 color, inout float3 coeffs[9]) { \n"
	"    float x = direction.x, y = direction.y, z = direction.z; \n"
	"    \n"
	"    // Band 0 \n"
	"    coeffs[0] += 0.282095 * color; \n"
	"    \n"
	"    // Band 1 \n"
	"    coeffs[1] += 0.488603 * color * y; \n"
	"    coeffs[2] += 0.488603 * color * z; \n"
	"    coeffs[3] += 0.488603 * color * x; \n"
	"    \n"
	"    // Band 2 \n"
	"    coeffs[4] += 1.092548 * color * x * y; \n"
	"    coeffs[5] += 1.092548 * color * y * z; \n"
	"    coeffs[6] += 0.315392 * color * (3.0 * z * z - 1.0); \n"
	"    coeffs[7] += 1.092548 * color * x * z; \n"
	"    coeffs[8] += 0.546274 * color * (x * x - y * y); \n"
	"} \n"
	" \n"
	"// Функция для реконструкции из SH \n"
	"float3 evaluateSH(float3 direction, float3 coeffs[9]) { \n"
	"    float x = direction.x, y = direction.y, z = direction.z; \n"
	"    \n"
	"    float3 result = \n"
	"        // Band 0 \n"
	"        0.282095 * coeffs[0] + \n"
	"        \n"
	"        // Band 1 \n"
	"        0.488603 * y * coeffs[1] + \n"
	"        0.488603 * z * coeffs[2] + \n"
	"        0.488603 * x * coeffs[3] + \n"
	"        \n"
	"        // Band 2 \n"
	"        1.092548 * x * y * coeffs[4] + \n"
	"        1.092548 * y * z * coeffs[5] + \n"
	"        0.315392 * (3.0 * z * z - 1.0) * coeffs[6] + \n"
	"        1.092548 * x * z * coeffs[7] + \n"
	"        0.546274 * (x * x - y * y) * coeffs[8]; \n"
	"    \n"
	"    return max(result, 0.0); \n"
	"} \n"
	" \n"
	"// Disney diffuse BRDF \n"
	"float disneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness) { \n"
	"    float FD90 = 0.5 + 2.0 * LdotH * LdotH * roughness; \n"
	"    float FdV = 1.0 + (FD90 - 1.0) * pow(1.0 - NdotV, 5.0); \n"
	"    float FdL = 1.0 + (FD90 - 1.0) * pow(1.0 - NdotL, 5.0); \n"
	"    return FdV * FdL / PI; \n"
	"} \n"
	" \n"
	"float4 main(float3 worldPos : TEXCOORD0, float3 normal : TEXCOORD1) : COLOR { \n"
	"    float3 N = normalize(normal); \n"
	"    \n"
	"    // Метод 1: Сферические гармоники \n"
	"    float3 shColor = evaluateSH(N, SHCoeffs); \n"
	"    \n"
	"    // Метод 2: Monte Carlo интеграция с Disney диффузом \n"
	"    float3 mcColor = float3(0.0, 0.0, 0.0); \n"
	"    float totalWeight = 0.0; \n"
	"    \n"
	"    for(uint i = 0u; i < SAMPLE_COUNT; i++) { \n"
	"        // Квази-случайное направление \n"
	"        float r1 = float(i) / float(SAMPLE_COUNT); \n"
	"        float r2 = frac(float(i) * 0.618033988749895); \n"
	"        \n"
	"        float phi = 2.0 * PI * r1; \n"
	"        float cosTheta = 1.0 - r2; \n"
	"        float sinTheta = sqrt(1.0 - cosTheta * cosTheta); \n"
	"        \n"
	"        float3 L = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta); \n"
	"        \n"
	"        // Преобразование в мировую систему координат \n"
	"        float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0); \n"
	"        float3 tangent = normalize(cross(up, N)); \n"
	"        float3 bitangent = cross(N, tangent); \n"
	"        L = tangent * L.x + bitangent * L.y + N * L.z; \n"
	"        \n"
	"        float NdotL = max(dot(N, L), 0.0); \n"
	"        float NdotV = max(dot(N, normalize(worldPos)), 0.0); \n"
	"        float LdotH = max(dot(L, normalize(L + normalize(worldPos))), 0.0); \n"
	"        \n"
	"        if(NdotL > 0.0) { \n"
	"            float3 sampleColor = texCUBE(s0, L).rgb; \n"
	"            float diffuse = disneyDiffuse(NdotV, NdotL, LdotH, 0.5); \n"
	"            \n"
	"            mcColor += sampleColor * diffuse * NdotL; \n"
	"            totalWeight += NdotL; \n"
	"        } \n"
	"    } \n"
	"    \n"
	"    mcColor = mcColor / totalWeight; \n"
	"    \n"
	"    // Комбинируем оба метода \n"
	"    float3 finalColor = 0.7 * shColor + 0.3 * mcColor; \n"
	"    \n"
	"    return float4(finalColor, 1.0); \n"
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

	// Матрицы обзора для 6 граней куба
	D3DXMATRIX viewMatrices[6] = {
		// +X
		D3DXMATRIX(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1),
		// -X
		D3DXMATRIX(0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1),
		// +Y
		D3DXMATRIX(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1),
		// -Y
		D3DXMATRIX(1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1),
		// +Z
		D3DXMATRIX(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1),
		// -Z
		D3DXMATRIX(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1)};

	// Предварительно вычисленные коэффициенты SH
	std::vector<D3DXVECTOR3> shCoefficients;

	bool InitializeShaders()
	{
		LPD3DXBUFFER shaderBuffer = NULL;
		LPD3DXBUFFER errorBuffer = NULL;

		// Компиляция вершинного шейдера
		HRESULT hr = D3DXCompileShader(advancedVertexShader, strlen(advancedVertexShader), NULL, NULL, "main", "vs_3_0",
									   0, &shaderBuffer, &errorBuffer, &vertexShaderConstants);
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
		hr = D3DXCompileShader(advancedSpecularPixelShader, strlen(advancedSpecularPixelShader), NULL, NULL, "main",
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
		hr = D3DXCompileShader(advancedIrradiancePixelShader, strlen(advancedIrradiancePixelShader), NULL, NULL, "main",
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

	void PrecomputeSHCoefficients(UINT sampleCount = 10000)
	{
		shCoefficients.resize(9, D3DXVECTOR3(0, 0, 0));

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dis(0.0f, 1.0f);

		for (UINT i = 0; i < sampleCount; ++i)
		{
			// Генерация равномерно распределенных точек на сфере
			float u1 = dis(gen);
			float u2 = dis(gen);

			float theta = 2.0f * D3DX_PI * u1;
			float phi = acos(2.0f * u2 - 1.0f);

			float x = sin(phi) * cos(theta);
			float y = sin(phi) * sin(theta);
			float z = cos(phi);

			D3DXVECTOR3 direction(x, y, z);

			// Сэмплирование cubemap
			D3DXVECTOR3 color = SampleCubeMap(direction);

			// Проекция на SH базис (упрощенная версия)
			// В реальной реализации нужно использовать полную проекцию
			float weight = 4.0f * D3DX_PI / sampleCount;

			// Band 0
			shCoefficients[0] += color * weight * 0.282095f;

			// Band 1
			shCoefficients[1] += color * weight * 0.488603f * y;
			shCoefficients[2] += color * weight * 0.488603f * z;
			shCoefficients[3] += color * weight * 0.488603f * x;

			// Band 2
			shCoefficients[4] += color * weight * 1.092548f * x * y;
			shCoefficients[5] += color * weight * 1.092548f * y * z;
			shCoefficients[6] += color * weight * 0.315392f * (3.0f * z * z - 1.0f);
			shCoefficients[7] += color * weight * 1.092548f * x * z;
			shCoefficients[8] += color * weight * 0.546274f * (x * x - y * y);
		}
	}

	D3DXVECTOR3 SampleCubeMap(const D3DXVECTOR3& direction)
	{
		// Упрощенная реализация сэмплирования cubemap
		// В реальной реализации нужно определить, какая грань cubemap используется
		// и выполнить билинейную фильтрацию
		D3DXVECTOR3 result(1.0f, 1.0f, 1.0f); // Заглушка
		return result;
	}

  public:
	IBLGenerator(LPDIRECT3DDEVICE9 dev)
		: device(dev), vertexShader(NULL), specularPixelShader(NULL), irradiancePixelShader(NULL),
		  vertexShaderConstants(NULL), specularPixelShaderConstants(NULL), irradiancePixelShaderConstants(NULL)
	{
		device->AddRef();
		InitializeShaders();
		PrecomputeSHCoefficients();
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

		// Установка коэффициентов SH
		SetSHCoefficients();

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
		// Вершины куба с нормалями
		Vertex vertices[] = {// Front face
							 {-1, -1, -1, 0, 0, -1},
							 {1, -1, -1, 0, 0, -1},
							 {1, 1, -1, 0, 0, -1},
							 {1, 1, -1, 0, 0, -1},
							 {-1, 1, -1, 0, 0, -1},
							 {-1, -1, -1, 0, 0, -1},
							 // Back face
							 {-1, -1, 1, 0, 0, 1},
							 {1, -1, 1, 0, 0, 1},
							 {1, 1, 1, 0, 0, 1},
							 {1, 1, 1, 0, 0, 1},
							 {-1, 1, 1, 0, 0, 1},
							 {-1, -1, 1, 0, 0, 1},
							 // Left face
							 {-1, -1, -1, -1, 0, 0},
							 {-1, 1, -1, -1, 0, 0},
							 {-1, 1, 1, -1, 0, 0},
							 {-1, 1, 1, -1, 0, 0},
							 {-1, -1, 1, -1, 0, 0},
							 {-1, -1, -1, -1, 0, 0},
							 // Right face
							 {1, -1, -1, 1, 0, 0},
							 {1, 1, -1, 1, 0, 0},
							 {1, 1, 1, 1, 0, 0},
							 {1, 1, 1, 1, 0, 0},
							 {1, -1, 1, 1, 0, 0},
							 {1, -1, -1, 1, 0, 0},
							 // Top face
							 {-1, 1, -1, 0, 1, 0},
							 {1, 1, -1, 0, 1, 0},
							 {1, 1, 1, 0, 1, 0},
							 {1, 1, 1, 0, 1, 0},
							 {-1, 1, 1, 0, 1, 0},
							 {-1, 1, -1, 0, 1, 0},
							 // Bottom face
							 {-1, -1, -1, 0, -1, 0},
							 {1, -1, -1, 0, -1, 0},
							 {1, -1, 1, 0, -1, 0},
							 {1, -1, 1, 0, -1, 0},
							 {-1, -1, 1, 0, -1, 0},
							 {-1, -1, -1, 0, -1, 0}};

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

		// Устанавливаем текстуру напрямую через устройство
		device->SetTexture(0, sourceCubeMap);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		// Устанавливаем параметры для пиксельного шейдера
		if (specularPixelShaderConstants)
		{
			// Устанавливаем roughness
			float roughness = (float)mip / (float)(totalMips - 1);
			D3DXHANDLE roughnessHandle = specularPixelShaderConstants->GetConstantByName(NULL, "roughness");
			specularPixelShaderConstants->SetFloat(device, roughnessHandle, roughness);

			// Устанавливаем разрешение
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

		// Устанавливаем текстуру напрямую через устройство
		device->SetTexture(0, sourceCubeMap);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		// Рендерим куб
		device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12);

		device->EndScene();
	}

	void SetSHCoefficients()
	{
		if (!irradiancePixelShaderConstants)
			return;

		// Создаем массив из 9 float3 для передачи в шейдер
		float shCoeffsData[9][3];
		for (int i = 0; i < 9; ++i)
		{
			shCoeffsData[i][0] = shCoefficients[i].x;
			shCoeffsData[i][1] = shCoefficients[i].y;
			shCoeffsData[i][2] = shCoefficients[i].z;
		}

		D3DXHANDLE shCoeffsHandle = irradiancePixelShaderConstants->GetConstantByName(NULL, "SHCoeffs");

		// Устанавливаем весь массив коэффициентов SH одним вызовом
		irradiancePixelShaderConstants->SetFloatArray(device, shCoeffsHandle, (float*)shCoeffsData, 9 * 3);
	}
};

// Пример использования
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