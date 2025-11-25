#include <DXSDK/d3d9.h>
#include <DXSDK/d3dx9.h>
#include <string>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <algorithm>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

// Шейдер для конвертации gloss в roughness (roughness = 1 - gloss)
const char* roughnessPixelShaderbuff = "sampler2D s0; \n"
								   "float4 main(float2 texCoord : TEXCOORD0) : COLOR { \n"
								   "    float4 color = tex2D(s0, texCoord); \n"
								   "    // Берем gloss из red канала и конвертируем в roughness \n"
								   "    float gloss = color.r; \n"
								   "    float roughness = pow(1.0f - gloss, 2.0f); \n"
								   "    // Сохраняем roughness в red канале, остальные каналы обнуляем \n"
								   "    return float4(roughness, 0.0, 0.0, 1.0); \n"
								   "} \n";

// Шейдер для создания metallic текстуры из gloss
const char* metallicPixelShaderbuff = "sampler2D s0; \n"
								  "float4 main(float2 texCoord : TEXCOORD0) : COLOR { \n"
								  "    float4 color = tex2D(s0, texCoord); \n"
								  "    // Берем gloss из red канала и конвертируем в metallic по PBR формулам \n"
								  "    float gloss = color.r; \n"
								  "    // Простая эвристика: высокий gloss часто коррелирует с низким metallic \n"
								  "    // Можно настроить эту формулу под конкретные нужды \n"
								  "    float metallic = 1.0f - gloss; \n"
								  "    // Сохраняем metallic в red канале, остальные каналы обнуляем \n"
								  "    return float4(metallic, 0.0, 0.0, 1.0); \n"
								  "} \n";

// Шейдер для создания normal текстуры из alpha и blue каналов
const char* normalPixelShaderbuff = "sampler2D s0; \n"
								"float4 main(float2 texCoord : TEXCOORD0) : COLOR { \n"
								"    float4 color = tex2D(s0, texCoord); \n"
								"    // Берем alpha и blue каналы для normal map \n"
								"    float alpha = color.a; \n"
								"    float blue = color.b; \n"
								"    float green = color.g; \n"
								"    return float4(alpha, blue, green, 1.0); \n"
								"} \n";

// Вершинный шейдер для полноэкранного квада
const char* simpleVertexShaderbuff = "float4x4 worldViewProj; \n"
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

struct Vertex
{
	float x, y, z;
	float tu, tv;
};

// Функции для работы с путями файлов
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

std::wstring GetFileExtension(const std::wstring& filePath)
{
	size_t lastDot = filePath.find_last_of(L".");
	if (lastDot == std::wstring::npos)
		return L"";
	return filePath.substr(lastDot + 1);
}

// Функция для замены суффикса _bump на указанный суффикс
std::wstring ReplaceBumpWithSuffix(const std::wstring& fileName, const std::wstring& newSuffix)
{
	std::wstring result = fileName;

	// Ищем суффикс _bump в имени файла (без учета регистра)
	std::wstring lowerFileName = fileName;
	std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

	size_t bumpPos = lowerFileName.find(L"_bump");
	if (bumpPos != std::wstring::npos)
	{
		// Заменяем _bump на новый суффикс
		result = result.substr(0, bumpPos) + newSuffix + result.substr(bumpPos + 5);
	}
	else
	{
		// Если суффикс _bump не найден, просто добавляем новый суффикс перед расширением
		size_t lastDot = result.find_last_of(L".");
		if (lastDot != std::wstring::npos)
		{
			result = result.substr(0, lastDot) + newSuffix + result.substr(lastDot);
		}
		else
		{
			result += newSuffix;
		}
	}

	return result;
}

class TextureProcessor
{
  private:
	LPDIRECT3DDEVICE9 device;
	LPDIRECT3DTEXTURE9 sourceTexture;

	// Шейдеры
	LPDIRECT3DVERTEXSHADER9 vertexShader;
	LPDIRECT3DPIXELSHADER9 roughnessPixelShader;
	LPDIRECT3DPIXELSHADER9 metallicPixelShader;
	LPDIRECT3DPIXELSHADER9 normalPixelShader;
	LPD3DXCONSTANTTABLE vertexShaderConstants;

