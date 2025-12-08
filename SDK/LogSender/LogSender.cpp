// LogSender.cpp
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <algorithm>

// --- НАСТРОЙКИ ---
const std::string GMAIL_TO = "nocturninglogs@gmail.com";

bool isDebugMode = false;

// --- ТИПЫ ФУНКЦИЙ ИЗ DLL ---
// Определяем, как выглядят функции внутри DLL, чтобы правильно их вызвать
typedef void (*GetCredsFunc)(char* buffer, int maxLen);

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ДЛЯ КРЕДОВ (чтобы заполнить их из DLL) ---
std::string g_GmailUser = "";
std::string g_GmailPass = "";

// --- ЗАГРУЗКА БИБЛИОТЕКИ ---
bool LoadCredentials(const std::wstring& exeDir)
{
	std::wstring dllPath = exeDir + L"\\AuthLib.dll";

	// Загружаем DLL
	HMODULE hDll = LoadLibraryW(dllPath.c_str());
	if (!hDll)
	{
		if (isDebugMode)
			std::wcout << L"[ERROR] Could not load AuthLib.dll at: " << dllPath << std::endl;
		return false;
	}

	// Получаем адреса функций
	GetCredsFunc getUserFunc = (GetCredsFunc)GetProcAddress(hDll, "GetUser");
	GetCredsFunc getPassFunc = (GetCredsFunc)GetProcAddress(hDll, "GetPass");

	if (!getUserFunc || !getPassFunc)
	{
		if (isDebugMode)
			std::cout << "[ERROR] DLL functions not found!" << std::endl;
		FreeLibrary(hDll);
		return false;
	}

	// Буферы для данных
	char userBuf[256] = {0};
	char passBuf[256] = {0};

	// Вызываем функции дешифровки
	getUserFunc(userBuf, 256);
	getPassFunc(passBuf, 256);

	// Сохраняем в string
	g_GmailUser = std::string(userBuf);
	g_GmailPass = std::string(passBuf);

	// Выгружаем DLL (безопасность)
	FreeLibrary(hDll);

	// Очищаем буферы в памяти (паранойя-левел)
	SecureZeroMemory(userBuf, sizeof(userBuf));
	SecureZeroMemory(passBuf, sizeof(passBuf));

	return true;
}

std::string WString_To_Utf8(const std::wstring& wstr)
{
	if (wstr.empty())
		return "";
	int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
	if (size <= 0)
		return "";
	std::vector<char> buffer(size);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], size, NULL, NULL);
	return std::string(buffer.data());
}

std::string AnsiFile_To_Utf8_String(const std::string& ansiContent)
{
	if (ansiContent.empty())
		return "";
	int wsize = MultiByteToWideChar(1251, 0, ansiContent.c_str(), -1, NULL, 0);
	if (wsize <= 0)
		return ansiContent;
	std::vector<wchar_t> wbuffer(wsize);
	MultiByteToWideChar(1251, 0, ansiContent.c_str(), -1, &wbuffer[0], wsize);
	return WString_To_Utf8(std::wstring(wbuffer.data()));
}

std::wstring Ansi_To_WString(const std::string& str)
{
	if (str.empty())
		return L"";
	int size = MultiByteToWideChar(1251, 0, str.c_str(), -1, NULL, 0);
	if (size <= 0)
		return L"";
	std::vector<wchar_t> buffer(size);
	MultiByteToWideChar(1251, 0, str.c_str(), -1, &buffer[0], size);
	return std::wstring(buffer.data());
}

// --- ФАЙЛОВАЯ СИСТЕМА ---

std::wstring GetExeDir()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	std::wstring path(buffer);
	return path.substr(0, path.find_last_of(L"\\/"));
}

std::wstring GetParentDir(std::wstring path)
{
	if (!path.empty() && (path.back() == L'\\' || path.back() == L'/'))
	{
		path.pop_back();
	}
	size_t found = path.find_last_of(L"\\/");
	if (found != std::wstring::npos)
	{
		return path.substr(0, found);
	}
	return path;
}

