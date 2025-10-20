#pragma once

#include "ShaderMacros.h"
#include <Windows.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <set>
#include <boost/crc.hpp>

// ��������������� ������� ��� ��������� ������� ����������� �����
static time_t GetFileModTime(const char* filename)
{
	string_path full_path;
	FS.update_path(full_path, "$game_shaders$", filename);

	struct _stat fileStat;
	if (_stat(full_path, &fileStat) == 0)
	{
		return fileStat.st_mtime;
	}

	// ���� �� ����� �� �������������� ����, ��������� ����������
	if (_stat(filename, &fileStat) == 0)
	{
		return fileStat.st_mtime;
	}

	Msg("! Cannot get mod time for file: %s", filename);
	return 0;
}

// ��������������� ������� ��� ������ ������� � �������
static void ShowShaderErrorDialog(const char* title, const std::string& message, bool isWarning = false)
{
	// � ������ MASTER_GOLD �� ���������� �������
#ifdef MASTER_GOLD
	return;
#else
	// ����������� ��������� ��� ������� (������� ������ ������� ��������������)
	std::string dialogMessage = message;

	// �������� ������� �������������� ���� �� ����� �������� ��� �������
	size_t pos = 0;
	while ((pos = dialogMessage.find("~", pos)) != std::string::npos)
	{
		dialogMessage.replace(pos, 1, "");
	}

	// ������� ������ �������� � ������
	while (!dialogMessage.empty() && (dialogMessage[0] == '\n' || dialogMessage[0] == '!'))
	{
		dialogMessage.erase(0, 1);
	}

	// �������� ������� ������ ���������
	if (dialogMessage.length() > 2000)
	{
		dialogMessage = dialogMessage.substr(0, 2000) + "\n\n... (message truncated)";
	}

	UINT type = MB_OK | MB_ICONERROR | MB_TOPMOST;
	if (isWarning)
	{
		type = MB_OK | MB_ICONWARNING | MB_TOPMOST;
	}

	MessageBoxA(NULL, dialogMessage.c_str(), title, type);
#endif
}

// ��������� ��� �������� ���������� ���� (������ ���� POD � ���������)
#pragma pack(push, 1)
struct ShaderCacheMetadata
{
	u32 crc;
	time_t sourceModTime;
	u32 buildId;
	u32 dependencyCount; // ���������� ������������
};
#pragma pack(pop)

struct ShaderDependencyInfo
{
	std::string filePath;
	time_t modTime;
	u32 crc;
};

// �������� ������������ ��������
class CShaderDependencyManager
{
  private:
	static const u32 DEPENDENCY_VERSION = 1;

  public:
	static bool WriteDependencyInfo(const std::string& cacheFilePath,
									const std::vector<ShaderDependencyInfo>& dependencies)
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Starting WriteDependencyInfo");
#endif

		// ���������� ������������ ���� � INI �����
		string_path iniPath;

		// �������� ������� ���� ��� ����������
		const char* xrcache_ext = strstr(cacheFilePath.c_str(), ".xrcache");
		if (xrcache_ext)
		{
			size_t base_len = xrcache_ext - cacheFilePath.c_str();
			if (base_len < sizeof(iniPath) - 5) // ��������� ����� ��� ".xrdep" + null
			{
				strncpy_s(iniPath, cacheFilePath.c_str(), base_len);
				strcat_s(iniPath, ".xrdep");
			}
			else
			{
				Msg("! [DEPS] Path too long: %s", cacheFilePath.c_str());
				return false;
			}
		}
		else
		{
			Msg("! [DEPS] Invalid cache file path (no .xrcache extension): %s", cacheFilePath.c_str());
			return false;
		}

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Writing dependency info to: %s", iniPath);
		Msg("* [DEPS] Number of dependencies to write: %d", dependencies.size());
#endif

		// ���������� ������������� ���� ��� ������
		IWriter* file = FS.w_open("$app_data_root$", iniPath);
		if (!file)
		{
			Msg("! [DEPS] Cannot open dependency file for writing: %s", iniPath);
			return false;
		}

		// ���������� ������
		file->w_u32(DEPENDENCY_VERSION);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Written version: %d", DEPENDENCY_VERSION);
#endif

		// ���������� ���������� ������������
		u32 depCount = (u32)dependencies.size();
		file->w_u32(depCount);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Written dependency count: %d", depCount);
#endif

