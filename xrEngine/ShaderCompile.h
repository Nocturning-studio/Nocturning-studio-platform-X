#include <sys/stat.h>

// Вспомогательная функция для получения времени модификации файла
static time_t GetFileModTime(const char* filename)
{
	struct _stat fileStat;
	if (_stat(filename, &fileStat) == 0)
	{
		return fileStat.st_mtime;
	}
	return 0;
}

// Структура для хранения метаданных кеша (должна быть POD и выровнена)
#pragma pack(push, 1)
struct ShaderCacheMetadata
{
	u32 crc;
	time_t sourceModTime;
	u32 buildId;
};
#pragma pack(pop)

//----------------------------------------------------------------
class CShaderIncluder : public ID3DXInclude
{
  private:
	u32 counter = 0;
	static const u32 max_size = 64;		 // KB
	static const u32 max_guard_size = 1; // KB
	char data[max_size * 1024];

  public:
	HRESULT __stdcall Open(D3DXINCLUDE_TYPE type, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
	{
		bool shared = type == D3DXINC_SYSTEM;
		LPCSTR shaders_path = shared ? "shared\\" : ::Render->getShaderPath();

		static string_path full_path;
		sprintf_s(full_path, "%s%s", shaders_path, pName);

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
		*pBytes = strlen(data);

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

		// Показываем диалог об ошибке
		std::string dialogTitle = make_string("Shader File Error - %s.%s", _name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), errorMsg, false);

		// Возвращаем шейдер с NULL указателем вместо ассерта
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
#endif

#ifdef DEBUG_SHADER_COMPILATION
	print_macros(_macros);
#endif

	// compile and create
	HRESULT _hr =
		CompileShader(_name, ext, (LPCSTR)file->pointer(), file->length(), c_target, c_entry, macros, (T*&)sh);

	FS.r_close(file);

	// Если компиляция не удалась, все равно возвращаем шейдер (с NULL указателем)
	return sh;
}

#include <boost/crc.hpp>

template <typename T> HRESULT CResourceManager::ReadShaderCache(string_path name, T*& result, time_t sourceModTime)
{
	IReader* file = FS.r_open(name);
	HRESULT cache_result = E_FAIL;

	if (file && file->length() > sizeof(ShaderCacheMetadata))
	{
		// Читаем метаданные
		ShaderCacheMetadata metadata;
		file->r(&metadata, sizeof(ShaderCacheMetadata));

		// Отладочная информация
		Msg("* Cache check: buildId=%d (expected %d), modTime=%lld (expected %lld)", metadata.buildId, build_id,
			metadata.sourceModTime, sourceModTime);

		// Проверяем актуальность кеша
		if (metadata.buildId == build_id && metadata.sourceModTime == sourceModTime)
		{
			// Вычисляем CRC оставшихся данных (сам шейдер)
			u32 data_size = file->elapsed();
			boost::crc_32_type processor;
			processor.process_block(((char*)file->pointer()), ((char*)file->pointer()) + data_size);
			u32 const real_crc = processor.checksum();

			if (real_crc == metadata.crc)
			{
				Msg("* Cache CRC valid: %u", real_crc);
				cache_result = ReflectShader((DWORD*)(file->pointer()), data_size, result);
			}
			else
			{
				Msg("! Shader cache CRC mismatch for: %s (expected: %u, got: %u)", name, metadata.crc, real_crc);
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
	result->sh = ShaderTypeTraits<T>::D3DCreateShader(src, size);

	const void* pReflection = 0;
	HRESULT _hr = D3DXFindShaderComment(src, MAKEFOURCC('C', 'T', 'A', 'B'), &pReflection, NULL);

	if (SUCCEEDED(_hr) && pReflection)
	{
		result->constants.parse((void*)pReflection, ShaderTypeTraits<T>::GetShaderDest());
		return _hr;
	}

	return E_FAIL;
}

static CTimer shader_compilation_timer;
extern XRCORE_API u32 build_id;

#include <Windows.h> // Добавляем для MessageBox

// Вспомогательная функция для показа диалога с ошибкой
static void ShowShaderErrorDialog(const char* title, const std::string& message, bool isWarning = false)
{
	// В режиме MASTER_GOLD не показываем диалоги
#ifdef MASTER_GOLD
	return;
#else
	// Форматируем сообщение для диалога (убираем лишние символы форматирования)
	std::string dialogMessage = message;

	// Заменяем символы форматирования лога на более читаемые для диалога
	size_t pos = 0;
	while ((pos = dialogMessage.find("~", pos)) != std::string::npos)
	{
		dialogMessage.replace(pos, 1, "");
	}

	// Убираем лишние переносы в начале
	while (!dialogMessage.empty() && (dialogMessage[0] == '\n' || dialogMessage[0] == '!'))
	{
		dialogMessage.erase(0, 1);
	}

	// Обрезаем слишком длинные сообщения
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

	Msg("\n* Compiling shader with name: %s.%s", name, ext);

	shader_compilation_timer.Start();
#endif

	HRESULT _result = E_FAIL;
	string_path cache_dest;
	sprintf_s(cache_dest, sizeof cache_dest, "shaders_cache\\%s%s.%s\\%s%d.xrcache", ::Render->getShaderPath(), name,
			  ext, macros.get_name().c_str(), build_id);
	FS.update_path(cache_dest, "$app_data_root$", cache_dest);

	// Получаем время модификации исходного файла шейдера
	string_path source_file_path;
	sprintf_s(source_file_path, sizeof source_file_path, "%s%s.%s", ::Render->getShaderPath(), name, ext);
	FS.update_path(source_file_path, "$game_shaders$", source_file_path);
	time_t sourceModTime = GetFileModTime(source_file_path);

	Msg("* Source file: %s, mod time: %lld", source_file_path, sourceModTime);

	// Try to find and read cache file
	if (FS.exist(cache_dest) && bUseShaderCache)
	{
		_result = ReadShaderCache(cache_dest, result, sourceModTime);

		if (SUCCEEDED(_result))
		{
#ifndef MASTER_GOLD
			Msg("- Successfully found and applied compiled shader cache file");
#endif
			return _result;
		}
		else
		{
			Msg("! Cache file invalid or outdated for shader: %s", cache_dest);
		}
	}

	CShaderIncluder Includer;
	ID3DXBuffer* pShaderBuf = NULL;
	ID3DXBuffer* pErrorBuf = NULL;
	ID3DXConstantTable* pConstants = NULL;

	u32 flags = D3DXSHADER_PACKMATRIX_ROWMAJOR | D3DXSHADER_PREFER_FLOW_CONTROL | D3DXSHADER_OPTIMIZATION_LEVEL3
#ifndef MASTER_GOLD
	//| D3DXSHADER_DEBUG
#endif
		;

	_result = D3DXCompileShader(src, size, &macros.get_macros()[0], &Includer, entry, target, flags, &pShaderBuf,
								&pErrorBuf, &pConstants);

	if (SUCCEEDED(_result) && pShaderBuf)
	{
		if (bUseShaderCache)
		{
			IWriter* file = FS.w_open(cache_dest);
			if (file)
			{
				// Вычисляем CRC данных шейдера
				boost::crc_32_type processor;
				processor.process_block(pShaderBuf->GetBufferPointer(),
										((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize());
				u32 crc = processor.checksum();

				// Подготавливаем метаданные
				ShaderCacheMetadata metadata;
				metadata.crc = crc;
				metadata.sourceModTime = sourceModTime;
				metadata.buildId = build_id;

				Msg("* Writing cache metadata: crc=%u, modTime=%lld, buildId=%d", metadata.crc, metadata.sourceModTime,
					metadata.buildId);

				// Записываем метаданные и данные шейдера
				file->w(&metadata, sizeof(ShaderCacheMetadata));
				file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
				FS.w_close(file);

				Msg("* Cache written successfully: %s", cache_dest);
			}
			else
			{
				Msg("! Cannot write cache file: %s", cache_dest);
			}
		}

		_result = ReflectShader((DWORD*)pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize(), result);

		if (FAILED(_result))
		{
			Msg("! D3DReflectShader %s.%s hr == 0x%08x", name, ext, _result);
			// Не ассертим здесь - просто продолжаем с ошибкой
		}

		if (bDisassebleShader && SUCCEEDED(_result))
		{
			ID3DXBuffer* pDisasm = 0;
			if (SUCCEEDED(D3DXDisassembleShader((DWORD*)pShaderBuf->GetBufferPointer(), TRUE, 0, &pDisasm)))
			{
				string_path disasm_dest;
				sprintf_s(disasm_dest, sizeof disasm_dest, "shaders_disasm\\%s%s.%s\\%s.html",
						  ::Render->getShaderPath(), name, ext, macros.get_name().c_str());
				IWriter* W = FS.w_open("$app_data_root$", disasm_dest);
				if (W)
				{
					W->w(pDisasm->GetBufferPointer(), pDisasm->GetBufferSize());
					FS.w_close(W);
				}
				_RELEASE(pDisasm);
			}
		}

		_RELEASE(pShaderBuf);
	}

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

		// Показываем диалог с предупреждением
		std::string dialogTitle = make_string("Shader Warning - %s.%s", name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), warning, true);

		// Вместо CHECK_OR_EXIT просто логируем предупреждение
		if (bWarningsAsErrors)
		{
			Msg("! Warnings treated as errors for shader: %s.%s", name, ext);
			// Не выходим - продолжаем работу
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
		error =
			make_string("\n! Error:\n! Shader: %s\n! Type: %s\n! Target: %s\n! HRESULT: %s\n", name, ext, target, code);
#endif
		error += message;

		Log(error.c_str());
		FlushLog();

		// Показываем диалог с ошибкой
		std::string dialogTitle = make_string("Shader Compilation Error - %s.%s", name, ext);
		ShowShaderErrorDialog(dialogTitle.c_str(), error, false);

		// Вместо CHECK_OR_EXIT просто логируем ошибку и продолжаем
		if (!bErrorsAsWarnings)
		{
			Msg("! Shader compilation failed: %s.%s", name, ext);
			// Не выходим - возвращаем шейдер с NULL указателем
		}
		else
		{
			Msg("! Shader compilation failed (treated as warning): %s.%s", name, ext);
		}

		// Устанавливаем шейдер в NULL чтобы избежать использования битого шейдера
		if (result)
		{
			result->sh = NULL;
		}
	}
#ifndef MASTER_GOLD
	else
	{
		Msg("- Shader successfully compiled");
		Msg("- Compile time: %d ms", shader_compilation_timer.GetElapsed_ms());

		// Также можно показывать диалог об успешной компиляции (опционально)
		if (strstr(Core.Params, "-show_shader_success"))
		{
			std::string successMsg =
				make_string("Shader compiled successfully!\n\nShader: %s.%s\nTarget: %s\nTime: %d ms", name, ext,
							target, shader_compilation_timer.GetElapsed_ms());
			MessageBoxA(NULL, successMsg.c_str(), "Shader Compilation Success",
						MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
		}
	}
#endif

	if (pErrorBuf)
		_RELEASE(pErrorBuf);
	if (pConstants)
		_RELEASE(pConstants);

	return _result;
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