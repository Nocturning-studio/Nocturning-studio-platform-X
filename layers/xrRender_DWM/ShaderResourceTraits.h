#pragma once

#	include "ResourceManager.h"



	template<typename T>
	struct ShaderTypeTraits;

	template<>
	struct ShaderTypeTraits<SVS>
	{
		typedef CResourceManager::map_VS	MapType;
		typedef ID3DVertexShader 			DXIface;

		static inline const char* GetShaderExt() {return ".vs";}

		static inline const char* GetCompilationTarget() 
		{
			switch (HW.FeatureLevel)
			{
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
			//	return "vs_5_1";
			//	break;

			case D3D_FEATURE_LEVEL_11_0:
			case D3D_FEATURE_LEVEL_11_1:
				return "vs_5_0";
				break;

			case D3D_FEATURE_LEVEL_10_1:
				return "vs_4_1";
				break;

			default: 
				return "vs_4_0"; 
			}
		}

		static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
		{
			DXIface* sh = 0;
			R_CHK(HW.pDevice->CreateVertexShader(buffer, size, NULL, &sh));
			return sh;
		}

		static inline u32 GetShaderDest() {return RC_dest_vertex;}
	};
	
	template<>
	struct ShaderTypeTraits<SPS>
	{
		typedef CResourceManager::map_PS	MapType;
		typedef ID3DPixelShader 			DXIface;

		static inline const char* GetShaderExt() {return ".ps";}

		static inline const char* GetCompilationTarget()
		{
			switch (HW.FeatureLevel)
			{
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
			//	return "ps_5_1";
			//	break;

			case D3D_FEATURE_LEVEL_11_0:
			case D3D_FEATURE_LEVEL_11_1:
				return "ps_5_0";
				break;

			case D3D_FEATURE_LEVEL_10_1:
				return "ps_4_1";
				break;

			default:
				return "ps_4_0";
			}
		}

		static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
		{
			DXIface* sh = 0;
			R_CHK(HW.pDevice->CreatePixelShader(buffer, size, NULL, &sh));
			return sh;
		}

		static inline u32 GetShaderDest() {return RC_dest_pixel;}
	};
	
	template<>
	struct ShaderTypeTraits<SGS>
	{
		typedef CResourceManager::map_GS	MapType;
		typedef ID3DGeometryShader 			DXIface;

		static inline const char* GetShaderExt() {return ".gs";}

		static inline const char* GetCompilationTarget()
		{
			switch (HW.FeatureLevel)
			{
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
			//	return "gs_5_1";
			//	break;

			case D3D_FEATURE_LEVEL_11_0:
			case D3D_FEATURE_LEVEL_11_1:
				return "gs_5_0";
				break;

			case D3D_FEATURE_LEVEL_10_1:
				return "gs_4_1";
				break;

			default:
				return "gs_4_0";
			}
		}

		static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
		{
			DXIface* sh = 0;
			R_CHK(HW.pDevice->CreateGeometryShader(buffer, size, NULL, &sh));
			return sh;
		}

		static inline u32 GetShaderDest() {return RC_dest_geometry;}
	};
	
	template<>
	struct ShaderTypeTraits<SHS>
	{
		typedef CResourceManager::map_HS	MapType;
		typedef ID3DHullShader 				DXIface;

		static inline const char* GetShaderExt() {return ".hs";}

		static inline const char* GetCompilationTarget()
		{
			switch (HW.FeatureLevel)
			{
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
			//	return "hs_5_1";
			//	break;

			default:
				return "hs_5_0";
			}
		}

		static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
		{
			DXIface* sh = 0;
			R_CHK(HW.pDevice->CreateHullShader(buffer, size, NULL, &sh));
			return sh;
		}

		static inline u32 GetShaderDest() {return RC_dest_hull;}
	};

	template<>
	struct ShaderTypeTraits<SDS>
	{
		typedef CResourceManager::map_DS	MapType;
		typedef ID3DDomainShader			DXIface;

		static inline const char* GetShaderExt() {return ".ds";}

		static inline const char* GetCompilationTarget()
		{
			switch (HW.FeatureLevel)
			{
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
			//	return "ds_5_1";
			//	break;

			default:
				return "ds_5_0";
			}
		}

		static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
		{
			DXIface* sh = 0;
			R_CHK(HW.pDevice->CreateDomainShader(buffer, size, NULL, &sh));
			return sh;
		}

		static inline u32 GetShaderDest() {return RC_dest_domain;}
	};

	template<>
	struct ShaderTypeTraits<SCS>
	{
		typedef CResourceManager::map_CS	MapType;
		typedef ID3DComputeShader			DXIface;

		static inline const char* GetShaderExt() {return ".cs";}

		static inline const char* GetCompilationTarget()
		{
			switch (HW.FeatureLevel)
			{
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
			//	return "cs_5_1";
			//	break;

			case D3D_FEATURE_LEVEL_11_0:
			case D3D_FEATURE_LEVEL_11_1:
				return "cs_5_0";
				break;

			case D3D_FEATURE_LEVEL_10_1:
				return "cs_4_1";
				break;

			default:
				return "cs_4_0";
			}
		}

		static inline DXIface* CreateHWShader(DWORD const* buffer, size_t size)
		{
			DXIface* sh = 0;
			R_CHK(HW.pDevice->CreateComputeShader(buffer, size, NULL, &sh));
			return sh;
		}

		static inline u32 GetShaderDest() {return RC_dest_compute;}
	};

	template<>
	inline CResourceManager::map_VS& CResourceManager::GetShaderMap(){return m_vs;}
	
	template<>
	inline CResourceManager::map_PS& CResourceManager::GetShaderMap(){return m_ps;}
	
	template<>
	inline CResourceManager::map_GS& CResourceManager::GetShaderMap(){return m_gs;}
	
	template<>
	inline CResourceManager::map_DS& CResourceManager::GetShaderMap(){return m_ds;}

	template<>
	inline CResourceManager::map_HS& CResourceManager::GetShaderMap(){return m_hs;}

	template<>
	inline CResourceManager::map_CS& CResourceManager::GetShaderMap(){return m_cs;}

    template<typename T>
	inline T* CResourceManager::CreateShader(pcstr name, pcstr filename, u32 flags)
	{
		ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<ShaderTypeTraits<T>::MapType>();
		LPSTR	N = LPSTR(name);
		ShaderTypeTraits<T>::MapType::iterator	I = sh_map.find(N);

		if (I!=sh_map.end())
			return		I->second;
		else
		{
			T*		sh = xr_new<T>();

			sh->dwFlags |= xr_resource_flagged::RF_REGISTERED;
			sh_map.insert(mk_pair(sh->set_name(name),sh));
			if (0==stricmp(name,"null"))
			{
				sh->sh				= NULL;
				return sh;
			}

			if (filename == NULL)
				filename = name;

			string_path					shName;
			const char*	pchr = strchr(filename, '(');
			ptrdiff_t	strSize = pchr?pchr- filename :xr_strlen(filename);
			strncpy(shName, filename, strSize);
			shName[strSize] = 0;

			// Open file
			string_path					cname;
			strconcat					(sizeof(cname), cname,::Render->getShaderPath(),shName, ShaderTypeTraits<T>::GetShaderExt());
			FS.update_path				(cname,	"$game_shaders$", cname);

			// duplicate and zero-terminate
			IReader* file				= FS.r_open(cname);
			R_ASSERT2					( file, cname );

			// Select target
			LPCSTR						c_target	= ShaderTypeTraits<T>::GetCompilationTarget();
			LPCSTR						c_entry		= "main";

			// Compile
			HRESULT	const _hr			= ::Render->shader_compile(name,(DWORD const*)file->pointer(),file->length(), c_entry, c_target, D3D_SHADER_PACK_MATRIX_ROW_MAJOR, (void*&)sh );

			FS.r_close					( file );

			VERIFY(SUCCEEDED(_hr));

			CHECK_OR_EXIT				(
				!FAILED(_hr),
				make_string("Your video card doesn't meet game requirements.\n\nTry to lower game settings.")
			);

			return			sh;
		}
	}

	template<typename T>
	inline bool CResourceManager::DestroyShader(const T* sh)
	{
		ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<ShaderTypeTraits<T>::MapType>();

		if (0==(sh->dwFlags&xr_resource_flagged::RF_REGISTERED))
			return false;

		LPSTR N = LPSTR(*sh->cName);
		typename ShaderTypeTraits<T>::MapType::iterator I = sh_map.find(N);
		
		if (I!=sh_map.end())
		{
			sh_map.erase(I);
			return true;
		}

		Msg	("! ERROR: Failed to find compiled geometry shader '%s'", *sh->cName);
		return false;
	}