		// ���������� ������ �����������
		for (u32 i = 0; i < depCount; i++)
		{
			const auto& dep = dependencies[i];

			// ����� ���� + ������
			u16 pathLen = (u16)dep.filePath.length();
			file->w_u16(pathLen);
			file->w(dep.filePath.c_str(), pathLen);
			file->w_u64((u64)dep.modTime);
			file->w_u32(dep.crc);

#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] Written dependency %d: %s (modTime: %lld, CRC: %u)", i, dep.filePath.c_str(),
				(long long)dep.modTime, dep.crc);
#endif
		}

		u32 fileSize = file->tell();
		FS.w_close(file);

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Successfully wrote dependency file: %s (%d bytes)", iniPath, fileSize);
#endif
		return true;
	}

	static bool ReadDependencyInfo(const std::string& cacheFilePath, std::vector<ShaderDependencyInfo>& dependencies)
	{
		string_path iniPath;
		strcpy_s(iniPath, cacheFilePath.c_str());

		// ���������� ������ ����������
		char* ext = strstr(iniPath, ".xrcache");
		if (ext)
		{
			// ��������� ������� ����� �������� � ������
			size_t remaining_size = sizeof(iniPath) - (ext - iniPath);
			if (remaining_size >= 6) // ".xrdep" + null terminator
			{
				strcpy_s(ext, remaining_size, ".xrdep");
			}
			else
			{
				Msg("! [DEPS] Not enough buffer space for extension replacement in ReadDependencyInfo");
				return false;
			}
		}
		else
		{
			// ���� �� ����� .xrcache, ��������� ��� ���� (��� �������� �������������)
#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] No .xrcache extension found, using path as-is: %s", iniPath);
#endif
		}

		// ���������� ������������� ���� ��� ������
		IReader* file = FS.r_open("$app_data_root$", iniPath);
		if (!file)
		{
#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] Cannot open dependency file for reading: %s", iniPath);
#endif
			return false;
		}

		if (file->length() < sizeof(u32) + sizeof(u32))
		{
			Msg("! [DEPS] Dependency file too small: %s (%d bytes)", iniPath, file->length());
			FS.r_close(file);
			return false;
		}

		// ������ ������
		u32 version = file->r_u32();
		if (version != DEPENDENCY_VERSION)
		{
			Msg("! [DEPS] Version mismatch in dependency file: %s (expected %d, got %d)", iniPath, DEPENDENCY_VERSION,
				version);
			FS.r_close(file);
			return false;
		}

		// ������ ���������� ������������
		u32 depCount = file->r_u32();
		dependencies.clear();

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Reading %d dependencies from: %s", depCount, iniPath);
#endif

		for (u32 i = 0; i < depCount; i++)
		{
			if (file->elapsed() < sizeof(u16))
			{
				Msg("! [DEPS] Not enough data for path length at dependency %d", i);
				break;
			}

			ShaderDependencyInfo dep;
			u16 pathLen = file->r_u16();

			if (file->elapsed() < pathLen + sizeof(u64) + sizeof(u32))
			{
				Msg("! [DEPS] Not enough data for dependency %d (pathLen: %d, elapsed: %d)", i, pathLen,
					file->elapsed());
				break;
			}

			// ������ ����
			char* pathBuf = (char*)alloca(pathLen + 1);
			file->r(pathBuf, pathLen);
			pathBuf[pathLen] = 0;
			dep.filePath = pathBuf;

			// ������ ����� ����������� � CRC
			dep.modTime = (time_t)file->r_u64();
			dep.crc = file->r_u32();

			dependencies.push_back(dep);

#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [DEPS] Read dependency %d: %s (modTime: %lld, CRC: %u)", i, dep.filePath.c_str(),
				(long long)dep.modTime, dep.crc);
#endif
		}

		FS.r_close(file);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [DEPS] Successfully read %d dependencies from: %s", dependencies.size(), iniPath);
