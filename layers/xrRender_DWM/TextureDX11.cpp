// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include <D3DX11Core.h>

#include "dxRenderDeviceRender.h"

// #include "std_classes.h"
// #include "xr_avi.h"

void fix_texture_name(LPSTR fn)
{
	LPSTR _ext = strext(fn);
	if(  _ext					&&
		(0==stricmp(_ext,".tga")	||
		0==stricmp(_ext,".dds")	||
		0==stricmp(_ext,".bmp")	||
		0==stricmp(_ext,".ogm") ||
		0==stricmp(_ext,".ogv")) )
		*_ext = 0;
}

int get_texture_load_lod(LPCSTR fn)
{
	CInifile::Sect& sect	= pSettings->r_section("reduce_lod_texture_list");
	CInifile::SectCIt it_	= sect.Data.begin();
	CInifile::SectCIt it_e_	= sect.Data.end();

	ENGINE_API bool is_enough_address_space_available();
	static bool enough_address_space_available = is_enough_address_space_available();

	CInifile::SectCIt it	= it_;
	CInifile::SectCIt it_e	= it_e_;

	for(;it!=it_e;++it)
	{
		if( strstr(fn, it->first.c_str()) )
		{
			if(psTextureLOD<1) {
				if ( enough_address_space_available )
					return 0;
				else
					return 1;
			}
			else
				if(psTextureLOD<3)
					return 1;
				else
					return 2;
		}
	}

	if(psTextureLOD<2) {
//		if ( enough_address_space_available )
			return 0;
//		else
//			return 1;
	}
	else
		if(psTextureLOD<4)
			return 1;
		else
			return 2;
}

u32 calc_texture_size(int lod, u32 mip_cnt, u32 orig_size)
{
	if(1==mip_cnt)
		return orig_size;

	int _lod		= lod;
	float res		= float(orig_size);

	while(_lod>0){
		--_lod;
		res		-= res/1.333f;
	}
	return iFloor	(res);
}

const float		_BUMPHEIGH = 8.f;
//////////////////////////////////////////////////////////////////////
// Utility pack
//////////////////////////////////////////////////////////////////////
IC u32 GetPowerOf2Plus1	(u32 v)
{
	u32 cnt=0;
	while (v) {v>>=1; cnt++; };
	return cnt;
}
IC void	Reduce				(int& w, int& h, int& l, int& skip)
{
	while ((l>1) && skip)
	{
		w /= 2;
		h /= 2;
		l -= 1;

		skip--;
	}
	if (w<1)	w=1;
	if (h<1)	h=1;
}

IC void	Reduce(UINT& w, UINT& h, int l, int skip)
{
	while ((l>1) && skip)
	{
		w /= 2;
		h /= 2;
		l -= 1;

		skip--;
	}
	if (w<1)	w=1;
	if (h<1)	h=1;
}

void				TW_Save	(ID3DTexture2D* T, LPCSTR name, LPCSTR prefix, LPCSTR postfix)
{
	string256		fn;		strconcat	(sizeof(fn),fn,name,"_",prefix,"-",postfix);
	for (int it=0; it<int(xr_strlen(fn)); it++)	
		if ('\\'==fn[it])	fn[it]	= '_';
	string256		fn2;	strconcat	(sizeof(fn2),fn2,"debug\\",fn,".dds");
	Log						("* debug texture save: ",fn2);
	R_CHK					(D3DX11SaveTextureToFile(HW.pContext, T, D3DX11_IFF_DDS, fn2));
}