	// Вершинный буфер для полноэкранного квада
	LPDIRECT3DVERTEXBUFFER9 vertexBuffer;

  public:
	TextureProcessor(LPDIRECT3DDEVICE9 dev)
		: device(dev), vertexShader(NULL), roughnessPixelShader(NULL), metallicPixelShader(NULL),
		  normalPixelShader(NULL), vertexShaderConstants(NULL), vertexBuffer(NULL), sourceTexture(NULL)
	{
		device->AddRef();
		InitializeShaders();
		CreateFullscreenQuad();
	}

	~TextureProcessor()
	{
		if (sourceTexture)
			sourceTexture->Release();
		if (vertexShader)
			vertexShader->Release();
		if (roughnessPixelShader)
			roughnessPixelShader->Release();
		if (metallicPixelShader)
			metallicPixelShader->Release();
		if (normalPixelShader)
			normalPixelShader->Release();
		if (vertexShaderConstants)
			vertexShaderConstants->Release();
		if (vertexBuffer)
			vertexBuffer->Release();
		device->Release();
	}

  private:
	bool InitializeShaders()
	{
		LPD3DXBUFFER shaderBuffer = NULL;
		LPD3DXBUFFER errorBuffer = NULL;

		// Компиляция вершинного шейдера
		HRESULT hr = D3DXCompileShader(simpleVertexShaderbuff, strlen(simpleVertexShaderbuff), NULL, NULL, "main", "vs_3_0", 0,
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

		// Компиляция пиксельного шейдера для roughness
		hr = D3DXCompileShader(roughnessPixelShaderbuff, strlen(roughnessPixelShaderbuff), NULL, NULL, "main", "ps_3_0", 0,
							   &shaderBuffer, &errorBuffer, NULL);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
				errorBuffer->Release();
			}
			return false;
		}
		hr = device->CreatePixelShader((DWORD*)shaderBuffer->GetBufferPointer(), &roughnessPixelShader);
		shaderBuffer->Release();

