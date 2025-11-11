#pragma once

#include "ShaderMacros.h"

#include "DXSDK/d3dcompiler.h"
#include <DXSDK/DxErr.h>
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxerr.lib")

#include "ShaderIncluder.h"

#include <boost/crc.hpp>
#include "ResourceManager.h"

ENGINE_API bool g_shader_compiled = false;

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

	// fill props
	ShaderCompileElementProps<T> props;
	strcpy_s(props.name, _name); // shader name from blender
	strcpy_s(props.entry, "main");
	strcpy_s(props.ext, ShaderTypeTraits<T>::GetShaderExt());
	strcpy_s(props.reg_name, name); // shader unique reg name
	sprintf_s(props.target, sizeof props.target, "%s_%u_%u", 
		props.ext, HW.Caps.raster_major, HW.Caps.raster_minor);
	props.result = sh;
	props.macros.add(macros);

	// send to compiler
	CompileShader<T>(props);

	return sh;
}

template <typename T> 
void CResourceManager::CompileShader(ShaderCompileElementProps<T> props)
{
	// open file
	string_path file_source;
	sprintf_s(file_source, sizeof file_source, "%s%s.%s", 
		::Render->getShaderPath(), props.name, props.ext);
	FS.update_path(file_source, "$game_shaders$", file_source);
	IReader* file = FS.r_open(file_source);
	R_ASSERT2(file, file_source);

//#ifndef MASTER_GOLD
	Msg("* Compiling shader: target=%s, source=%s.%s", props.target, props.name, props.ext);
//#endif

//#ifdef DEBUG_SHADER_COMPILATION
	print_macros(props.macros);
//#endif

	string_path cache_dest;
	sprintf_s(cache_dest, sizeof cache_dest, "shaders_cache\\%s%s.%s\\%s", 
		::Render->getShaderPath(), props.name, props.ext, props.macros.get_name().c_str());
	FS.update_path(cache_dest, "$app_data_root$", cache_dest);

	// TODO: fix shader log name
	char name_ext[256];
	sprintf_s(name_ext, "%s.%s", props.reg_name, props.ext);

#ifndef MASTER_GOLD
	Msg("*   cache: %s.%s", cache_dest, props.ext);
#endif

	CShaderIncluder Includer;
	ID3DBlob* pShaderBuf = NULL;
	ID3DBlob* pErrorBuf = NULL;

	D3D_SHADER_MACRO* _macros = (D3D_SHADER_MACRO*)&props.macros.get_macros()[0];

	// u32 flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3;
	u32 flags =
		D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined DEBUG || defined RELEASE_NOOPT
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif

	HRESULT _result = D3DCompile(file->pointer(), file->length(), name_ext, _macros, 
		&Includer, props.entry, props.target, flags, 0, &pShaderBuf, &pErrorBuf);

	if (SUCCEEDED(_result))
	{
		IWriter* file = FS.w_open(cache_dest);

		boost::crc_32_type processor;
		processor.process_block(pShaderBuf->GetBufferPointer(),
								((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize());
		u32 const crc = processor.checksum();

		file->w_u32(crc);
		file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());
		FS.w_close(file);

		_result = ReflectShader((DWORD*)pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize(), props.result);

		if (FAILED(_result))
		{
			Msg("! D3DReflectShader %s.%s hr == 0x%08x", props.reg_name, props.ext, _result);
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
			sprintf_s(disasm_dest, sizeof disasm_dest, "shaders_disasm\\%s%s.%s\\%s.html", ::Render->getShaderPath(),
					  props.reg_name, props.ext, props.macros.get_name().c_str());
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

		std::string error = make_string("! Can't compile shader %s\nfile: %s.%s, target: %s\n", 
			code, props.reg_name, props.ext, props.target);
		error += message;

		Log(error.c_str());
		FlushLog();
		CHECK_OR_EXIT(!FAILED(_result), error);
	}

	FS.r_close(file);
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
