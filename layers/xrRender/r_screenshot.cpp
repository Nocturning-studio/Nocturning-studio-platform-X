#include "stdafx.h"
#include "..\xrEngine\xr_effgamma.h"
#include "..\xrRender\tga.h"
#include "..\xrEngine\xrImage_Resampler.h"
#include "..\..\xrEngine\XR_IOConsole.h"

XRCORE_API u32 build_id;

IC u32 convert(float c)
{
	u32 C = iFloor(c);
	if (C > 255)
		C = 255;
	return C;
}
IC void MouseRayFromPoint(Fvector& direction, int x, int y, Fmatrix& m_CamMat)
{
	int halfwidth = Device.dwWidth / 2;
	int halfheight = Device.dwHeight / 2;

	Ivector2 point2;
	point2.set(x - halfwidth, halfheight - y);

	float size_y = VIEWPORT_NEAR * tanf(deg2rad(60.f) * 0.5f);
	float size_x = size_y / (Device.fHeight_2 / Device.fWidth_2);

	float r_pt = float(point2.x) * size_x / (float)halfwidth;
	float u_pt = float(point2.y) * size_y / (float)halfheight;

	direction.mul(m_CamMat.k, VIEWPORT_NEAR);
	direction.mad(direction, m_CamMat.j, u_pt);
	direction.mad(direction, m_CamMat.i, r_pt);
	direction.normalize();
}

void CPUTextureKillAlpha(ID3D11Texture2D* pTexture)
{
	D3D11_MAPPED_SUBRESOURCE rect;
	// R_CHK(Target->surf_screenshot_normal->LockRect(&rect, 0, D3DLOCK_NOSYSLOCK));
	R_CHK(HW.pContext11->Map(pTexture, 0, D3D11_MAP_READ_WRITE, 0, &rect));

	u32* pPixel = (u32*)rect.pData;
	u32* pEnd = pPixel + (Device.dwWidth * Device.dwHeight);

	for (; pPixel != pEnd; pPixel++)
	{
		u32 p = *pPixel;
		*pPixel = color_xrgb(color_get_R(p), color_get_G(p), color_get_B(p));
	}

	//R_CHK(Target->surf_screenshot_normal->UnlockRect());
	HW.pContext11->Unmap(pTexture, 0);
}