		// Компиляция пиксельного шейдера для metallic
		hr = D3DXCompileShader(metallicPixelShaderbuff, strlen(metallicPixelShaderbuff), NULL, NULL, "main", "ps_3_0", 0,
							   &shaderBuffer, &errorBuffer, NULL);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
				errorBuffer->Release();
			}
			return false;
		}
		hr = device->CreatePixelShader((DWORD*)shaderBuffer->GetBufferPointer(), &metallicPixelShader);
		shaderBuffer->Release();

		// Компиляция пиксельного шейдера для normal
		hr = D3DXCompileShader(normalPixelShaderbuff, strlen(normalPixelShaderbuff), NULL, NULL, "main", "ps_3_0", 0,
							   &shaderBuffer, &errorBuffer, NULL);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
				errorBuffer->Release();
			}
			return false;
		}
		hr = device->CreatePixelShader((DWORD*)shaderBuffer->GetBufferPointer(), &normalPixelShader);
		if (shaderBuffer)
			shaderBuffer->Release();
		if (errorBuffer)
			errorBuffer->Release();

		return SUCCEEDED(hr);
	}

	void CreateFullscreenQuad()
	{
		// Создаем полноэкранный квад
		Vertex vertices[] = {
			// x, y, z, tu, tv
			{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f}, // левый нижний
			{-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},  // левый верхний
			{1.0f, -1.0f, 0.0f, 1.0f, 1.0f},  // правый нижний
			{1.0f, 1.0f, 0.0f, 1.0f, 0.0f},	  // правый верхний
		};

		device->CreateVertexBuffer(4 * sizeof(Vertex), 0, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_DEFAULT, &vertexBuffer,
								   NULL);

		void* data;
		vertexBuffer->Lock(0, 0, &data, 0);
		memcpy(data, vertices, sizeof(vertices));
		vertexBuffer->Unlock();

		device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
		device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	}

	HRESULT ProcessWithShader(LPDIRECT3DPIXELSHADER9 shader, const wchar_t* outputFilename, D3DFORMAT format)
	{
		if (!sourceTexture)
			return E_FAIL;

		// Получаем размер исходной текстуры
		D3DSURFACE_DESC desc;
		sourceTexture->GetLevelDesc(0, &desc);

		// Создаем целевую текстуру в указанном формате
		LPDIRECT3DTEXTURE9 targetTexture = NULL;
		HRESULT hr = device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT,
										   &targetTexture, NULL);
		if (FAILED(hr))
		{
			// Если не получилось создать текстуру в нужном формате, пробуем A8R8G8B8
			hr = device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
									   D3DPOOL_DEFAULT, &targetTexture, NULL);
			if (FAILED(hr))
				return hr;
		}

		// Получаем поверхность рендер-таргета
		LPDIRECT3DSURFACE9 targetSurface = NULL;
		targetTexture->GetSurfaceLevel(0, &targetSurface);

		// Сохраняем старый рендер-таргет
		LPDIRECT3DSURFACE9 oldRenderTarget = NULL;
		device->GetRenderTarget(0, &oldRenderTarget);

		// Устанавливаем новый рендер-таргет
		device->SetRenderTarget(0, targetSurface);

		// Настройка состояний рендеринга
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// Настройка сэмплера
		device->SetTexture(0, sourceTexture);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		// Устанавливаем шейдеры
		device->SetVertexShader(vertexShader);
		device->SetPixelShader(shader);

		// Устанавливаем матрицу ортографической проекции
		D3DXMATRIX orthoMatrix;
		D3DXMatrixOrthoOffCenterLH(&orthoMatrix, -1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

		if (vertexShaderConstants)
		{
			D3DXHANDLE handle = vertexShaderConstants->GetConstantByName(NULL, "worldViewProj");
			vertexShaderConstants->SetMatrix(device, handle, &orthoMatrix);
		}

		// Очищаем и начинаем сцену
		device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
		device->BeginScene();

		// Рендерим полноэкранный квад
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

		device->EndScene();

		// Сохраняем результат в формате DDS
		hr = D3DXSaveTextureToFile(outputFilename, D3DXIFF_DDS, targetTexture, NULL);

		// Восстанавливаем состояния
		device->SetVertexShader(NULL);
		device->SetPixelShader(NULL);
		device->SetRenderTarget(0, oldRenderTarget);

		// Очистка
		targetSurface->Release();
		targetTexture->Release();
		oldRenderTarget->Release();

		return hr;
	}

  public:
	HRESULT LoadSourceTexture(const wchar_t* filename)
	{
		if (sourceTexture)
			sourceTexture->Release();

		// Загружаем текстуру из файла
		return D3DXCreateTextureFromFileEx(device, filename, D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_A8R8G8B8,
										   D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &sourceTexture);
	}

	bool ProcessAllTextures(const std::wstring& filePath)
	{
		wprintf(L"Обработка файла: %s\n", filePath.c_str());

		// Загрузка исходной текстуры
		if (FAILED(LoadSourceTexture(filePath.c_str())))
		{
			wprintf(L"Ошибка: Не удалось загрузить текстуру: %s\n", filePath.c_str());
			return false;
		}

		// Формирование путей для сохранения
		std::wstring baseName = GetFileNameWithoutExtension(filePath);
		std::wstring directory = GetDirectory(filePath);

		// Заменяем _bump на соответствующие суффиксы
		std::wstring roughnessName = ReplaceBumpWithSuffix(baseName, L"_roughness");
		std::wstring metallicName = ReplaceBumpWithSuffix(baseName, L"_metallic");
		std::wstring normalName = ReplaceBumpWithSuffix(baseName, L"_normal");

		std::wstring roughnessPath = directory + roughnessName + L".dds";
		std::wstring metallicPath = directory + metallicName + L".dds";
		std::wstring normalPath = directory + normalName + L".dds";

		bool success = true;

		// Обработка и сохранение roughness текстуры
		wprintf(L"Генерация roughness текстуры...\n");
		if (FAILED(ProcessWithShader(roughnessPixelShader, roughnessPath.c_str(), D3DFMT_L8)))
		{
			wprintf(L"Ошибка: Не удалось создать roughness текстуру\n");
			success = false;
		}
		else
		{
			wprintf(L"Сохранено: %s\n", roughnessPath.c_str());
		}

		// Обработка и сохранение metallic текстуры
		wprintf(L"Генерация metallic текстуры...\n");
		if (FAILED(ProcessWithShader(metallicPixelShader, metallicPath.c_str(), D3DFMT_L8)))
		{
			wprintf(L"Ошибка: Не удалось создать metallic текстуру\n");
			success = false;
		}
		else
		{
			wprintf(L"Сохранено: %s\n", metallicPath.c_str());
		}

		// Обработка и сохранение normal текстуры
		wprintf(L"Генерация normal текстуры...\n");
		if (FAILED(ProcessWithShader(normalPixelShader, normalPath.c_str(), D3DFMT_A8R8G8B8)))
		{
			wprintf(L"Ошибка: Не удалось создать normal текстуру\n");
			success = false;
		}
		else
		{
			wprintf(L"Сохранено: %s\n", normalPath.c_str());
		}

		if (success)
		{
			wprintf(L"Успешно обработан файл: %s\n\n", filePath.c_str());
		}
		else
		{
			wprintf(L"Обработка файла завершена с ошибками: %s\n\n", filePath.c_str());
		}

		return success;
	}
};

