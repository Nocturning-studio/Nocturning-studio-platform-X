#define _CRT_SECURE_NO_WARNINGS
#include <DXSDK/d3d9.h>
#include <DXSDK/d3dx9.h>
#include <string>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

// Шейдер для конвертации XYZ нормалей в карту высот
const char* heightMapPixelShaderbuff = "sampler2D s0; \n"
									   "float4 main(float2 texCoord : TEXCOORD0) : COLOR { \n"
									   "    float4 color = tex2D(s0, texCoord); \n"
									   "    // Получаем нормаль из RGB каналов (предполагается [-1,1] диапазон) \n"
									   "    float3 normal = color.rgb * 2.0 - 1.0; \n"
									   "    // Нормализуем на случай неточностей \n"
									   "    normal = normalize(normal); \n"
									   "    // Используем Z-компонент нормали как высоту \n"
									   "    // В нормалях Z обычно указывает вверх"
									   ", поэтому это хорошее приближение \n"
									   "    float height = normal.z; \n"
									   "    // Преобразуем из [-1,1] в [0,1] для grayscale \n"
									   "    height = height * 0.5 + 0.5; \n"
									   "    // Возвращаем как grayscale в L8 формате \n"
									   "    return float4(height, height, height, 1.0); \n"
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

// Глобальные переменные для логгирования
std::wofstream logFile;
bool logInitialized = false;

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

std::wstring GetCurrentDateTimeString()
{
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	std::wstringstream ss;
	ss << std::put_time(std::localtime(&time_t), L"%Y-%m-%d %H:%M:%S");
	return ss.str();
}

// Функция для замены _normal на _displacement в имени файла
std::wstring ReplaceNormalWithDisplacement(const std::wstring& fileName)
{
	std::wstring result = fileName;

	// Ищем суффикс _normal в имени файла (без учета регистра)
	std::wstring lowerFileName = fileName;
	std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

	size_t normalPos = lowerFileName.find(L"_normal");
	if (normalPos != std::wstring::npos)
	{
		// Заменяем _normal на _displacement
		result = result.substr(0, normalPos) + L"_displacement" + result.substr(normalPos + 7);
	}
	else
	{
		// Если суффикс _normal не найден, просто добавляем _displacement перед расширением
		size_t lastDot = result.find_last_of(L".");
		if (lastDot != std::wstring::npos)
		{
			result = result.substr(0, lastDot) + L"_displacement" + result.substr(lastDot);
		}
		else
		{
			result += L"_displacement";
		}
	}

	return result;
}

// Функции для логгирования
void InitializeLog()
{
	if (!logInitialized)
	{
		// Создаем папку logs если её нет
		CreateDirectory(L"logs", NULL);

		// Создаем имя файла лога с временной меткой
		auto now = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);
		std::wstringstream ss;
		ss << L"logs/heightmap_converter_" << std::put_time(std::localtime(&time_t), L"%Y%m%d_%H%M%S") << L".log";

		logFile.open(ss.str(), std::ios::out | std::ios::app);
		if (logFile.is_open())
		{
			logInitialized = true;
			logFile << L"[" << GetCurrentDateTimeString() << L"] Лог файл инициализирован" << std::endl;
			logFile << L"[" << GetCurrentDateTimeString() << L"] HeightMap Converter запущен" << std::endl;
		}
	}
}

void LogMessage(const std::wstring& message)
{
	std::wcout << L"[" << GetCurrentDateTimeString() << L"] " << message << std::endl;

	if (logInitialized && logFile.is_open())
	{
		logFile << L"[" << GetCurrentDateTimeString() << L"] " << message << std::endl;
		logFile.flush();
	}

	OutputDebugStringW((L"[" + GetCurrentDateTimeString() + L"] " + message + L"\n").c_str());
}

void LogError(const std::wstring& errorMessage)
{
	std::wcerr << L"[" << GetCurrentDateTimeString() << L"] ОШИБКА: " << errorMessage << std::endl;

	if (logInitialized && logFile.is_open())
	{
		logFile << L"[" << GetCurrentDateTimeString() << L"] ОШИБКА: " << errorMessage << std::endl;
		logFile.flush();
	}

	OutputDebugStringW((L"[" + GetCurrentDateTimeString() + L"] ОШИБКА: " + errorMessage + L"\n").c_str());
}