#endif
		return true;
	}

	static bool AreDependenciesValid(const std::vector<ShaderDependencyInfo>& dependencies)
	{
		for (const auto& dep : dependencies)
		{
			string_path full_path;
			FS.update_path(full_path, "$game_shaders$", dep.filePath.c_str());

			time_t currentModTime = GetFileModTime(full_path);
			if (currentModTime == 0)
			{
				// ���� �� ������ - ������� ����������� ����������
				Msg("! Dependency file not found: %s", dep.filePath.c_str());
				return false;
			}

			if (currentModTime != dep.modTime)
			{
				Msg("! Dependency changed: %s (was: %lld, now: %lld)", dep.filePath.c_str(), (long long)dep.modTime,
					(long long)currentModTime);
				return false;
			}

			// �������������� �������� CRC ��� ������ ������
			if (ShouldCheckCRC(dep.filePath))
			{
				u32 currentCRC = CalculateFileCRC(dep.filePath.c_str());
				if (currentCRC != dep.crc)
				{
					Msg("! Dependency CRC mismatch: %s (was: %u, now: %u)", dep.filePath.c_str(), dep.crc, currentCRC);
					return false;
				}
			}
		}
		return true;
	}

	static u32 CalculateFileCRC(const char* filePath)
	{
		string_path full_path;
		FS.update_path(full_path, "$game_shaders$", filePath);

		IReader* file = FS.r_open(full_path);
		if (!file)
		{
			// ��������� ������� ��� ������������� ����
			file = FS.r_open("$game_shaders$", filePath);
			if (!file)
			{
				Msg("! Cannot open file for CRC calculation: %s", filePath);
				return 0;
			}
		}

		boost::crc_32_type processor;
		processor.process_block(file->pointer(), ((char*)file->pointer()) + file->length());
		u32 crc = processor.checksum();

		FS.r_close(file);
		return crc;
	}

	static bool ShouldCheckCRC(const std::string& filePath)
	{
		// ��������� CRC ������ ��� .h � .xrh ������ (��������� ���������)
		const char* ext = strrchr(filePath.c_str(), '.');
		return ext && (strcmp(ext, ".h") == 0 || strcmp(ext, ".hlsl") == 0 || strcmp(ext, ".xrh") == 0 ||
					   strcmp(ext, ".xrps") == 0 || strcmp(ext, ".xrvs") == 0);
	}
};

//----------------------------------------------------------------
class CShaderIncluder : public ID3DXInclude
{
  private:
	u32 counter = 0;
	static const u32 max_size = 64;
	static const u32 max_guard_size = 1;
	char data[max_size * 1024];

	// ������������ ������������
	std::vector<std::string> includedFiles;
	std::string currentShaderName;

  public:
	void SetCurrentShader(const char* shaderName)
	{
		currentShaderName = shaderName;
		includedFiles.clear();
	}

	const std::vector<std::string>& GetIncludedFiles() const
	{
		return includedFiles;
	}

	HRESULT __stdcall Open(D3DXINCLUDE_TYPE type, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
	{
		bool shared = type == D3DXINC_SYSTEM;
		LPCSTR shaders_path = shared ? "shared\\" : ::Render->getShaderPath();

		static string_path full_path;
		sprintf_s(full_path, "%s%s", shaders_path, pName);

		// ��������� ���� � ������ ������������
		includedFiles.push_back(full_path);

		IReader* R = FS.r_open("$game_shaders$", full_path);

		if (R == NULL)
			return E_FAIL;

		if (R->length() + 1 + max_guard_size >= max_size * 1024)
		{
			Msg("! max shader file size: %uKB", max_size);
			return E_FAIL;
		}

		static string_path hash;
		size_t i = 0;
		for (i = 0; i < strlen(full_path); i++)
		{
			hash[i] = (full_path[i] >= '0' && full_path[i] <= '9') || (full_path[i] >= 'a' && full_path[i] <= 'z') ||
							  (full_path[i] >= 'A' && full_path[i] <= 'Z')
						  ? full_path[i]
						  : '_';
		}
		hash[i] = 0;

		char* offset = data;

		sprintf(offset, "#ifndef _%s_included\n", hash);
		offset = strchr(offset, 0);

		memcpy(offset, R->pointer(), R->length());
		offset += R->length();

		sprintf(offset, "\n#define _%s_included\n", hash);
		offset = strchr(offset, 0);

		sprintf(offset, "\n#endif");
		offset = strchr(offset, 0);

		FS.r_close(R);

		*ppData = data;
		*pBytes = (UINT)strlen(data);

#ifdef DEBUG_SHADER_COMPILATION
		Msg("*   includer open: (id:%u): %s", counter, pName);
		Msg("FILE BEGIN");
		Log(data);
		Msg("FILE END");
		Msg("*   guard                  _%s_included", hash);
#endif

		counter++;

		return D3D_OK;
	}

	HRESULT __stdcall Close(LPCVOID pData)
	{
		return D3D_OK;
	}
};

//----------------------------------------------------------------
template <typename T> T* CResourceManager::RegisterShader(const char* _name)
{
	T* sh = xr_new<T>();
	sh->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();
	sh_map.insert(mk_pair(sh->set_name(_name), sh));
	return sh;
}

template <typename T> T* CResourceManager::FindShader(const char* _name)
{
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();
	ShaderTypeTraits<T>::Map_S::iterator I = sh_map.find(_name);

	if (I != sh_map.end())
		return I->second;

	// if the shader is not supported or not exist
	if (!ShaderTypeTraits<T>::IsSupported() ||
		(xr_strlen(_name) >= 4 && _name[0] == 'n' && _name[1] == 'u' && _name[2] == 'l' && _name[3] == 'l'))
	{
		T* sh = RegisterShader<T>("null");
		sh->sh = NULL;
		return sh;
	}

	return NULL;
}

#ifdef DEBUG_SHADER_COMPILATION
void print_macros(CShaderMacros& macros)
{
	if (macros.get_name().length() < 1)
		return;

	Msg("*   macro count: %d", macros.get_name().length());

	for (u32 i = 0; i < macros.get_impl().size(); ++i)
	{
		CShaderMacros::MacroImpl m = macros.get_impl()[i];
		Msg("*   macro: (%s : %s : %c)", m.Name, m.Definition, m.State);
	}
}
#endif

template <typename T> T* CResourceManager::CreateShader(const char* _name, CShaderMacros& _macros)
{
	// get shader macros
	CShaderMacros macros;
	macros.add(::Render->FetchShaderMacros());
	macros.add(_macros);
	macros.add(TRUE, NULL, NULL);

	// make the unique shader name
	string_path name;
	sprintf_s(name, sizeof name, "%s%s", _name, macros.get_name().c_str());

	// if the shader is already exist, return it
	T* sh = FindShader<T>(name);

	if (sh)
		return sh;

	// create a new shader
	sh = RegisterShader<T>(name);

	// open file
	string_path file_source;
	const char* ext = ShaderTypeTraits<T>::GetShaderExt();
	sprintf_s(file_source, sizeof file_source, "%s%s.%s", ::Render->getShaderPath(), _name, ext);
	FS.update_path(file_source, "$game_shaders$", file_source);
	IReader* file = FS.r_open(file_source);

	if (!file)
	{
		std::string errorMsg =
			make_string("Cannot open shader file:\n%s\n\nShader will not be available.", file_source);
		Msg("! %s", errorMsg.c_str());

		// ���������� ������ �� ������
		std::string dialogTitle = make_string("Shader File Error - %s.%s", _name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), errorMsg, false);

		// ���������� ������ � NULL ���������� ������ �������
		sh->sh = NULL;
		return sh;
	}

