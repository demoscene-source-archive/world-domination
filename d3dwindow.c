#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#ifdef _DEBUG
#include <stdio.h>
#endif

#include "resource.h"
#include "d3dwindow.h"

static LRESULT CALLBACK window_proc(HWND win,UINT message,WPARAM wparam,LPARAM lparam);

typedef LPDIRECT3D9 (WINAPI * DIRECT3DCREATE9) (unsigned int sdkVersion);
static HMODULE library = 0;

IDirect3D9 *direct3d = NULL;
IDirect3DDevice9 *device = NULL;
HWND win = NULL;
HINSTANCE inst;

unsigned int width;
unsigned int height;
BOOL fullscreen;

IDirect3DDevice9 *d3dwin_open( char *title, unsigned int width_, unsigned int height_, D3DFORMAT format, BOOL fullscreen_ ){
	DIRECT3DCREATE9 Direct3DCreate9;
	RECT rect;
	WNDCLASS wc;
	UINT32 style;

	D3DDISPLAYMODE displaymode;
	D3DPRESENT_PARAMETERS presentparameters;
	D3DCAPS9 caps;

	width = width_;
	height = height_;
	fullscreen = fullscreen_;
	inst = GetModuleHandle(NULL);

	library = (HMODULE) LoadLibrary("d3d9.dll");
	if(!library) return NULL;
	Direct3DCreate9 = (DIRECT3DCREATE9) GetProcAddress(library,"Direct3DCreate9");
	if(!Direct3DCreate9) return NULL;
	direct3d=Direct3DCreate9(D3D_SDK_VERSION);
	if(!direct3d) return NULL;

	if(!fullscreen)
		style = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
	else{
		style = WS_POPUP;
		ShowCursor(FALSE);
	}

	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;

	if(!fullscreen) AdjustWindowRect( &rect, style, FALSE );
	
	wc.style = CS_VREDRAW|CS_HREDRAW;
	wc.lpfnWndProc = window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = LoadIcon(inst,MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor = LoadCursor(inst,IDC_ARROW);
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = "d3d9";
	RegisterClass(&wc);
	
	win = CreateWindowEx(0, "d3d9", title, (style)|WS_VISIBLE, 0, 0, rect.right-rect.left, rect.bottom-rect.top, 0, 0, inst, 0);
	if(!win) return NULL;

	memset(&presentparameters, 0, sizeof(D3DPRESENT_PARAMETERS));
	if(!fullscreen){
		if(FAILED(IDirect3D9_GetAdapterDisplayMode( direct3d, D3DADAPTER_DEFAULT, &displaymode ))){
			d3dwin_close();
			return NULL;
		}
	}

	if(!fullscreen) presentparameters.Windowed = TRUE;

	presentparameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentparameters.BackBufferWidth = width;
	presentparameters.BackBufferHeight = height;

	if( fullscreen ) presentparameters.BackBufferFormat = format;
	else presentparameters.BackBufferFormat = displaymode.Format;


	presentparameters.AutoDepthStencilFormat = D3DFMT_D24S8;
	presentparameters.EnableAutoDepthStencil = TRUE;

	IDirect3D9_GetDeviceCaps(direct3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

	if(FAILED(IDirect3D9_CreateDevice( direct3d,
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		win,
		(caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT) ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&presentparameters,
		&device))){

		d3dwin_close();
		return NULL;
	}

	IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
	IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

	return device;
}

void d3dwin_close(){
	if(fullscreen) ShowCursor(TRUE);
	if(device){
		IDirect3DDevice9_EvictManagedResources(device);
		IDirect3DDevice9_Release(device);
		device = NULL;
	}
	if(direct3d){
		IDirect3D9_Release(direct3d);
		direct3d = NULL;
	}
	if(win){
		DestroyWindow(win);
		win = NULL;
	}
	UnregisterClass("d3d9", inst);
}

static LRESULT CALLBACK window_proc(HWND win,UINT message,WPARAM wparam,LPARAM lparam){
	switch (message){
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	break;
	case WM_SYSCOMMAND:
		switch(wparam){
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
	break;
	}
    return DefWindowProc(win,message,wparam,lparam);
}