void CloseLog()
{
	if (logInitialized && logFile.is_open())
	{
		LogMessage(L"Завершение работы HeightMap Converter");
		logFile.close();
	}
}

class HeightMapConverter
{
  private:
	LPDIRECT3DDEVICE9 device;
	LPDIRECT3DTEXTURE9 sourceTexture;

	// Шейдеры
	LPDIRECT3DVERTEXSHADER9 vertexShader;
	LPDIRECT3DPIXELSHADER9 heightMapPixelShader;
	LPD3DXCONSTANTTABLE vertexShaderConstants;

	// Вершинный буфер для полноэкранного квада
	LPDIRECT3DVERTEXBUFFER9 vertexBuffer;

  public:
	HeightMapConverter(LPDIRECT3DDEVICE9 dev)
		: device(dev), vertexShader(NULL), heightMapPixelShader(NULL), vertexShaderConstants(NULL), vertexBuffer(NULL),
		  sourceTexture(NULL)
	{
		device->AddRef();
		LogMessage(L"Инициализация конвертера высот...");
		InitializeShaders();
		CreateFullscreenQuad();
		LogMessage(L"Конвертер высот успешно инициализирован");
	}

	~HeightMapConverter()
	{
		LogMessage(L"Очистка ресурсов конвертера...");
		if (sourceTexture)
			sourceTexture->Release();
		if (vertexShader)
			vertexShader->Release();
		if (heightMapPixelShader)
			heightMapPixelShader->Release();
		if (vertexShaderConstants)
			vertexShaderConstants->Release();
		if (vertexBuffer)
			vertexBuffer->Release();
		device->Release();
		LogMessage(L"Ресурсы конвертера очищены");
	}

