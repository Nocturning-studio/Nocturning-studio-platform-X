// xrRender_R4.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "dxRenderFactory.h"
#include "dxUIRender.h"
#include "dxDebugRender.h"
#include "xrRender_R4.h"

#pragma comment(lib,"xrEngine.lib")

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH	:
		//	Can't call CreateDXGIFactory from DllMain
		//if (!xrRender_test_hw())	return FALSE;
		::Render					= &RImplementation;
		::RenderFactory				= &RenderFactoryImpl;
		::DU						= &DUImpl;
		//::vid_mode_token			= inited by HW;
		UIRender					= &UIRenderImpl;
#ifdef DEBUG
		DRender						= &DebugRenderImpl;
#endif	//	DEBUG
		xrRender_initconsole		();
		break	;
	case DLL_THREAD_ATTACH	:
	case DLL_THREAD_DETACH	:
	case DLL_PROCESS_DETACH	:
		break;
	}
	return TRUE;
}

extern "C"
{
	RenderCreationParams::RendererSupport _declspec(dllexport) SupportsDX11Rendering();
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}

typedef HRESULT(__stdcall* FuncPtrD3D11CreateDeviceAndSwapChain)(
	IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain** ppSwapChain,
	ID3D11Device** ppDevice,
	D3D_FEATURE_LEVEL* pFeatureLevel,
	ID3D11DeviceContext** ppImmediateContext);

RenderCreationParams::RendererSupport _declspec(dllexport) SupportsDX11Rendering()
{
	RenderCreationParams::RendererSupport support;
	support.dx11 = false;

	HMODULE hD3D11 = LoadLibrary("d3d11.dll");

	if (!hD3D11)
	{
		Msg("! DX11: failed to load d3d11.dll");
		return support;
	}

	FuncPtrD3D11CreateDeviceAndSwapChain pD3D11CreateDeviceAndSwapChain =
		(FuncPtrD3D11CreateDeviceAndSwapChain)GetProcAddress(hD3D11, "D3D11CreateDeviceAndSwapChain");

	if (!pD3D11CreateDeviceAndSwapChain)
	{
		Msg("! DX11: failed to get address of D3D11CreateDeviceAndSwapChain");
		return support;
	}

	// Register class
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.lpszClassName = "TestDX11WindowClass";
	if (!RegisterClassEx(&wcex))
	{
		Msg("! DX11: failed to register window class");
		return support;
	}

	// Create window
	HWND hWnd = CreateWindow("TestDX11WindowClass", "",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, NULL, NULL);

	DXGI_SWAP_CHAIN_DESC sd;

	if (!hWnd)
	{
		Msg("! DX11: failed to create window");
		return support;
	}

	HRESULT hr = E_FAIL;

	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	ID3D11Device* pd3dDevice = NULL;
	ID3D11DeviceContext* pContext = NULL;
	IDXGISwapChain* pSwapChain = NULL;

#ifdef FEATURE_LEVELS_DEBUG
	hr = pD3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, RenderCreationParams::levels12, RenderCreationParams::count12,
		D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &support.level, &pContext);

	if (FAILED(hr))
	{
		hr = pD3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, RenderCreationParams::levels, RenderCreationParams::count,
			D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &support.level, &pContext);
	}
#else
	D3D_FEATURE_LEVEL FeatureLevel;
	hr = pD3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, RenderCreationParams::levels, RenderCreationParams::count,
		D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &FeatureLevel, &pContext);
#endif

	if (FAILED(hr))
		Msg("! D3D11: device creation failed with hr=0x%08x", hr);

	if (SUCCEEDED(hr))
		support.dx11 = true;

	if (pContext) pContext->Release();
	if (pSwapChain) pSwapChain->Release();
	if (pd3dDevice) pd3dDevice->Release();

	FreeLibrary(hD3D11);

	DestroyWindow(hWnd);

	return support;
}