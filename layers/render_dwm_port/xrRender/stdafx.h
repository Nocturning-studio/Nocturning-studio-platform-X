// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#define dSINGLE

#pragma warning(disable:4995)
#include "../xrEngine/stdafx.h"
#pragma warning(disable:4995)
#include <d3dx9core.h>
#pragma warning(default:4995)
#pragma warning(disable:4714)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4237)

//#include "stdintport/stdintport.h"

#include <DXSDK/d3d11.h>
//#include <D3D11_3.h>
#include <D3Dx11core.h>
#include <D3DCompiler.h>

//#define __GFSDK_DX11__

#ifdef __GFSDK_DX11__

// HBAO+
#include <GFSDK/GFSDK_SSAO.h>
#pragma comment(lib, "GFSDK_SSAO_D3D11.win32.lib")

// TXAA
#include <GFSDK/GFSDK_TXAA.h>
#pragma comment(lib,"GFSDK_Txaa.win32.lib")

#endif

#include "xrD3DDefs.h"

//#include "dxPixEventWrapper.h"

#define R_R1 0
#define R_R2 1
#define R_R4 1
#define RENDER R_R4

#include "HW.h"
#include "Shader.h"
#include "R_Backend.h"
#include "R_Backend_Runtime.h"

#include "resourcemanager.h"

#include "../xrEngine/vis_common.h"
#include "../xrEngine/render.h"
#include "../xrEngine/_d3d_extensions.h"
#include "../xrEngine/igame_level.h"
#include "blender.h"
#include "blender_clsid.h"
#include "../xrParticles/psystem.h"
#include "xrRender_console.h"
#include "render.h"