  private:
	bool InitializeShaders()
	{
		LPD3DXBUFFER shaderBuffer = NULL;
		LPD3DXBUFFER errorBuffer = NULL;

		LogMessage(L"Компиляция вершинного шейдера...");
		// Компиляция вершинного шейдера
		HRESULT hr = D3DXCompileShader(simpleVertexShaderbuff, strlen(simpleVertexShaderbuff), NULL, NULL, "main",
									   "vs_3_0", 0, &shaderBuffer, &errorBuffer, &vertexShaderConstants);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				std::string errorMsg = (char*)errorBuffer->GetBufferPointer();
				LogError(L"Ошибка компиляции вершинного шейдера: " + std::wstring(errorMsg.begin(), errorMsg.end()));
				errorBuffer->Release();
			}
			else
			{
				LogError(L"Неизвестная ошибка компиляции вершинного шейдера");
			}
			return false;
		}

		hr = device->CreateVertexShader((DWORD*)shaderBuffer->GetBufferPointer(), &vertexShader);
		shaderBuffer->Release();
		if (FAILED(hr))
		{
			LogError(L"Ошибка создания вершинного шейдера");
			return false;
		}

		LogMessage(L"Компиляция пиксельного шейдера для генерации карты высот...");
		// Компиляция пиксельного шейдера для генерации карты высот
		hr = D3DXCompileShader(heightMapPixelShaderbuff, strlen(heightMapPixelShaderbuff), NULL, NULL, "main", "ps_3_0",
							   0, &shaderBuffer, &errorBuffer, NULL);
		if (FAILED(hr))
		{
			if (errorBuffer)
			{
				std::string errorMsg = (char*)errorBuffer->GetBufferPointer();
				LogError(L"Ошибка компиляции пиксельного шейдера: " + std::wstring(errorMsg.begin(), errorMsg.end()));
				errorBuffer->Release();
			}
			else
			{
				LogError(L"Неизвестная ошибка компиляции пиксельного шейдера");
			}
			return false;
		}

		hr = device->CreatePixelShader((DWORD*)shaderBuffer->GetBufferPointer(), &heightMapPixelShader);
		if (shaderBuffer)
			shaderBuffer->Release();
		if (errorBuffer)
			errorBuffer->Release();

		if (SUCCEEDED(hr))
		{
			LogMessage(L"Шейдеры успешно скомпилированы и созданы");
		}
		else
		{
			LogError(L"Ошибка создания пиксельного шейдера");
		}

		return SUCCEEDED(hr);
	}

	void CreateFullscreenQuad()
	{
		LogMessage(L"Создание полноэкранного квада...");
		// Создаем полноэкранный квад
		Vertex vertices[] = {
			// x, y, z, tu, tv
			{-1.0f, -1.0f, 0.0f, 0.0f, 1.0f}, // левый нижний
			{-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},  // левый верхний
			{1.0f, -1.0f, 0.0f, 1.0f, 1.0f},  // правый нижний
			{1.0f, 1.0f, 0.0f, 1.0f, 0.0f},	  // правый верхний
		};

		HRESULT hr = device->CreateVertexBuffer(4 * sizeof(Vertex), 0, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_DEFAULT,
												&vertexBuffer, NULL);

		if (SUCCEEDED(hr))
		{
			void* data;
			vertexBuffer->Lock(0, 0, &data, 0);
			memcpy(data, vertices, sizeof(vertices));
			vertexBuffer->Unlock();

			device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
			device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
			LogMessage(L"Полноэкранный квад успешно создан");
		}
		else
		{
			LogError(L"Ошибка создания вершинного буфера для полноэкранного квада");
		}
	}

	HRESULT ProcessHeightMapConversion(const wchar_t* outputFilename)
	{
		if (!sourceTexture)
		{
			LogError(L"Исходная текстура не загружена");
			return E_FAIL;
		}

		// Получаем размер исходной текстуры
		D3DSURFACE_DESC desc;
		sourceTexture->GetLevelDesc(0, &desc);

		LogMessage(L"Размер текстуры: " + std::to_wstring(desc.Width) + L"x" + std::to_wstring(desc.Height));
		LogMessage(L"Формат текстуры: " + std::to_wstring(desc.Format));

		// Создаем целевую текстуру в формате D3DFMT_L8
		LPDIRECT3DTEXTURE9 targetTexture = NULL;
		LogMessage(L"Создание целевой текстуры в формате D3DFMT_L8...");
		HRESULT hr = device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_L8,
										   D3DPOOL_DEFAULT, &targetTexture, NULL);

		if (FAILED(hr))
		{
			LogMessage(L"Не удалось создать текстуру в D3DFMT_L8, пробуем A8R8G8B8...");
			// Если не получилось создать текстуру в D3DFMT_L8, пробуем A8R8G8B8
			hr = device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
									   D3DPOOL_DEFAULT, &targetTexture, NULL);
			if (FAILED(hr))
			{
				LogError(L"Не удалось создать целевую текстуру");
				return hr;
			}
			else
			{
				LogMessage(L"Целевая текстура создана в формате D3DFMT_A8R8G8B8");
			}
		}
		else
		{
			LogMessage(L"Целевая текстура создана в формате D3DFMT_L8");
		}

		// Получаем поверхность рендер-таргета
		LPDIRECT3DSURFACE9 targetSurface = NULL;
		targetTexture->GetSurfaceLevel(0, &targetSurface);

		// Сохраняем старый рендер-таргет
		LPDIRECT3DSURFACE9 oldRenderTarget = NULL;
		device->GetRenderTarget(0, &oldRenderTarget);

		// Устанавливаем новый рендер-таргет
		device->SetRenderTarget(0, targetSurface);

		LogMessage(L"Настройка состояний рендеринга...");
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
		device->SetPixelShader(heightMapPixelShader);

		// Устанавливаем матрицу ортографической проекции
		D3DXMATRIX orthoMatrix;
		D3DXMatrixOrthoOffCenterLH(&orthoMatrix, -1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

		if (vertexShaderConstants)
		{
			D3DXHANDLE handle = vertexShaderConstants->GetConstantByName(NULL, "worldViewProj");
			vertexShaderConstants->SetMatrix(device, handle, &orthoMatrix);
		}

		LogMessage(L"Начало рендеринга...");
		// Очищаем и начинаем сцену
		device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
		device->BeginScene();

		// Рендерим полноэкранный квад
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

		device->EndScene();
		LogMessage(L"Рендеринг завершен");

		// Сохраняем результат в формате DDS
		LogMessage(L"Сохранение результата в файл: " + std::wstring(outputFilename));
		hr = D3DXSaveTextureToFile(outputFilename, D3DXIFF_DDS, targetTexture, NULL);

		if (SUCCEEDED(hr))
		{
			LogMessage(L"Карта высот успешно сохранена");
		}
		else
		{
			LogError(L"Ошибка сохранения карты высот. HRESULT: " + std::to_wstring(hr));
		}

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
		{
			sourceTexture->Release();
			sourceTexture = NULL;
		}

		LogMessage(L"Загрузка исходной текстуры нормалей: " + std::wstring(filename));
		// Загружаем текстуру из файла в исходном формате
		HRESULT hr =
			D3DXCreateTextureFromFileEx(device, filename, D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN,
										D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &sourceTexture);

		if (SUCCEEDED(hr))
		{
			LogMessage(L"Текстура нормалей успешно загружена");
		}
		else
		{
			LogError(L"Ошибка загрузки текстуры нормалей. HRESULT: " + std::to_wstring(hr));
		}

		return hr;
	}

	bool ConvertToHeightMap(const std::wstring& filePath)
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		LogMessage(L"=== НАЧАЛО ОБРАБОТКИ ===");
		LogMessage(L"Конвертация нормалей в карту высот: " + filePath);

		// Загрузка исходной текстуры нормалей
		if (FAILED(LoadSourceTexture(filePath.c_str())))
		{
			LogError(L"Не удалось загрузить текстуру нормалей: " + filePath);
			return false;
		}

		// Генерируем путь для сохранения карты высот
		std::wstring outputPath = GenerateHeightMapPath(filePath);
		LogMessage(L"Выходной путь: " + outputPath);

		// Обработка и сохранение карты высот
		LogMessage(L"Преобразование XYZ нормалей в карту высот...");
		LogMessage(L"Используется Z-компонент нормали как высота");

		if (FAILED(ProcessHeightMapConversion(outputPath.c_str())))
		{
			LogError(L"Не удалось сгенерировать карту высот");
			return false;
		}
		else
		{
			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

			LogMessage(L"Успешно сохранено: " + outputPath);
			LogMessage(L"Время обработки: " + std::to_wstring(duration.count()) + L" мс");
			LogMessage(L"=== ОБРАБОТКА ЗАВЕРШЕНА УСПЕШНО ===");
		}

		return true;
	}

  private:
	std::wstring GenerateHeightMapPath(const std::wstring& normalMapPath)
	{
		std::wstring directory = GetDirectory(normalMapPath);
		std::wstring fileName = GetFileNameWithoutExtension(normalMapPath);
		std::wstring extension = L".dds";

		// Заменяем _normal на _displacement
		std::wstring newFileName = ReplaceNormalWithDisplacement(fileName);

		return directory + newFileName + extension;
	}
};