	// select target
	const char* type = ShaderTypeTraits<T>::GetShaderType();
	string32 c_target, c_entry;
	sprintf_s(c_entry, sizeof c_entry, "main");
	sprintf_s(c_target, sizeof c_target, "%s_%u_%u", type, HW.Caps.raster_major, HW.Caps.raster_minor);

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* Compiling shader: target=%s, source=%s.%s", c_target, _name, ext);
	print_macros(_macros);
#endif

	// compile and create
	HRESULT _hr =
		CompileShader(_name, ext, (LPCSTR)file->pointer(), file->length(), c_target, c_entry, macros, (T*&)sh);

	FS.r_close(file);

	// ���� ���������� �� �������, ��� ����� ���������� ������ (� NULL ����������)
	return sh;
}

template <typename T> HRESULT CResourceManager::ReadShaderCache(string_path name, T*& result, time_t sourceModTime)
{
	// ���������� ������������� ���� ��� ������
	IReader* file = FS.r_open("$app_data_root$", name);
	HRESULT cache_result = E_FAIL;

	if (file && file->length() > sizeof(ShaderCacheMetadata))
	{
		// ������ ����������
		ShaderCacheMetadata metadata;
		file->r(&metadata, sizeof(ShaderCacheMetadata));

#ifdef DEBUG_SHADER_COMPILATION
		// ���������� ����������
		Msg("* Cache check: buildId=%d (expected %d), modTime=%lld (expected %lld)", metadata.buildId, build_id,
			(long long)metadata.sourceModTime, (long long)sourceModTime);
#endif

		// ��������� ������������ ����
		if (metadata.buildId == build_id && metadata.sourceModTime == sourceModTime)
		{
			// ��������: ��������� ������� ������������ ��� �������
			bool bEnableDependencySystem = false;
#ifndef MASTER_GOLD
			bEnableDependencySystem = true; // strstr(Core.Params, "-use_shader_dependencies") != nullptr;
#endif

			// ��������� ����������� ������ ���� ������� ��������
			std::vector<ShaderDependencyInfo> dependencies;
			if (bEnableDependencySystem && CShaderDependencyManager::ReadDependencyInfo(name, dependencies))
			{
				if (CShaderDependencyManager::AreDependenciesValid(dependencies))
				{
					// ��������� CRC ���������� ������ (��� ������)
					u32 data_size = file->elapsed();
					boost::crc_32_type processor;
					processor.process_block(((char*)file->pointer()), ((char*)file->pointer()) + data_size);
					u32 const real_crc = processor.checksum();

					if (real_crc == metadata.crc)
					{
#ifdef DEBUG_SHADER_COMPILATION
						Msg("* Cache CRC valid: %u, all dependencies valid", real_crc);
#endif
						cache_result = ReflectShader((DWORD*)(file->pointer()), data_size, result);
					}
					else
					{
						Msg("! Shader cache CRC mismatch for: %s (expected: %u, got: %u)", name, metadata.crc,
							real_crc);
					}
				}
				else
				{
					Msg("! Dependencies changed for: %s", name);
				}
			}
			else
			{
				// ���������� �������� ������������
#ifdef DEBUG_SHADER_COMPILATION
				Msg("* Skipping dependency check (system disabled or no info)");
#endif
				// ��������� ������ CRC ��������� �������
				u32 data_size = file->elapsed();
				boost::crc_32_type processor;
				processor.process_block(((char*)file->pointer()), ((char*)file->pointer()) + data_size);
				u32 const real_crc = processor.checksum();

				if (real_crc == metadata.crc)
				{
#ifdef DEBUG_SHADER_COMPILATION
					Msg("* Cache CRC valid: %u", real_crc);
#endif
					cache_result = ReflectShader((DWORD*)(file->pointer()), data_size, result);
				}
				else
				{
					Msg("! Shader cache CRC mismatch for: %s (expected: %u, got: %u)", name, metadata.crc, real_crc);
				}
			}
		}
		else
		{
			if (metadata.buildId != build_id)
				Msg("! Shader cache build ID mismatch for: %s (expected: %d, got: %d)", name, build_id,
					metadata.buildId);
			else
				Msg("! Shader source file modified for: %s", name);
		}
	}
	else
	{
		if (!file)
			Msg("! Cannot open cache file: %s", name);
		else
			Msg("! Cache file too small: %s (%d bytes)", name, file->length());
	}

	if (file)
		FS.r_close(file);

	return cache_result;
}

