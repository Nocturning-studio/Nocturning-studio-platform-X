#pragma once

#include "DXSDK/d3dcompiler.h"

class CShaderIncluder : public ID3DInclude
{
  private:
	u32 counter = 0;
	static const u32 max_size = 64;		 // KB
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
