// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#pragma warning(disable:4995)
#include "../xrEngine/stdafx.h"
#pragma warning(disable:4995)
#include <d3dx9core.h>
#pragma warning(default:4995)
#pragma warning(disable:4714)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4237)

#include "stdintport/stdintport.h"

#include <D3D11.h>
#include <D3D11_3.h>
#include <D3Dx11core.h>
#include <D3DCompiler.h>

#define __GFSDK_DX11__

#ifdef __GFSDK_DX11__
#include <GFSDK/GFSDK_TXAA.h>
#include <GFSDK/GFSDK_SSAO.h>
#endif

#include "xrD3DDefs.h"

#include "dxPixEventWrapper.h"

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