void CRender::Screenshot(IRender_interface::ScreenshotMode mode, LPCSTR name)
{
	if (!Device.b_is_Ready)
		return;

#if RENDER==R_R1
	//if (psDeviceFlags.test(rsFullscreen))
	//	R_CHK(HW.pDevice->GetFrontBufferData(NULL, Target->surf_screenshot_normal));
	//else
	//	R_CHK(HW.pDevice->GetRenderTargetData(HW.pBaseRT, Target->surf_screenshot_normal));

	ID3D11Texture2D* tex_backbuffer;
	HW.m_pSwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&tex_backbuffer);
	HW.pContext11->CopyResource(Target->tex_screenshot_normal, tex_backbuffer);

	string64 t_stemp;
	string_path file_name;

	// ID3DXBuffer* saved = 0;
	ID3DBlob* saved = 0;

	switch (mode)
	{
	case IRender_interface::SM_FOR_GAMESAVE: 
	{
		D3DX11_TEXTURE_LOAD_INFO info;
		memset(&info, 0, sizeof(info));
		info.SrcFirstMip = 0;
		info.DstFirstMip = 0;
		info.NumMips = 1;
		info.SrcFirstElement = 0;
		info.DstFirstElement = 0;
		info.NumElements = 1;
		info.Filter = D3DX11_FILTER_TRIANGLE;
		info.MipFilter = D3DX11_FILTER_TRIANGLE;

		// R_CHK(D3DXLoadSurfaceFromSurface(Target->surf_screenshot_gamesave, NULL, NULL,
		// Target->surf_screenshot_normal, NULL, NULL, D3DX_DEFAULT, NULL));
		R_CHK(D3DX11LoadTextureFromTexture(HW.pContext11, Target->tex_screenshot_normal, &info, Target->tex_screenshot_gamesave));

		//R_CHK(D3DXSaveTextureToFileInMemory(&saved, D3DXIFF_DDS, Target->tex_screenshot_gamesave, NULL));
		R_CHK(D3DX11SaveTextureToMemory(HW.pContext11, Target->tex_screenshot_gamesave, D3DX11_IFF_DDS, &saved, 0));
		
		IWriter* fs = FS.w_open(name);
		if (fs)
		{
			fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
			FS.w_close(fs);
		}
	}
	break;
	case IRender_interface::SM_NORMAL: 
	{
		sprintf_s(file_name, sizeof(file_name), "Xray (build id - %d) (user - %s) (time - %s) (%s)", build_id,
				  Core.UserName, timestamp(t_stemp), (g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");

		if (ps_render_flags.test(RFLAG_PNG_SCREENSHOTS))
		{
			strcat_s(file_name, ".png");
			CPUTextureKillAlpha(Target->tex_screenshot_normal);
			//R_CHK(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_PNG, Target->surf_screenshot_normal, NULL, NULL));
			R_CHK(D3DX11SaveTextureToMemory(HW.pContext11, Target->tex_screenshot_normal, D3DX11_IFF_PNG, &saved, 0));
		}
		else
		{
			strcat_s(file_name, ".jpg");
			//R_CHK(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_JPG, Target->surf_screenshot_normal, NULL, NULL));
			R_CHK(D3DX11SaveTextureToMemory(HW.pContext11, Target->tex_screenshot_normal, D3DX11_IFF_JPG, &saved, 0));
		}

		IWriter* fs = FS.w_open("$screenshots$", file_name);
		if (fs)
		{
			fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
			FS.w_close(fs);
		}
	}
	break;
	case IRender_interface::SM_FOR_LEVELMAP:
	{
		if (!g_pGameLevel)
		{
			Msg("! Can't capture level map, level does no loaded");
			return;
		}
		
		sprintf_s(file_name, sizeof(string_path), "level_map_%s_%s.dds", g_pGameLevel->name().c_str(), timestamp(t_stemp));

		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = 2048;
		desc.Height = 2048;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Format = DXGI_FORMAT_BC1_UNORM;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		//IDirect3DTexture9* texture;
		//R_CHK(HW.pDevice->CreateTexture(2048, 2048, 1, NULL, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &texture, NULL));
		//IDirect3DSurface9* surface;
		//R_CHK(texture->GetSurfaceLevel(0, &surface));
		ID3D11Texture2D* tex_levelmap;
		R_CHK(HW.pDevice11->CreateTexture2D(&desc, NULL, &tex_levelmap));

		D3DX11_TEXTURE_LOAD_INFO info;
		memset(&info, 0, sizeof(info));
		info.SrcFirstMip = 0;
		info.DstFirstMip = 0;
		info.NumMips = 1;
		info.SrcFirstElement = 0;
		info.DstFirstElement = 0;
		info.NumElements = 1;
		info.Filter = D3DX11_FILTER_TRIANGLE;
		info.MipFilter = D3DX11_FILTER_TRIANGLE;

		//R_CHK(D3DXLoadSurfaceFromSurface(surface, NULL, NULL, Target->surf_screenshot_normal, NULL, NULL, D3DX_DEFAULT, NULL));
		R_CHK(D3DX11LoadTextureFromTexture(HW.pContext11, Target->tex_screenshot_normal, &info, tex_levelmap));

		//R_CHK(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_DDS, surface, NULL, NULL));
		R_CHK(D3DX11SaveTextureToMemory(HW.pContext11, tex_levelmap, D3DX11_IFF_DDS, &saved, 0));

		IWriter* fs = FS.w_open("$screenshots$", file_name);
		if (fs)
		{
			fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
			FS.w_close(fs);
		}

		tex_levelmap->Release();
	}
	break;
	case IRender_interface::SM_FOR_CUBEMAP:
	{
		/* u32 face_size = ps_r_cubemap_size / 4;

		//static IDirect3DCubeTexture9* cubemap = 0;
		//static IDirect3DSurface9* surface[6] = {0};
		static ID3D11Texture2D* cubemap = 0;

		u32 id = (int)name[0] - (int)'1';

		// begin
		if (id == 0)
		{
			D3D11_TEXTURE2D_DESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.Width = 2048;
			desc.Height = 2048;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Format = DXGI_FORMAT_BC1_UNORM;
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			//HW.pDevice->CreateCubeTexture(face_size, 1, NULL, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &cubemap, NULL);
			R_CHK(HW.pDevice11->CreateTexture2D(&desc, NULL, &cubemap));
		}

		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = 2048;
		desc.Height = 2048;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Format = DXGI_FORMAT_BC1_UNORM;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		//D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)id;
		//cubemap->GetCubeMapSurface(face, 0, &surface[id]);
		static ID3D11Texture2D* face[6] = {0};
		R_CHK(HW.pDevice11->CreateTexture2D(&desc, NULL, &face[id]));

		D3DX11_TEXTURE_LOAD_INFO info;
		memset(&info, 0, sizeof(info));
		info.SrcFirstMip = 0;
		info.DstFirstMip = 0;
		info.NumMips = 1;
		info.SrcFirstElement = 0;
		info.DstFirstElement = 0;
		info.NumElements = 1;
		info.Filter = D3DX11_FILTER_TRIANGLE;
		info.MipFilter = D3DX11_FILTER_TRIANGLE;

		//R_CHK(D3DXLoadSurfaceFromSurface(surface[id], NULL, NULL, Target->surf_screenshot_normal, NULL, NULL, D3DX_DEFAULT, NULL));
		R_CHK(D3DX11LoadTextureFromTexture(HW.pContext, Target->tex_screenshot_normal, &info, face[id]));

		// end
		if (id == 5)
		{
			int i = D3D11CalcSubresource(0, id, 1);
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HW.pContext->Map(cubemap, i, D3D11_MAP_READ_WRITE, 0, &mappedResource);
			Texel* data = (Texel*)mappedResource.pData;

			// ???

			sprintf_s(file_name, sizeof(string_path), "cubemap_%s_%s.dds", Core.UserName, timestamp(t_stemp));

			//D3DXSaveTextureToFileInMemory(&saved, D3DXIFF_DDS, cubemap, NULL);
			R_CHK(D3DX11SaveTextureToMemory(HW.pContext, cubemap, D3DX11_IFF_DDS, &saved, 0));

			IWriter* fs = FS.w_open("$screenshots$", file_name);
			R_ASSERT(fs);

			fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
			FS.w_close(fs);

			_RELEASE(saved);

			_RELEASE(face[0]);
			_RELEASE(face[1]);
			_RELEASE(face[2]);
			_RELEASE(face[3]);
			_RELEASE(face[4]);
			_RELEASE(face[5]);

			_RELEASE(cubemap);
		}*/
	}
	break;
	}

	_RELEASE(saved);
	tex_backbuffer->Release();

	/*BOOL fullscreen = psDeviceFlags.test(rsFullscreen);

	if (fullscreen)
		R_CHK(HW.pDevice->GetFrontBufferData(NULL, Target->surf_screenshot_normal));
	else
		R_CHK(HW.pDevice->GetRenderTargetData(HW.pBaseRT, Target->surf_screenshot_normal));

	D3DLOCKED_RECT rect;
	R_CHK(Target->surf_screenshot_normal->LockRect(&rect, 0, D3DLOCK_NOSYSLOCK));

	u32* pPixel = (u32*)rect.pBits;
	u32* pEnd = pPixel + (Device.dwWidth * Device.dwHeight);

	for (; pPixel != pEnd; pPixel++)
	{
		u32 p = *pPixel;
		*pPixel = color_xrgb(color_get_R(p), color_get_G(p), color_get_B(p));
	}

	R_CHK(Target->surf_screenshot_normal->UnlockRect());

	string64 t_stemp;
	string_path file_name;

	switch (mode)
	{
	case IRender_interface::SM_FOR_GAMESAVE: 
	{
		R_CHK(D3DXLoadSurfaceFromSurface(Target->surf_screenshot_gamesave, NULL, NULL, 
			Target->surf_screenshot_normal, NULL, NULL, D3DX_DEFAULT, NULL));

		ID3DXBuffer* saved = 0;
		R_CHK(D3DXSaveTextureToFileInMemory(&saved, D3DXIFF_DDS, Target->tex_screenshot_gamesave, NULL));

		IWriter* fs = FS.w_open(name);

		if (fs)
		{
			fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
			FS.w_close(fs);
		}

		_RELEASE(saved);

		return;
	} 
	break;
	case IRender_interface::SM_NORMAL: 
	{
		sprintf_s(file_name, sizeof(file_name), "Xray (build id - %d) (user - %s) (time - %s) (%s)", build_id,
				  Core.UserName, timestamp(t_stemp), (g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");

		ID3DXBuffer* saved = 0;

#ifndef DEMO_BUILD
		bool UsePngFormat = strstr(Core.Params, "-screenshot_format_png");
#else
		bool UsePngFormat = true;
#endif

		if (UsePngFormat)
		{
			strconcat(sizeof(file_name), file_name, file_name, ".png");
			R_CHK(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_PNG, Target->surf_screenshot_normal, NULL, NULL));
		}
		else
		{
			strconcat(sizeof(file_name), file_name, file_name, ".jpg");
			R_CHK(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_JPG, Target->surf_screenshot_normal, NULL, NULL));
		}

		IWriter* fs = FS.w_open("$screenshots$", file_name);
		R_ASSERT(fs);

		fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
		FS.w_close(fs);

		_RELEASE(saved);

		return;
	} 
	break;
	case IRender_interface::SM_FOR_LEVELMAP:
	{
		if (!g_pGameLevel)
		{
			Msg("! Can't capture level map, level does no loaded");
			return;
		}
		
		sprintf_s(file_name, sizeof(string_path), "level_map_%s_%s.dds", g_pGameLevel->name().c_str(), timestamp(t_stemp));
		
		IDirect3DTexture9* texture;
		R_CHK(HW.pDevice->CreateTexture(2048, 2048, 1, NULL, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &texture, NULL));

		IDirect3DSurface9* surface;
		R_CHK(texture->GetSurfaceLevel(0, &surface));
		
		R_CHK(D3DXLoadSurfaceFromSurface(surface, NULL, NULL, Target->surf_screenshot_normal, NULL, NULL, D3DX_DEFAULT, NULL));

		ID3DXBuffer* saved = 0;
		R_CHK(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_DDS, surface, NULL, NULL));

		IWriter* fs = FS.w_open("$screenshots$", file_name);
		R_ASSERT(fs);

		fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
		FS.w_close(fs);

		_RELEASE(surface);
		_RELEASE(texture);

		_RELEASE(saved);

		return;
	} 
	break;
	case IRender_interface::SM_FOR_CUBEMAP: 
	{
		u32 face_size = ps_r_cubemap_size / 4;

		static IDirect3DCubeTexture9* cubemap = NULL;
		static IDirect3DSurface9* surface[6] = {NULL};

		u32 id = (int)name[0] - (int)'1';

		// begin
		if (id == 0)
		{
			HW.pDevice->CreateCubeTexture(face_size, 1, NULL, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &cubemap, NULL);
		}

		D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)id;
		cubemap->GetCubeMapSurface(face, 0, &surface[id]);
		R_CHK(D3DXLoadSurfaceFromSurface(surface[id], NULL, NULL, Target->surf_screenshot_normal, NULL, NULL, D3DX_DEFAULT, NULL));

		// end
		if (id == 5)
		{
			sprintf_s(file_name, sizeof(string_path), "cubemap_%s_%s.dds", Core.UserName, timestamp(t_stemp));

			ID3DXBuffer* saved = 0;
			D3DXSaveTextureToFileInMemory(&saved, D3DXIFF_DDS, cubemap, NULL);

			IWriter* fs = FS.w_open("$screenshots$", file_name);
			R_ASSERT(fs);

			fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
			FS.w_close(fs);

			_RELEASE(saved);

			_RELEASE(surface[0]);
			_RELEASE(surface[1]);
			_RELEASE(surface[2]);
			_RELEASE(surface[3]);
			_RELEASE(surface[4]);
			_RELEASE(surface[5]);

			_RELEASE(cubemap);
		}

		return;
	}
	break;
	}*/
#else
#pragma message(Reminder("Implement R2 screenshots"))
#endif
}
