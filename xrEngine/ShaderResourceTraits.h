#pragma once

#include "ResourceManager.h"

template <typename T> struct ShaderTypeTraits;
/*
template <> struct ShaderTypeTraits<SVS>
{
	typedef CResourceManager::map_VS Map_S;
	typedef IDirect3DVertexShader9 ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_vertex;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "vs";
	}
	static inline BOOL IsSupported()
	{
		return TRUE;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
#pragma message(Reminder("Not implemented!"))
		// R_CHK(HW.pDevice->CreateVertexShader(buffer, &s));
		return s;
	}
};

template <> struct ShaderTypeTraits<SPS>
{
	typedef CResourceManager::map_PS Map_S;
	typedef IDirect3DPixelShader9 ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_pixel;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "ps";
	}
	static inline BOOL IsSupported()
	{
		return TRUE;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
#pragma message(Reminder("Not implemented!"))
		// R_CHK(HW.pDevice->CreatePixelShader(buffer, &s));
		return s;
	}
};
*/

template <> inline CResourceManager::map_VS& CResourceManager::GetShaderMap()
{
	return m_vs;
}
template <> inline CResourceManager::map_PS& CResourceManager::GetShaderMap()
{
	return m_ps;
}
template <> inline CResourceManager::map_CS& CResourceManager::GetShaderMap()
{
	return m_cs;
}
template <> inline CResourceManager::map_GS& CResourceManager::GetShaderMap()
{
	return m_gs;
}
template <> inline CResourceManager::map_HS& CResourceManager::GetShaderMap()
{
	return m_hs;
}
template <> inline CResourceManager::map_DS& CResourceManager::GetShaderMap()
{
	return m_ds;
}


template <> struct ShaderTypeTraits<SVS>
{
	typedef CResourceManager::map_VS Map_S;
	typedef ID3D11VertexShader ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_vertex;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "vs";
	}
	static inline void GetShaderTarget(string32 c_target)
	{
		sprintf_s(c_target, sizeof c_target, "vs_%u_%u", HW.Caps.raster_major, HW.Caps.raster_minor);
	}
	static inline BOOL IsSupported()
	{
		return HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_0;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
		R_CHK(HW.pDevice11->CreateVertexShader(buffer, size, NULL, &s));
		return s;
	}
};

template <> struct ShaderTypeTraits<SPS>
{
	typedef CResourceManager::map_PS Map_S;
	typedef ID3D11PixelShader ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_pixel;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "ps";
	}
	static inline void GetShaderTarget(string32 c_target)
	{
		sprintf_s(c_target, sizeof c_target, "ps_%u_%u", HW.Caps.raster_major, HW.Caps.raster_minor);
	}
	static inline BOOL IsSupported()
	{
		return HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_0;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
		R_CHK(HW.pDevice11->CreatePixelShader(buffer, size, NULL, &s));
		return s;
	}
};

template <> struct ShaderTypeTraits<SGS>
{
	typedef CResourceManager::map_GS Map_S;
	typedef ID3D11GeometryShader ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_pixel;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "gs";
	}
	static inline void GetShaderTarget(string32 c_target)
	{
		sprintf_s(c_target, sizeof c_target, "gs_%u_%u", HW.Caps.raster_major, HW.Caps.raster_minor);
	}
	static inline BOOL IsSupported()
	{
		return HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_0;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
		R_CHK(HW.pDevice11->CreateGeometryShader(buffer, size, NULL, &s));
		return s;
	}
};

template <> struct ShaderTypeTraits<SDS>
{
	typedef CResourceManager::map_DS Map_S;
	typedef ID3D11DomainShader ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_domain;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "ds";
	}
	static inline void GetShaderTarget(string32 c_target)
	{
		sprintf_s(c_target, sizeof c_target, "ds_%u_%u", HW.Caps.raster_major, HW.Caps.raster_minor);
	}
	static inline BOOL IsSupported()
	{
		return HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
		R_CHK(HW.pDevice11->CreateDomainShader(buffer, size, NULL, &s));
		return s;
	}
};

template <> struct ShaderTypeTraits<SHS>
{
	typedef CResourceManager::map_HS Map_S;
	typedef ID3D11HullShader ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_hull;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "hs";
	}
	static inline void GetShaderTarget(string32 c_target)
	{
		sprintf_s(c_target, sizeof c_target, "hs_%u_%u", HW.Caps.raster_major, HW.Caps.raster_minor);
	}
	static inline BOOL IsSupported()
	{
		return HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
		R_CHK(HW.pDevice11->CreateHullShader(buffer, size, NULL, &s));
		return s;
	}
};

template <> struct ShaderTypeTraits<SCS>
{
	typedef CResourceManager::map_CS Map_S;
	typedef ID3D11ComputeShader ID3DShader;

	static inline u32 GetShaderDest()
	{
		return RC_dest_compute;
	}
	static inline LPCSTR GetShaderExt()
	{
		return "cs";
	}
	static inline void GetShaderTarget(string32 c_target)
	{
		sprintf_s(c_target, sizeof c_target, "cs_%u_%u", HW.Caps.raster_major, HW.Caps.raster_minor);
	}
	static inline BOOL IsSupported()
	{
		return HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0 || HW.m_cs_support;
	}

	static inline ID3DShader* D3DCreateShader(DWORD const* buffer, size_t size)
	{
		ID3DShader* s = 0;
		R_CHK(HW.pDevice11->CreateComputeShader(buffer, size, NULL, &s));
		return s;
	}
};