template <typename T> HRESULT CResourceManager::ReflectShader(DWORD const* src, UINT size, T*& result)
{
#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [REFLECT] Starting ReflectShader");
#endif

	result->sh = ShaderTypeTraits<T>::D3DCreateShader(src, size);

	if (result->sh == NULL)
	{
		Msg("! [REFLECT] Failed to create shader");
		return E_FAIL;
	}

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [REFLECT] Shader created successfully");
#endif

	const void* pReflection = 0;
	HRESULT _hr = D3DXFindShaderComment(src, MAKEFOURCC('C', 'T', 'A', 'B'), &pReflection, NULL);

	if (SUCCEEDED(_hr) && pReflection)
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [REFLECT] Parsing constants...");
#endif
		result->constants.parse((void*)pReflection, ShaderTypeTraits<T>::GetShaderDest());
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [REFLECT] Constants parsed successfully");
#endif
		return _hr;
	}
	else
	{
		Msg("! [REFLECT] No reflection data found");
	}

	return E_FAIL;
}

static CTimer shader_compilation_timer;
extern XRCORE_API u32 build_id;

// ������� ��� �������� ����������
static bool EnsureCacheDirectoryExists(const char* cachePath)
{
	string_path directory;
	strcpy_s(directory, cachePath);

	// ������� ��������� ����
	char* lastSlash = strrchr(directory, '\\');
	if (!lastSlash)
		return true; // ��� ���������� ��� ��������

	*lastSlash = 0; // �������� �� ����������

	// ��������� ������������� � ������� ���� �����
	if (!FS.path_exist(directory))
	{
		Msg("* Creating cache directory: %s", directory);
		// if (!FS.create_path("$app_data_root$", directory))
		//{
		//	Msg("! Failed to create cache directory: %s", directory);
		//	return false;
		//}
	}

	return true;
}