// Функция для обработки одной текстуры
bool ProcessHeightMapFile(const std::wstring& filePath, HeightMapConverter& converter)
{
	return converter.ConvertToHeightMap(filePath);
}

// Оконная процедура
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		LogMessage(L"Получено сообщение WM_DESTROY - завершение работы");
		PostQuitMessage(0);
		return 0;

	case WM_DROPFILES: {
		HDROP hDrop = (HDROP)wParam;
		UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

		HeightMapConverter* converter = (HeightMapConverter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (converter && fileCount > 0)
		{
			LogMessage(L"Обнаружено файлов для обработки: " + std::to_wstring(fileCount));

			int successCount = 0;
			int failCount = 0;

			for (UINT i = 0; i < fileCount; i++)
			{
				wchar_t filePath[MAX_PATH];
				DragQueryFile(hDrop, i, filePath, MAX_PATH);

				// Проверяем расширение файла
				std::wstring ext = GetFileExtension(filePath);
				std::wstring lowerExt = ext;
				std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

				if (lowerExt == L"dds")
				{
					LogMessage(L"Обработка файла " + std::to_wstring(i + 1) + L" из " + std::to_wstring(fileCount) +
							   L": " + std::wstring(filePath));
					if (ProcessHeightMapFile(filePath, *converter))
					{
						successCount++;
					}
					else
					{
						failCount++;
					}
				}
				else
				{
					LogMessage(L"Пропуск неподдерживаемого файла (только DDS): " + std::wstring(filePath));
					failCount++;
				}
			}

			LogMessage(L"Обработка всех файлов завершена. Успешно: " + std::to_wstring(successCount) +
					   L", С ошибками: " + std::to_wstring(failCount));

			// Показываем сообщение в окне
			std::wstring resultMsg = L"Обработка завершена!\nУспешно: " + std::to_wstring(successCount) +
									 L"\nС ошибками: " + std::to_wstring(failCount);
			MessageBox(hwnd, resultMsg.c_str(), L"Результат обработки", MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			LogError(L"Конвертер не инициализирован или файлы не найдены");
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
				 L"Перетащите DDS текстуры нормалей (XYZ) в это окно\n\n"
				 L"Текстуры будут конвертированы из нормалей в карты высот:\n"
				 L"- Используется Z-компонент нормали как высота\n"
				 L"- Формат сохраняется как D3DFMT_L8 (8-bit grayscale)\n"
				 L"- Файлы сохраняются с суффиксом _displacement\n\n"
				 L"Поддерживаются только DDS файлы нормалей\n\n"
				 L"Прогресс обработки отображается в консоли и записывается в лог-файл",
				 -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

		EndPaint(hwnd, &ps);
		return 0;
	}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Функция для создания консоли
void CreateConsole()
{
	AllocConsole();

	// Перенаправляем стандартные потоки
	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

	// Устанавливаем заголовок консоли
	SetConsoleTitle(L"HeightMap Converter - Console");

	// Увеличиваем буфер консоли для прокрутки
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	consoleInfo.dwSize.Y = 1000; // Большой буфер для логов
	SetConsoleScreenBufferSize(hConsole, consoleInfo.dwSize);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Создаем консоль для логгирования
	CreateConsole();

	// Инициализируем логгирование
	InitializeLog();

	LogMessage(L"=== ЗАПУСК HEIGHTMAP CONVERTER ===");
	LogMessage(L"Версия: 1.0 (конвертер нормалей в карты высот)");
	LogMessage(L"Аргументы командной строки: " +
			   (lpCmdLine ? std::wstring(lpCmdLine, lpCmdLine + strlen(lpCmdLine)) : L"нет"));

	// Регистрация класса окна
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"HeightMapConverterWindow";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClass(&wc);

	// Создание окна
	HWND hwnd = CreateWindowEx(0, L"HeightMapConverterWindow", L"HeightMap Converter - Normal to Displacement (v1.0)",
							   WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL,
							   hInstance, NULL);

	if (!hwnd)
	{
		LogError(L"Не удалось создать окно");
		CloseLog();
		return -1;
	}

	LogMessage(L"Главное окно создано успешно");

	// Включаем поддержку drag and drop
	DragAcceptFiles(hwnd, TRUE);
	LogMessage(L"Drag and drop поддержка включена");

	// Инициализация Direct3D
	LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d)
	{
		LogError(L"Не удалось инициализировать Direct3D 9");
		MessageBox(hwnd, L"Не удалось инициализировать Direct3D 9", L"Ошибка", MB_ICONERROR);
		CloseLog();
		return -1;
	}

	LogMessage(L"Direct3D 9 инициализирован успешно");

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
		LogError(L"Не удалось создать устройство Direct3D. HRESULT: " + std::to_wstring(hr));
		MessageBox(hwnd, L"Не удалось создать устройство Direct3D", L"Ошибка", MB_ICONERROR);
		d3d->Release();
		CloseLog();
		return -1;
	}

	LogMessage(L"Устройство Direct3D создано успешно");

	// Создание конвертера высот и сохранение указателя в пользовательских данных окна
	HeightMapConverter* converter = new HeightMapConverter(device);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)converter);

	// Вывод инструкции
	std::wcout << L"\n";
	std::wcout << L"==============================================" << std::endl;
	std::wcout << L"   HeightMap Converter v1.0" << std::endl;
	std::wcout << L"   Normal to Displacement Map Converter" << std::endl;
	std::wcout << L"==============================================" << std::endl;
	std::wcout << L"Инструкция:" << std::endl;
	std::wcout << L"- Перетащите DDS текстуры нормалей в окно приложения" << std::endl;
	std::wcout << L"- Текстуры должны быть в формате XYZ нормалей" << std::endl;
	std::wcout << L"- Имена файлов должны содержать '_normal'" << std::endl;
	std::wcout << L"- Будут созданы карты высот с суффиксом '_displacement'" << std::endl;
	std::wcout << L"- Формат вывода: D3DFMT_L8 (8-bit grayscale)" << std::endl;
	std::wcout << L"- Подробный лог ведется в консоли и файле logs/heightmap_converter_*.log" << std::endl;
	std::wcout << L"==============================================" << std::endl;
	std::wcout << L"\n";

	LogMessage(L"Приложение готово к работе. Ожидание файлов...");

	// Цикл сообщений
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	LogMessage(L"Завершение работы приложения...");

	// Очистка ресурсов
	delete converter;
	device->Release();
	d3d->Release();

	CloseLog();

	return 0;
}
