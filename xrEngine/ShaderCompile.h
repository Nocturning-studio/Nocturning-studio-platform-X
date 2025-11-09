#pragma once

#include "ShaderMacros.h"

#include "DXSDK/d3dcompiler.h"
#include <DXSDK/DxErr.h>
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxerr.lib")

ENGINE_API bool g_shader_compiled = false;

//----------------------------------------------------------------
class CShaderIncluder : public ID3DInclude
{
  private:
	u32 counter = 0;
	static const u32 max_size = 64; // KB
	static const u32 max_guard_size = 1; // KB
	char data[max_size * 1024];

  public:
	HRESULT __stdcall Open(D3D_INCLUDE_TYPE type, LPCSTR pName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
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
		int i = 0;
		for (i = 0; i < strlen(full_path); i++)
		{
			hash[i] = (full_path[i] >= '0' && full_path[i] <= '9')
				  ||  (full_path[i] >= 'a' && full_path[i] <= 'z')
				  ||  (full_path[i] >= 'A' && full_path[i] <= 'Z')
				? full_path[i] : '_';
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
template<typename T>
T* CResourceManager::RegisterShader(const char* _name)
{
	T* sh = xr_new<T>();
	sh->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();
	sh_map.insert(mk_pair(sh->set_name(_name), sh));
	return sh;
}

template<typename T>
T* CResourceManager::FindShader(const char* _name)
{
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();
	ShaderTypeTraits<T>::Map_S::iterator I = sh_map.find(_name);

	if (I != sh_map.end())
		return I->second;

	// if the shader is not supported or not exist
	if (!ShaderTypeTraits<T>::IsSupported() ||
		(xr_strlen(_name) >= 4 &&
		_name[0] == 'n' && _name[1] == 'u' && 
		_name[2] == 'l' && _name[3] == 'l'))
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

template<typename T>
T* CResourceManager::CreateShader(const char* _name, CShaderMacros& _macros)
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
	if (sh) return sh;

	// create a new shader
	sh = RegisterShader<T>(name);

	// open file
	string_path file_source;
	const char* ext = ShaderTypeTraits<T>::GetShaderExt();
	sprintf_s(file_source, sizeof file_source, "%s%s.%s", ::Render->getShaderPath(), _name, ext);
	FS.update_path(file_source, "$game_shaders$", file_source);
	IReader* file = FS.r_open(file_source);
	R_ASSERT2(file, file_source);

	// select target
	string32 c_entry;sprintf_s(c_entry, sizeof c_entry, "main");
	string32 c_target;
	sprintf_s(c_target, sizeof c_target, "%s_%u_%u", ext, HW.Caps.raster_major, HW.Caps.raster_minor);
	//ShaderTypeTraits<T>::GetShaderTarget(c_target);

//#ifndef MASTER_GOLD
	Msg("* Compiling shader: target=%s, source=%s.%s", c_target, _name, ext);
//#endif

//#ifdef DEBUG_SHADER_COMPILATION
	print_macros(_macros);
//#endif

	// compile and create
	HRESULT _hr = CompileShader(_name, ext, (LPCSTR)file->pointer(), file->length(), c_target, c_entry, macros, (T*&)sh);
	R_ASSERT(!FAILED(_hr));

	FS.r_close(file);

	return sh;
}

#include <boost/crc.hpp>

template<typename T>
HRESULT CResourceManager::CompileShader(
	LPCSTR			name,
	LPCSTR			ext,
	LPCSTR			src,
	UINT			size,
	LPCSTR			target,
	LPCSTR			entry,
	CShaderMacros&	macros,
	T*&				result)
{
	string_path cache_dest;
	sprintf_s(cache_dest, sizeof cache_dest, "shaders_cache\\%s%s.%s\\%s", ::Render->getShaderPath(), name, ext, macros.get_name().c_str());
	FS.update_path(cache_dest, "$app_data_root$", cache_dest);

	// TODO: fix shader log name
	char name_ext[256];
	sprintf_s(name_ext, "%s.%s", name, ext);

#ifndef MASTER_GOLD
	Msg("*   cache: %s.%s", cache_dest, ext);
#endif

	CShaderIncluder	Includer;
	ID3DBlob* pShaderBuf = NULL;
	ID3DBlob* pErrorBuf = NULL;

	D3D_SHADER_MACRO* _macros = (D3D_SHADER_MACRO*)&macros.get_macros()[0];
	
	//u32 flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3;
	u32 flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined DEBUG || defined RELEASE_NOOPT
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

	HRESULT _result = D3DCompile(src, size, name_ext, _macros, 
		&Includer, entry, target, flags, 0, &pShaderBuf, &pErrorBuf);
	
	if (SUCCEEDED(_result))
	{
		IWriter* file			= FS.w_open(cache_dest);

		boost::crc_32_type		processor;
		processor.process_block	( pShaderBuf->GetBufferPointer(), ((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize() );
		u32 const crc			= processor.checksum( );

		file->w_u32				(crc);
		file->w					(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
		FS.w_close				(file);

		_result					= ReflectShader((DWORD*)pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize(), result);

		if (FAILED(_result))
		{
			Msg("! D3DReflectShader %s.%s hr == 0x%08x", name, ext, _result);
			Msg("%s", DXGetErrorString(_result));
			R_ASSERT(NULL);
		}

#ifdef MASTER_GOLD
		bool disasm = strstr(Core.Params, "-disasm") ? true : false;

		if (disasm)
#endif
		{
			ID3DBlob* pDisasm = 0;
			u32 flags = D3D_DISASM_ENABLE_COLOR_CODE;
#if defined DEBUG || defined RELEASE_NOOPT
			flags |= D3D_DISASM_DISABLE_DEBUG_INFO | D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING;
#endif
			D3DDisassemble(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(), flags, "", &pDisasm);
			string_path disasm_dest;
			sprintf_s(disasm_dest, sizeof disasm_dest, "shaders_disasm\\%s%s.%s\\%s.html", 
				::Render->getShaderPath(), name, ext, macros.get_name().c_str());
			if (pDisasm)
			{
				IWriter* W = FS.w_open("$app_data_root$", disasm_dest);
				W->w(pDisasm->GetBufferPointer(), pDisasm->GetBufferSize());
				FS.w_close(W);
				_RELEASE(pDisasm);
			}
		}

		g_shader_compiled = true;
	}
	else
	{
		string16 code;
		sprintf_s(code, sizeof code, "hr=0x%08x", _result);

		std::string message = std::string(pErrorBuf ? (char*)pErrorBuf->GetBufferPointer() : "");

		std::string error = make_string("! Can't compile shader %s\nfile: %s.%s, target: %s\n", code, name, ext, target);
		error += message;

		Log(error.c_str());
		FlushLog();
		CHECK_OR_EXIT(!FAILED(_result), error);
	}

	return _result;
}

template<typename T>
HRESULT CResourceManager::ReflectShader(
	DWORD const*	src,
	UINT			size,
	T*&				result)
{
	result->sh = ShaderTypeTraits<T>::D3DCreateShader(src, size);

	ID3D11ShaderReflection* pReflection = 0;
	HRESULT const _hr = D3DReflect(src, size, IID_ID3D11ShaderReflection, (void**)&pReflection);
	Msg("*   reflect shader: %u", SUCCEEDED(_hr));

	if (SUCCEEDED(_hr) && pReflection)
	{
		result->constants.parse((void*)pReflection, ShaderTypeTraits<T>::GetShaderDest());
		CreateSignature(src, size, result);
		_RELEASE(pReflection);
		return _hr;
	}

	return E_FAIL;
}

template <typename T> void CResourceManager::CreateSignature(DWORD const* src, UINT size, T*& result)
{
}

template <> void CResourceManager::CreateSignature<SVS>(DWORD const* src, UINT size, SVS*& result)
{
	//	Store input signature (need only for VS)
	ID3DBlob* pSignatureBlob;
	HRESULT const _hr = D3DGetInputSignatureBlob(src, size, &pSignatureBlob);
	Msg("*   get signature shader: %u", SUCCEEDED(_hr));
	CHK_DX(_hr);
	VERIFY(pSignatureBlob);
	result->signature = _CreateInputSignature(pSignatureBlob);
}

template<typename T>
void CResourceManager::DestroyShader(const T* sh)
{
	ShaderTypeTraits<T>::Map_S& sh_map = GetShaderMap<ShaderTypeTraits<T>::Map_S>();

	if (0==(sh->dwFlags&xr_resource_flagged::RF_REGISTERED))
		return;

	LPSTR N = LPSTR(*sh->cName);
	typename ShaderTypeTraits<T>::Map_S::iterator I = sh_map.find(N);
		
	if (I!=sh_map.end())
	{
		sh_map.erase(I);
		return;
	}
}
