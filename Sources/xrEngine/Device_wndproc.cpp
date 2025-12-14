#include "stdafx.h"
#include "../xrEngine/xr_ioconsole.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ACTIVATE:
		Device.OnWM_Activate(wParam, lParam);
		break;
	case WM_SETCURSOR:
		return 1;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_MOVE:
		case SC_SIZE:
		case SC_MAXIMIZE:
		case SC_MONITORPOWER:
			return 1;
		}
		break;
	case WM_CLOSE:
		Console->Execute("quit");
		return 0;
	case WM_KEYDOWN:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