template <typename T>
HRESULT CResourceManager::CompileShader(LPCSTR name, LPCSTR ext, LPCSTR src, UINT size, LPCSTR target, LPCSTR entry,
										CShaderMacros& macros, T*& result)
{
	bool bUseShaderCache = true;
	bool bDisassebleShader = true;
	bool bWarningsAsErrors = false;
	bool bErrorsAsWarnings = false;

#ifndef MASTER_GOLD
	bUseShaderCache = !strstr(Core.Params, "-do_not_use_shader_cache");
	bDisassebleShader = strstr(Core.Params, "-save_disassembled_shaders");
	bWarningsAsErrors = strstr(Core.Params, "-shader_warnings_as_errors");
	bErrorsAsWarnings = strstr(Core.Params, "-shader_errors_as_warnings");

#ifdef DEBUG_SHADER_COMPILATION
	Msg("\n* [COMPILE] Starting shader compilation: %s.%s", name, ext);
	shader_compilation_timer.Start();
#endif
#endif

	HRESULT _result = E_FAIL;

	// ��������� ������������� ���� � ���� (��� FS.update_path)
	string_path cache_dest;
	sprintf_s(cache_dest, sizeof cache_dest, "shaders_cache\\%s%s.%s\\%s%d.xrcache", ::Render->getShaderPath(), name,
			  ext, macros.get_name().c_str(), build_id);

	// �������� ����� ����������� ��������� ����� �������
	string_path source_file_path;
	sprintf_s(source_file_path, sizeof source_file_path, "%s%s.%s", ::Render->getShaderPath(), name, ext);
	FS.update_path(source_file_path, "$game_shaders$", source_file_path);
	time_t sourceModTime = GetFileModTime(source_file_path);

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] Source file: %s, mod time: %lld", source_file_path, (long long)sourceModTime);
	Msg("* [COMPILE] Cache file: %s", cache_dest);
#endif

	// Try to find and read cache file - ���������� ������������� ����
	if (FS.exist("$app_data_root$", cache_dest) && bUseShaderCache)
	{
		_result = ReadShaderCache(cache_dest, result, sourceModTime);

		if (SUCCEEDED(_result))
		{
#ifdef DEBUG_SHADER_COMPILATION
			Msg("- [COMPILE] Successfully found and applied compiled shader cache file");
#endif
			return _result;
		}
		else
		{
			Msg("! [COMPILE] Cache file invalid or outdated for shader: %s", cache_dest);
		}
	}

	CShaderIncluder Includer;
	ID3DXBuffer* pShaderBuf = NULL;
	ID3DXBuffer* pErrorBuf = NULL;
	ID3DXConstantTable* pConstants = NULL;

	// ������������� ������� ������ ��� includer'�
	Includer.SetCurrentShader(name);

	u32 flags = D3DXSHADER_PACKMATRIX_ROWMAJOR | D3DXSHADER_PREFER_FLOW_CONTROL | D3DXSHADER_OPTIMIZATION_LEVEL3
#ifndef MASTER_GOLD
	//| D3DXSHADER_DEBUG
#endif
		;

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] Calling D3DXCompileShader...");
#endif
	_result = D3DXCompileShader(src, size, &macros.get_macros()[0], &Includer, entry, target, flags, &pShaderBuf,
								&pErrorBuf, &pConstants);
#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] D3DXCompileShader completed with result: 0x%08x", _result);
#endif

	if (SUCCEEDED(_result) && pShaderBuf)
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Shader compilation successful, gathering dependencies...");
#endif

		// �������� ���������� � ������������ (� ��������� ����������)
		std::vector<ShaderDependencyInfo> dependencies;
		const auto& includedFiles = Includer.GetIncludedFiles();
		std::set<std::string> uniqueFiles; // ��� �������� ������������

		for (const auto& includedFile : includedFiles)
		{
			// ���������� ���������
			if (uniqueFiles.find(includedFile) != uniqueFiles.end())
				continue;

			uniqueFiles.insert(includedFile);

			ShaderDependencyInfo dep;
			dep.filePath = includedFile;

			// �������� ���������� ���� ��� ��������
			string_path check_path;
			FS.update_path(check_path, "$game_shaders$", includedFile.c_str());

			dep.modTime = GetFileModTime(check_path);
			if (dep.modTime == 0)
			{
				// ���� �� �����, ������� �������� ����
				dep.modTime = GetFileModTime(includedFile.c_str());
			}

			// ���� ��� ��� 0, ���� �� ������ - ���������� ��� ������������� ���� ������
			if (dep.modTime == 0)
			{
				Msg("! [COMPILE] Warning: Cannot get mod time for dependency: %s", includedFile.c_str());
				// ����������, �� ��� ���� �����������
				continue;
			}

			dep.crc = CShaderDependencyManager::CalculateFileCRC(includedFile.c_str());
			dependencies.push_back(dep);

#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [COMPILE] Dependency: %s (modTime: %lld, CRC: %u)", includedFile.c_str(), (long long)dep.modTime,
				dep.crc);