// Функция для обработки одной текстуры
bool ProcessTextureFile(const std::wstring& filePath, TextureProcessor& processor)
{
	return processor.ProcessAllTextures(filePath);
}

// Оконная процедура
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

		TextureProcessor* processor = (TextureProcessor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (processor && fileCount > 0)
		{
			wprintf(L"Обнаружено %d файлов для обработки:\n", fileCount);

			for (UINT i = 0; i < fileCount; i++)
			{
				wchar_t filePath[MAX_PATH];
				DragQueryFile(hDrop, i, filePath, MAX_PATH);

				// Проверяем расширение файла
				std::wstring ext = GetFileExtension(filePath);
				std::wstring supported[] = {L"jpg", L"jpeg", L"png", L"bmp", L"tga", L"dds"};
				bool isSupported = false;

				for (const auto& supportedExt : supported)
				{
					if (_wcsicmp(ext.c_str(), supportedExt.c_str()) == 0)
					{
						isSupported = true;
						break;
					}
				}

				if (isSupported)
				{
					ProcessTextureFile(filePath, *processor);
				}
				else
				{
					wprintf(L"Пропуск неподдерживаемого файла: %s\n", filePath);
				}
			}

			wprintf(L"Обработка всех файлов завершена!\n");
		}

		DragFinish(hDrop);
		return 0;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT rect;
		GetClientRect(hwnd, &rect);
		DrawText(hdc,
				 L"Перетащите bump текстуры (.jpg, .png, .bmp, .tga, .dds) в это окно\n\n"
				 L"Для каждой текстуры будут созданы:\n"
				 L"- roughness (из red канала) как D3DFMT_R8\n"
				 L"- metallic (PBR из red канала) как D3DFMT_R8\n"
				 L"- normal (из alpha и blue каналов) как D3DFMT_R8G8B8A8\n\n"
				 L"Файлы должны иметь суффикс _bump",
				 -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

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
	wc.lpszClassName = L"PBRTextureConverterWindow";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClass(&wc);

	// Создание окна
	HWND hwnd = CreateWindowEx(0, L"PBRTextureConverterWindow", L"PBR Texture Converter - Drag & Drop Tool",
							   WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 700, 500, NULL, NULL,
							   hInstance, NULL);

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

	// Создание процессора текстур и сохранение указателя в пользовательских данных окна
	TextureProcessor* processor = new TextureProcessor(device);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)processor);

	// Вывод инструкции
	wprintf(L"PBR Texture Converter запущен!\n");
	wprintf(L"Перетащите bump текстуры в окно для обработки.\n");
	wprintf(L"Для каждой текстуры будут созданы:\n");
	wprintf(L"  - roughness текстура (из red канала) в формате D3DFMT_R8\n");
	wprintf(L"  - metallic текстура (PBR из red канала) в формате D3DFMT_R8\n");
	wprintf(L"  - normal текстура (из alpha и blue каналов) в формате D3DFMT_R8G8\n");
	wprintf(L"Исходные файлы должны иметь суффикс _bump.\n\n");

	// Цикл сообщений
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Очистка ресурсов
	delete processor;
	device->Release();
	d3d->Release();

	return 0;
}