ID3DBaseTexture*	CRender::texture_load(LPCSTR fRName, u32& ret_msize, bool bStaging)
{
	//	Moved here just to avoid warning
	D3DX11_IMAGE_INFO			IMG;
	ZeroMemory(&IMG, sizeof(IMG));

	//	Staging control
	if(!opt(R__USE_TEX_STAGING))
		bStaging = false;

	ID3DBaseTexture*		pTexture2D		= NULL;
	//IDirect3DCubeTexture9*	pTextureCUBE	= NULL;
	string_path				fn;
	//u32						dwWidth,dwHeight;
	u32						img_size		= 0;
	int						img_loaded_lod	= 0;

	u32						mip_cnt=u32(-1);
	// validation
	R_ASSERT				(fRName);
	R_ASSERT				(fRName[0]);

	// make file name
	string_path				fname;
	xr_strcpy(fname,fRName); //. andy if (strext(fname)) *strext(fname)=0;
	fix_texture_name		(fname);
	IReader* S				= NULL;
	if (!FS.exist(fn,"$game_textures$",	fname,	".dds")	&& strstr(fname,"_bump"))	goto _BUMP_from_base;
	if (FS.exist(fn,"$level$",			fname,	".dds"))							goto _DDS;
	if (FS.exist(fn,"$game_saves$",		fname,	".dds"))							goto _DDS;
	if (FS.exist(fn,"$game_textures$",	fname,	".dds"))							goto _DDS;


#ifdef _EDITOR
	ELog.Msg(mtError,"Can't find texture '%s'",fname);
	return 0;
#else

	Msg("! Can't find texture '%s'",fname);
	R_ASSERT(FS.exist(fn,"$game_textures$",	"ed\\ed_not_existing_texture",".dds"));
	goto _DDS;

	//	Debug.fatal(DEBUG_INFO,"Can't find texture '%s'",fname);

#endif

_DDS:
	{
		// Load and get header

		S						= FS.r_open	(fn);
#ifdef DEBUG
		Msg						("* Loaded: %s[%d]b",fn,S->length());
#endif // DEBUG
		img_size				= S->length	();
		R_ASSERT				(S);
		//R_CHK2					(D3DXGetImageInfoFromFileInMemory	(S->pointer(),S->length(),&IMG), fn);

		R_CHK2 (D3DX11GetImageInfoFromMemory(S->pointer(),S->length(), 0, &IMG, 0), fn);

		//if (IMG.ResourceType	== D3DRTYPE_CUBETEXTURE)			goto _DDS_CUBE;
		if (IMG.MiscFlags & D3D_RESOURCE_MISC_TEXTURECUBE)			goto _DDS_CUBE;
		else														goto _DDS_2D;

_DDS_CUBE:
		{
			//R_CHK(D3DXCreateCubeTextureFromFileInMemoryEx(
			//	HW.pDevice,
			//	S->pointer(),S->length(),
			//	D3DX_DEFAULT,
			//	IMG.MipLevels,0,
			//	IMG.Format,
			//	D3DPOOL_MANAGED,
			//	D3DX_DEFAULT,
			//	D3DX_DEFAULT,
			//	0,&IMG,0,
			//	&pTextureCUBE
			//	));

			//	Inited to default by provided default constructor

			D3DX11_IMAGE_LOAD_INFO LoadInfo;

			//LoadInfo.Usage = D3D_USAGE_IMMUTABLE;
			if (bStaging)
			{
				LoadInfo.Usage = D3D_USAGE_STAGING;
				LoadInfo.BindFlags = 0;
				LoadInfo.CpuAccessFlags = D3D_CPU_ACCESS_WRITE;
			}
			else
			{
				LoadInfo.Usage = D3D_USAGE_DEFAULT;
				LoadInfo.BindFlags = D3D_BIND_SHADER_RESOURCE;
			}
			
			LoadInfo.pSrcInfo = &IMG;

			R_CHK(D3DX11CreateTextureFromMemory(
				HW.pDevice,
				S->pointer(),S->length(),
				&LoadInfo,
				0,
				&pTexture2D,
				0
				));


			FS.r_close				(S);

			// OK
			mip_cnt					= IMG.MipLevels;
			ret_msize				= calc_texture_size(img_loaded_lod, mip_cnt, img_size);
			return					pTexture2D;
		}
_DDS_2D:
		{
			// Check for LMAP and compress if needed
			strlwr					(fn);


			// Load   SYS-MEM-surface, bound to device restrictions
			//ID3DTexture2D*		T_sysmem;
			//R_CHK2(D3DXCreateTextureFromFileInMemoryEx
			//	(
			//	HW.pDevice,S->pointer(),S->length(),
			//	D3DX_DEFAULT,D3DX_DEFAULT,
			//	IMG.MipLevels,0,
			//	IMG.Format,
			//	D3DPOOL_SYSTEMMEM,
			//	D3DX_DEFAULT,
			//	D3DX_DEFAULT,
			//	0,&IMG,0,
			//	&T_sysmem
			//	), fn);

			img_loaded_lod			= get_texture_load_lod(fn);

			//	Inited to default by provided default constructor

			D3DX11_IMAGE_LOAD_INFO LoadInfo;

			//LoadInfo.FirstMipLevel = img_loaded_lod;
			LoadInfo.Width	= IMG.Width;
			LoadInfo.Height	= IMG.Height;

			if (img_loaded_lod)
			{
				Reduce(LoadInfo.Width, LoadInfo.Height, IMG.MipLevels, img_loaded_lod);
			}

			//LoadInfo.Usage = D3D_USAGE_IMMUTABLE;
			if (bStaging)
			{
				LoadInfo.Usage = D3D_USAGE_STAGING;
				LoadInfo.BindFlags = 0;
				LoadInfo.CpuAccessFlags = D3D_CPU_ACCESS_WRITE;
			}
			else
			{
				LoadInfo.Usage = D3D_USAGE_DEFAULT;
				LoadInfo.BindFlags = D3D_BIND_SHADER_RESOURCE;
			}
			LoadInfo.pSrcInfo = &IMG;


			R_CHK2(D3DX11CreateTextureFromMemory
				(
				HW.pDevice,S->pointer(),S->length(),
				&LoadInfo,
				0,
				&pTexture2D,
				0
				), fn);

			FS.r_close				(S);
			mip_cnt					= IMG.MipLevels;
			// OK
			ret_msize				= calc_texture_size(img_loaded_lod, mip_cnt, img_size);
			return					pTexture2D;
		}
	}

_BUMP_from_base:
	{
		//Msg			("! auto-generated bump map: %s",fname);
		Msg			("! Fallback to default bump map: %s",fname);
		//////////////////
		if (strstr(fname,"_bump#"))			
		{
			R_ASSERT2	(FS.exist(fn,"$game_textures$",	"ed\\ed_dummy_bump#",	".dds"), "ed_dummy_bump#");
			S						= FS.r_open	(fn);
			R_ASSERT2				(S, fn);
			img_size				= S->length	();
			goto		_DDS_2D;
		}
		if (strstr(fname,"_bump"))			
		{
			R_ASSERT2	(FS.exist(fn,"$game_textures$",	"ed\\ed_dummy_bump",	".dds"),"ed_dummy_bump");
			S						= FS.r_open	(fn);

			R_ASSERT2	(S, fn);

			img_size				= S->length	();
			goto		_DDS_2D;
		}
		//////////////////
	}

	return 0;
}
