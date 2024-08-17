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

void CRender::Screenshot(IRender_interface::ScreenshotMode mode, LPCSTR name)
{
	if (!Device.b_is_Ready)
		return;

	BOOL fullscreen = psDeviceFlags.test(rsFullscreen);

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

#ifndef MASTER_GOLD
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
			HW.pDevice->CreateCubeTexture(face_size, 1, NULL, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM, &cubemap, NULL);
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

			IWriter* fs = FS.w_open("$cubemaps$", file_name);
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
	}
}