#endif
		}

		if (bUseShaderCache)
		{
#ifdef DEBUG_SHADER_COMPILATION
			Msg("* [COMPILE] Writing cache files...");
#endif

			// ������� ���������� ����� ������� �����
			if (!EnsureCacheDirectoryExists(cache_dest))
			{
				Msg("! [COMPILE] Failed to create cache directory for: %s", cache_dest);
			}
			else
			{
				// ���������� ������������� ���� ��� ������!
				IWriter* file = FS.w_open("$app_data_root$", cache_dest);
				if (file)
				{
					// ��������� CRC ������ �������
					boost::crc_32_type processor;
					processor.process_block(pShaderBuf->GetBufferPointer(),
											((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize());
					u32 crc = processor.checksum();

					// �������������� ���������� � ����������� � ������������
					ShaderCacheMetadata metadata;
					metadata.crc = crc;
					metadata.sourceModTime = sourceModTime;
					metadata.buildId = build_id;
					metadata.dependencyCount = (u32)dependencies.size();

#ifdef DEBUG_SHADER_COMPILATION
					Msg("* [COMPILE] Writing cache metadata with %d dependencies", dependencies.size());
#endif

					// ���������� ���������� � ������ �������
					file->w(&metadata, sizeof(ShaderCacheMetadata));
					file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
					FS.w_close(file);

#ifdef DEBUG_SHADER_COMPILATION
					Msg("* [COMPILE] Main cache file written successfully");
#endif

					// ���������� ���������� � ������������ � ��������� INI ����
#ifdef DEBUG_SHADER_COMPILATION
					Msg("* [COMPILE] Writing dependency info...");
#endif
					if (CShaderDependencyManager::WriteDependencyInfo(cache_dest, dependencies))
					{
#ifdef DEBUG_SHADER_COMPILATION
						Msg("* [COMPILE] Dependency file written successfully");
#endif
					}
					else
					{
						Msg("! [COMPILE] Failed to write dependency file");
					}
				}
				else
				{
					Msg("! [COMPILE] Cannot write cache file: %s", cache_dest);
				}
			}
		}

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Before ReflectShader");
#endif
		_result = ReflectShader((DWORD*)pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize(), result);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] After ReflectShader, result: 0x%08x", _result);
#endif

		if (FAILED(_result))
		{
			Msg("! [COMPILE] D3DReflectShader %s.%s hr == 0x%08x", name, ext, _result);
			// �� �������� ����� - ������ ���������� � �������
		}

		// ���������� ������������������� ������ �������
		if (bDisassebleShader && SUCCEEDED(_result))
		{
			ID3DXBuffer* pDisassembly = NULL;
			HRESULT disasmResult = D3DXDisassembleShader((DWORD*)pShaderBuf->GetBufferPointer(),
														 TRUE, // �� ���������� �����������
														 NULL, // �� ������������ �������������� �����������
														 &pDisassembly);

			if (SUCCEEDED(disasmResult) && pDisassembly)
			{
				// ��������� ���� ��� ���������� �������������������� �������
				string_path disasm_path;
				sprintf_s(disasm_path, sizeof disasm_path, "shaders_cache\\%s%s.%s\\%s%d.html",
						  ::Render->getShaderPath(), name, ext, macros.get_name().c_str(), build_id);

				// ������� ���������� ���� �����
				EnsureCacheDirectoryExists(disasm_path);

				// ��������� ������������������� ���
				IWriter* disasm_file = FS.w_open("$app_data_root$", disasm_path);
				if (disasm_file)
				{
					disasm_file->w(pDisassembly->GetBufferPointer(), (u32)pDisassembly->GetBufferSize());
					FS.w_close(disasm_file);
					Msg("* [COMPILE] Disassembled shader saved to: %s", disasm_path);
				}
				else
				{
					Msg("! [COMPILE] Failed to save disassembled shader: %s", disasm_path);
				}

				_RELEASE(pDisassembly);
			}
			else
			{
				Msg("! [COMPILE] Failed to disassemble shader: 0x%08x", disasmResult);
			}
		}

#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Releasing shader buffer");
#endif
		_RELEASE(pShaderBuf);
#ifdef DEBUG_SHADER_COMPILATION
		Msg("* [COMPILE] Shader buffer released");
#endif
	}
	else
	{
		Msg("! [COMPILE] Shader compilation failed or no shader buffer");
	}

	// ��������� ������ � ��������������
	string16 code;
	sprintf_s(code, sizeof code, "%08x", _result);
	std::string message = pErrorBuf ? std::string((char*)pErrorBuf->GetBufferPointer()) : "";

	if (pErrorBuf && !FAILED(_result) && message.length() > 0)
	{
		std::string warning = make_string("\n~ Warning:\n~ Shader: %s\n~ Type: %s\n~ Target: %s\n~ HRESULT: %s\n", name,
										  ext, target, code);

		warning += message;

		Log(warning.c_str());
		FlushLog();

		// ���������� ������ � ���������������
		std::string dialogTitle = make_string("Shader Warning - %s.%s", name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), warning, true);

		// ������ CHECK_OR_EXIT ������ �������� ��������������
		if (bWarningsAsErrors)
		{
			Msg("! [COMPILE] Warnings treated as errors for shader: %s.%s", name, ext);
		}
	}

	if (FAILED(_result))
	{
		std::string error = make_string("");

#ifdef MASTER_GOLD
		error = make_string("An error occurred during the shader compilation process.\nReport the problem to the "
							"developer, error:\nShader file: %s\nShader type: %s\nShader target: %s\nError: %s\n",
							name, ext, target, code);
#else
		error = make_string("\n! [COMPILE] Error:\n! Shader: %s\n! Type: %s\n! Target: %s\n! HRESULT: %s\n", name, ext,
							target, code);
#endif
		error += message;

		Log(error.c_str());
		FlushLog();

		// ���������� ������ � �������
		std::string dialogTitle = make_string("Shader Compilation Error - %s.%s", name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), error, false);

		// ������������� ������ � NULL ����� �������� ������������� ������ �������
		if (result)
		{
			result->sh = NULL;
		}
	}