bool FileExists(const std::wstring& filepath)
{
	DWORD fileAttr = GetFileAttributesW(filepath.c_str());
	return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

std::string ReadFileRaw(const std::wstring& filepath)
{
	FILE* f;
	if (_wfopen_s(&f, filepath.c_str(), L"rb") != 0)
		return "";
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (fsize <= 0)
	{
		fclose(f);
		return "";
	}
	std::string content(fsize, 0);
	fread(&content[0], 1, fsize, f);
	fclose(f);
	return content;
}

// Функция для безопасного склеивания путей
std::wstring JoinPath(std::wstring part1, std::wstring part2)
{
	// Чистим конец первой части
	while (!part1.empty() && (part1.back() == L'\\' || part1.back() == L'/' || part1.back() == L' '))
	{
		part1.pop_back();
	}
	// Чистим начало второй части
	while (!part2.empty() && (part2.front() == L'\\' || part2.front() == L'/' || part2.front() == L' '))
	{
		part2.erase(0, 1);
	}

	if (part1.empty())
		return part2;
	if (part2.empty())
		return part1;

	return part1 + L"\\" + part2;
}

std::string ParseLtxValue(const std::string& content, const std::string& varName)
{
	size_t pos = content.find(varName);
	if (pos == std::string::npos)
		return "";

	size_t endLine = content.find('\n', pos);
	if (endLine == std::string::npos)
		endLine = content.length();
	std::string line = content.substr(pos, endLine - pos);

	size_t commentPos = line.find(';');
	if (commentPos != std::string::npos)
		line = line.substr(0, commentPos);

	size_t lastPipe = line.find_last_of('|');
	if (lastPipe != std::string::npos && lastPipe + 1 < line.length())
	{
		std::string value = line.substr(lastPipe + 1);
		// Trim пробелов
		const std::string whitespace = " \t\r\n";
		size_t first = value.find_first_not_of(whitespace);
		if (first == std::string::npos)
			return "";
		size_t last = value.find_last_not_of(whitespace);
		return value.substr(first, (last - first + 1));
	}
	return "";
}

// --- АВТОПОИСК ---
std::wstring AutoDiscoverLogPath(const std::wstring& exeDir)
{
	std::wstring dir = GetParentDir(exeDir);
	std::wstring gameFolder = GetParentDir(dir);

	if (isDebugMode)
		std::wcout << L"[AUTO] Game Root: " << gameFolder << std::endl;

	std::wstring ltxPath = JoinPath(gameFolder, L"game_filesystem.ltx");
	if (!FileExists(ltxPath))
	{
		if (isDebugMode)
			std::wcout << L"[AUTO-ERR] File not found: " << ltxPath << std::endl;
		return L"";
	}

	std::string ltxContent = ReadFileRaw(ltxPath);
	if (ltxContent.empty())
		return L"";

	std::string appDataRel = ParseLtxValue(ltxContent, "$app_data_root$");
	std::string logsRel = ParseLtxValue(ltxContent, "$logs$");

	if (appDataRel.empty() || logsRel.empty())
		return L"";

	// Склеиваем безопасным методом, который не допустит двойных слэшей
	std::wstring fullPath = gameFolder;
	fullPath = JoinPath(fullPath, Ansi_To_WString(appDataRel));
	fullPath = JoinPath(fullPath, Ansi_To_WString(logsRel));
	fullPath = JoinPath(fullPath, L"xray_engine.log");

	if (isDebugMode)
		std::wcout << L"[AUTO] Log Path: " << fullPath << std::endl;
	return fullPath;
}

// --- ЗАПУСК ---
int RunProcess(const std::wstring& cmdLine, bool showWindow)
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if (!showWindow)
	{
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	ZeroMemory(&pi, sizeof(pi));

	std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
	cmdBuffer.push_back(0);

	if (!CreateProcessW(NULL, &cmdBuffer[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		if (isDebugMode)
			std::cerr << "[ERROR] CreateProcess failed: " << GetLastError() << std::endl;
		return -1;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitCode = 0;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (int)exitCode;
}

void CreateDebugConsole()
{
	if (AllocConsole())
	{
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONIN$", "r", stdin);
	}
}

// --- MAIN ---
int wmain(int argc, wchar_t* argv[])
{
	std::wstring exeDir = GetExeDir();

	// Парсинг аргументов
	std::wstring inputW = L"";
	for (int i = 1; i < argc; i++)
	{
		std::wstring arg = argv[i];
		if (arg == L"--debug")
			isDebugMode = true;
		else
			inputW = arg;
	}

	if (isDebugMode)
		CreateDebugConsole();

	// --- ГЛАВНОЕ ИЗМЕНЕНИЕ: ЗАГРУЗКА ПАРОЛЕЙ ---
	if (!LoadCredentials(exeDir))
	{
		if (isDebugMode)
		{
			std::cout << "[FATAL ERROR] Cannot load credentials from AuthLib.dll." << std::endl;
			std::cout << "Make sure the DLL is in the same folder." << std::endl;
			system("pause");
		}
		return 1;
	}
	// ---------------------------------------------

	// Логика выбора файла (та же самая)
	if (inputW.empty())
	{
		std::wstring autoLog = AutoDiscoverLogPath(exeDir);
		if (!autoLog.empty() && FileExists(autoLog))
			inputW = autoLog;
		else
		{
			if (isDebugMode)
				system("pause");
			return 1;
		}
	}

	if (!FileExists(inputW))
		return 1;

	// Чтение и подготовка файла
	std::string rawContent = ReadFileRaw(inputW);
	std::string bodyUtf8 = AnsiFile_To_Utf8_String(rawContent);

	std::wstring curlPath = JoinPath(exeDir, L"curl.exe");
	std::wstring certPath = JoinPath(exeDir, L"cacert.pem");
	std::wstring tempFilePath = JoinPath(exeDir, L"mail_temp.txt");

	std::ofstream mailFile(tempFilePath, std::ios::binary);
	if (!mailFile.is_open())
		return 1;

	// ИСПОЛЬЗУЕМ g_GmailUser (загруженный из DLL)
	mailFile << "From: " << g_GmailUser << "\r\n";
	mailFile << "To: " << GMAIL_TO << "\r\n";
	mailFile << "Subject: Crash Report (X-Ray Log)\r\n";
	mailFile << "MIME-Version: 1.0\r\n";
	mailFile << "Content-Type: text/plain; charset=UTF-8\r\n";
	mailFile << "\r\n";
	mailFile << "\xEF\xBB\xBF";
	mailFile << bodyUtf8 << "\r\n";
	mailFile.close();

	// ФОРМИРОВАНИЕ КОМАНДЫ (Используем g_GmailUser и g_GmailPass)
	std::wstringstream cmd;
	cmd << L"\"" << curlPath << L"\" ";
	if (isDebugMode)
		cmd << L"-v ";

	// Формируем аргументы, используя переменные из DLL
	cmd << L"--user \"" << Ansi_To_WString(g_GmailUser) << L":" << Ansi_To_WString(g_GmailPass) << L"\" ";
	cmd << L"--url \"smtp://smtp.gmail.com:587\" ";
	cmd << L"--ssl-reqd ";
	cmd << L"--cacert \"" << certPath << L"\" ";
	cmd << L"--mail-from \"" << Ansi_To_WString(g_GmailUser) << L"\" ";
	cmd << L"--mail-rcpt \"" << Ansi_To_WString(GMAIL_TO) << L"\" ";
	cmd << L"--upload-file \"" << tempFilePath << L"\" ";
	cmd << L"--login-options AUTH=LOGIN ";

	// Запуск
	int result = RunProcess(cmd.str(), isDebugMode);

	_wremove(tempFilePath.c_str());

	// Важно: очищаем строки с паролем из памяти программы перед выходом
	g_GmailPass.assign(g_GmailPass.length(), '0');
	g_GmailUser.assign(g_GmailUser.length(), '0');

	if (isDebugMode)
	{
		if (result == 0)
			std::cout << "[SUCCESS]" << std::endl;
		else
			std::cout << "[ERROR] Code: " << result << std::endl;
		system("pause");
		FreeConsole();
	}

	return result;
}