#ifndef MASTER_GOLD
	else
	{
#ifdef DEBUG_SHADER_COMPILATION
		Msg("- [COMPILE] Shader successfully compiled");
		Msg("- [COMPILE] Compile time: %d ms", shader_compilation_timer.GetElapsed_ms());
#endif
	}
#endif

#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] Releasing DX buffers");
#endif
	if (pErrorBuf)
		_RELEASE(pErrorBuf);
	if (pConstants)
		_RELEASE(pConstants);
#ifdef DEBUG_SHADER_COMPILATION
	Msg("* [COMPILE] DX buffers released");
	Msg("* [COMPILE] CompileShader completed");
#endif

	return _result;
}

// �������������� ������� ��� ������ ������������
void CResourceManager::RecompileDependentShaders(const std::string& changedHeader)
{
	// ��� ������� ����� ���� ������� ��� ����������� ��������� header-�����
	// ��� ������ ��� �������, ������� ������� �� ����� header'� � ������� �� ��� ��� ����������

	string_path searchPattern;
	sprintf_s(searchPattern, "shaders_cache\\*\\*.xrcache.xrdep");

	FS_FileSet files;
	FS.file_list(files, "$app_data_root$", FS_ListFiles, searchPattern);

	int recompiledCount = 0;

	for (const auto& file : files)
	{
		// � XRay Engine FS_FileSet �������� ������ name, ���� ��������� ������������ $app_data_root$
		string_path iniPath;
		sprintf_s(iniPath, "shaders_cache\\%s", file.name.c_str());

		std::vector<ShaderDependencyInfo> dependencies;
		if (CShaderDependencyManager::ReadDependencyInfo(iniPath, dependencies))
		{
			for (const auto& dep : dependencies)
			{
				if (dep.filePath.find(changedHeader) != std::string::npos)
				{
					// ����� ������, ��������� �� ����������� header'�
					string_path cacheFile;
					strcpy_s(cacheFile, iniPath); // ���������� strcpy_s ������ ������������
					char* ext = strstr(cacheFile, ".xrdep");
					if (ext)
						*ext = 0; // ������� .xrdep

					Msg("! Marking shader as outdated due to header change: %s", cacheFile);
					// ������� ����� ����
					FS.file_delete("$app_data_root$", cacheFile);
					FS.file_delete("$app_data_root$", iniPath);
					recompiledCount++;
					break;
				}
			}
		}
	}

	if (recompiledCount > 0)
	{
		Msg("* Recompiled %d shaders due to change in: %s", recompiledCount, changedHeader.c_str());
	}
}

template <typename T> void CResourceManager::DestroyShader(const T* sh)
{
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();

	if (0 == (sh->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	LPSTR N = LPSTR(*sh->cName);
	typename ShaderTypeTraits<T>::Map_S::iterator I = sh_map.find(N);

	if (I != sh_map.end())
	{
		sh_map.erase(I);
		return;
	}
}