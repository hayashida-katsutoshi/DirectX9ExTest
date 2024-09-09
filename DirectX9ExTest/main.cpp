
#define STRICT				// Strict type checking
#define WINVER        0x501	// Windows XP or later
#define _WIN32_WINNT  0x501 

#define SAFE_RELEASE(x)  { if(x) { (x)->Release(); (x)=NULL; } }

#define D3D_DEBUG_INFO				// Enable Direct3D debugging information.
#define D3DXFX_LARGEADDRESS_HANDLE	// Pass strings to D3DXHANDLE parameters.

#include <windows.h>
#include <crtdbg.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <string>
#include <sstream>
#include <d3dx9math.h>
#include <vector>

// Link required libraries.
#pragma comment( lib, "d3d9.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif
#pragma comment( lib, "dxerr.lib" )
#pragma comment( lib, "dxguid.lib" )

/*-------------------------------------------
	Global variables(Application)
--------------------------------------------*/
HINSTANCE	g_hInstance		= NULL;
std::vector<HWND>	g_hWindow;

WCHAR		g_szAppTitle[]	= L"Direct3D 9 Sample01";
WCHAR		g_szWndClass[]	= L"D3D9S01";

std::vector<SIZE> g_screens;

D3DFORMAT	g_formatFull	= D3DFMT_X8R8G8B8;		// Back buffer format

bool		g_bActive		= false;

/*-------------------------------------------
	Global variables(DirectX)
--------------------------------------------*/

// Interface
LPDIRECT3D9EX			g_pD3D			= NULL; // Direct3D interface.
LPDIRECT3DDEVICE9EX		g_pD3DDevice	= NULL; // Direct3DDevice interface.
std::vector <D3DPRESENT_PARAMETERS>	g_D3DPP;	// D3DDevice settings.

bool g_bDeviceLost = false;

// Sprite
LPD3DXSPRITE			g_pD3DXSprite = NULL;		// Sprite
LPDIRECT3DTEXTURE9		g_pD3DTexture = NULL;		// Texture for sprite.
WCHAR g_szSpriteFile[] = L".\\data\\canvas.dds";	// Location of texture data.

HRESULT ERR_MSGBOX(const WCHAR* str, HRESULT hr)
{
	DXTrace(__FILE__, (DWORD)__LINE__, hr, str, TRUE);
	exit(1);
	return hr;
}

/*-------------------------------------------

--------------------------------------------*/
LRESULT CALLBACK MainWndProc(HWND hWnd,UINT msg,UINT wParam,LONG lParam);

/*-------------------------------------------
	
--------------------------------------------*/
static int FindAdapter()
{
	HRESULT hr = E_FAIL;

	IDirect3D9Ex* dd = 0;
	if (FAILED(hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &dd)))
		return ERR_MSGBOX(L"[FindAdapter] Direct3DCreate9 failed.", hr);

	if (dd == NULL)
		return ERR_MSGBOX(L"[FindAdapter] Direct3DCreate9 D3D=NULL.", hr);

	Direct3DCreate9Ex( D3D_SDK_VERSION, &dd );

	UINT count = dd->GetAdapterCount();
	for( UINT i = 0 ; i < count ; i++ )
	{
		D3DADAPTER_IDENTIFIER9 id;
		if( dd->GetAdapterIdentifier( i, 0, &id ) == D3D_OK )
		{
			if (id.VendorId != 0x8086 || strstr(id.Description, "Intel(R) Arc(TM)") != NULL)
			{
				dd->Release();
				return i;
			}
		}
	}
	dd->Release();
	return 0;
}

static D3DCOLOR GetClearColor(int index)
{
	D3DCOLOR ClearColor[] =
	{
		D3DCOLOR_XRGB(0, 0, 255),
		D3DCOLOR_XRGB(0, 255, 0),
		D3DCOLOR_XRGB(255, 0, 0),
		D3DCOLOR_XRGB(255, 0, 255),
	};

	int num = _countof(ClearColor);
	if( index > num-1 )
	{
		index = num-1;
	}

	return ClearColor[index];
}

static D3DXVECTOR3 GetScrollPos(int index, float velocity, LPDIRECT3DTEXTURE9 tex)
{
	static std::vector<D3DXVECTOR3> sPos;

	if(sPos.empty())
	{
		sPos.resize(g_screens.size(), D3DXVECTOR3(0.0f, 0.0f, 1.0f));
	}

	if( index >= sPos.size())
	{
		return D3DXVECTOR3(0.0f,0.0f,1.0f);
	}

	D3DXVECTOR3 *pos = &sPos[index];
	pos->x += velocity;
	if( pos->x > g_screens[index].cx )
	{
		D3DSURFACE_DESC desc;
		if( tex && SUCCEEDED( tex->GetLevelDesc( 0, &desc ) ) )
		{
			pos->x = (float)(desc.Width) * -1.0f;
		}
	}

	if( pos->y == 0 )
	{
		D3DSURFACE_DESC desc;
		if( tex && SUCCEEDED( tex->GetLevelDesc( 0, &desc ) ) )
		{
			pos->y = (float)(g_screens[index].cy / 2 - desc.Height / 2);
		}
	}

	return *pos;
}

/*-------------------------------------------
	
--------------------------------------------*/
static D3DDISPLAYMODEEX *SetupDisplayModeEx(D3DPRESENT_PARAMETERS *pdpp, UINT numberOfAdaptersInGroup)
{
	D3DDISPLAYMODEEX *dm = NULL;

	if (pdpp && pdpp->Windowed == FALSE)
	{
		dm = new D3DDISPLAYMODEEX[sizeof(D3DDISPLAYMODEEX)*numberOfAdaptersInGroup];
		for( UINT i = 0; i < numberOfAdaptersInGroup; ++i )
		{
			D3DDISPLAYMODEEX *p = &dm[i];
			p->Format = pdpp[i].BackBufferFormat;
			p->Height = pdpp[i].BackBufferHeight;
			p->Width = pdpp[i].BackBufferWidth;
			p->ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
			p->RefreshRate = pdpp[i].FullScreen_RefreshRateInHz;
			p->Size = sizeof(D3DDISPLAYMODEEX);
		}
	}

	return dm;
}


/*-------------------------------------------

--------------------------------------------*/
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFOEX monitorInfo;

	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);

	SIZE resolution;
	resolution.cx = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	resolution.cy = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
	g_screens.push_back(resolution);

	return TRUE;
}

HRESULT InitApp(HINSTANCE hInst)
{
	g_hInstance = hInst;

	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= (WNDPROC)MainWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInst;
	wc.hIcon			= NULL;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName		= 0;
	wc.lpszClassName	= g_szWndClass;

	if (!RegisterClass(&wc))
		return ERR_MSGBOX(L"[InitApp] RegisterClass failed.", GetLastError());

	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

	// Create main window.
	g_hWindow.resize(g_screens.size());
	for(int i = 0; i < g_hWindow.size(); i++ )
	{
		g_hWindow[i] = CreateWindow(g_szWndClass, g_szAppTitle,
				WS_POPUP,
				0, 0, g_screens[i].cx, g_screens[i].cy,
				NULL, NULL, hInst, NULL);
		if (g_hWindow[i] == NULL)
			return ERR_MSGBOX(L"[InitApp] g_hWindow == NULL.", GetLastError());

		ShowWindow(g_hWindow[i], SW_SHOWNORMAL);
		UpdateWindow(g_hWindow[i]);
	}

	return S_OK;
}

/*-------------------------------------------

--------------------------------------------*/
HRESULT InitDXGraphics()
{
	HRESULT hr = E_FAIL;
	if (FAILED(hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3D)))
		return ERR_MSGBOX(L"[InitDXGraphics] Direct3DCreate9 failed.", hr);

	if (g_pD3D == NULL)
		return ERR_MSGBOX(L"[InitDXGraphics] Direct3DCreate9 D3D=NULL.", hr);

	D3DPRESENT_PARAMETERS val = { 0 };
	g_D3DPP.resize(g_screens.size(), val);

	DWORD BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	BehaviorFlags |= D3DCREATE_MULTITHREADED;
	BehaviorFlags |= D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX;
	if (g_screens.size() >= 2)
	{
		BehaviorFlags |= D3DCREATE_ADAPTERGROUP_DEVICE;
	}

	for( int i = 0; i < g_D3DPP.size(); i++ )
	{
		g_D3DPP[i].BackBufferWidth				= g_screens[i].cx;
		g_D3DPP[i].BackBufferHeight				= g_screens[i].cy;
		g_D3DPP[i].BackBufferFormat				= g_formatFull;
		g_D3DPP[i].BackBufferCount				= 1;
		g_D3DPP[i].MultiSampleType				= D3DMULTISAMPLE_NONE;
//		g_D3DPP[i].MultiSampleType				= D3DMULTISAMPLE_4_SAMPLES;
		g_D3DPP[i].MultiSampleQuality			= 0;
		g_D3DPP[i].hDeviceWindow				= g_hWindow[i];
		g_D3DPP[i].SwapEffect					= D3DSWAPEFFECT_DISCARD;
		g_D3DPP[i].Windowed = FALSE;
		g_D3DPP[i].FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		{
			// Multihead (Direct3D 9)
			// https://learn.microsoft.com/en-us/windows/win32/direct3d9/multihead
			// if EnableAutoDepthStencil is TRUE, then each of the following fields must have the same value for each D3DPRESENT_PARAMETERS:
			//		AutoDepthStencilFormat
			//		BackBufferWidth
			//		BackBufferHeight
			//		BackBufferFormat
			g_D3DPP[i].EnableAutoDepthStencil		= FALSE;
			g_D3DPP[i].AutoDepthStencilFormat		= D3DFMT_UNKNOWN;
		}
		g_D3DPP[i].Flags						= 0;
//		g_D3DPP[i].PresentationInterval			= D3DPRESENT_INTERVAL_IMMEDIATE;
		g_D3DPP[i].PresentationInterval			= D3DPRESENT_INTERVAL_ONE;
	}

	D3DDISPLAYMODEEX *dm = SetupDisplayModeEx(&g_D3DPP[0], static_cast<UINT>(g_D3DPP.size()));

	int adapter = FindAdapter();

	hr = g_pD3D->CreateDeviceEx(adapter, D3DDEVTYPE_HAL, g_hWindow[0],
						BehaviorFlags, &g_D3DPP[0], dm, &g_pD3DDevice);
	if (FAILED(hr))
	{
		hr = g_pD3D->CreateDeviceEx(adapter, D3DDEVTYPE_REF, g_hWindow[0],
						BehaviorFlags, &g_D3DPP[0], dm, &g_pD3DDevice);
		if (FAILED(hr))
		{
			free(dm);
			return ERR_MSGBOX(L"[InitDXGraphics] CreateDevice failed.", hr);
		}
	}

	if(dm)
	{
		free(dm);
	}

	// Viewport settings.
	D3DVIEWPORT9 vp;
	vp.X		= 0;
	vp.Y		= 0;
	vp.Width	= g_D3DPP[0].BackBufferWidth;
	vp.Height	= g_D3DPP[0].BackBufferHeight;
	vp.MinZ		= 0.0f;
	vp.MaxZ		= 1.0f;
	hr = g_pD3DDevice->SetViewport(&vp);
	if (FAILED(hr))
		return DXTRACE_ERR(L"InitDXGraphics SetViewport", hr);

	// Create a sprite.
	hr = D3DXCreateTextureFromFile(g_pD3DDevice, g_szSpriteFile, &g_pD3DTexture);
	if (FAILED(hr))
		return ERR_MSGBOX(L"[InitDXGraphics] D3DXCreateTextureFromFile failed.", hr);

	hr = D3DXCreateSprite(g_pD3DDevice, &g_pD3DXSprite);
	if (FAILED(hr))
		return ERR_MSGBOX(L"[InitDXGraphics] D3DXCreateSprite failed.", hr);

	return S_OK;
}

/*-------------------------------------------

--------------------------------------------*/
HRESULT InitD3DObject(void)
{
	if (g_pD3DXSprite)
		g_pD3DXSprite->OnResetDevice();

	return S_OK;
}

/*--------------------------------------------

--------------------------------------------*/
HRESULT Render(void)
{
	for( int i = 0; i < g_D3DPP.size(); i++ )
	{
		IDirect3DSwapChain9 *pChain = NULL;
		{
			HRESULT hr = g_pD3DDevice->GetSwapChain(i, &pChain);
			if (FAILED(hr))
			{
				ERR_MSGBOX(L"GetSwapChain() failed.", hr);
			}
		}
		IDirect3DSurface9 *pBackBuffer = NULL;
		{
			HRESULT hr = pChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
			if (FAILED(hr))
			{
				ERR_MSGBOX(L"GetRenderTarget() failed.", hr);
			}
		}

		HRESULT hr = g_pD3DDevice->SetRenderTarget(0, pBackBuffer);
		if (FAILED(hr))
		{
			ERR_MSGBOX(L"SetRenderTarget() failed.", hr);
		}

		// Clear scene.
		g_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, GetClearColor(i), 1.0f, 0);

		// Draw scene
		if (SUCCEEDED(g_pD3DDevice->BeginScene()))
		{
			// Draw sprite
			if( g_pD3DXSprite )
			{
				g_pD3DXSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DONOTSAVESTATE);
				g_pD3DXSprite->Draw(g_pD3DTexture, NULL, NULL, &GetScrollPos(i, 5.0f, g_pD3DTexture), D3DCOLOR_ARGB(255,255,255,255));
				g_pD3DXSprite->End();
			}

			g_pD3DDevice->EndScene();
		}
	}

	// Show scene.
	return g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

/*-------------------------------------------

--------------------------------------------*/
HRESULT CleanupD3DObject(void)
{
	if (g_pD3DXSprite)
		g_pD3DXSprite->OnLostDevice();

	return S_OK;
}

/*-------------------------------------------

--------------------------------------------*/
HRESULT ChangeWindowSize(void)
{
	CleanupD3DObject();

	D3DDISPLAYMODEEX *dm = SetupDisplayModeEx(&g_D3DPP[0], static_cast<UINT>(g_D3DPP.size()));

	HRESULT hr = g_pD3DDevice->ResetEx(&g_D3DPP[0], dm);
	if (FAILED(hr))
	{
		if (hr == D3DERR_DEVICELOST)
			g_bDeviceLost = true;
		else
		{
			for(int i = 0; i < g_hWindow.size(); i++ )
			{
				DestroyWindow(g_hWindow[i]);
			}
		}
		ERR_MSGBOX(L"[ChangeWindowSize] ResetEx() failed.", hr);
	}
	hr = InitD3DObject();
	if (FAILED(hr))
	{
		for(int i = 0; i < g_hWindow.size(); i++ )
		{
			DestroyWindow(g_hWindow[i]);
		}
		ERR_MSGBOX(L"[ChangeWindowSize] InitD3DObject failed.", hr);
	}

	// Viewport settings.
	D3DVIEWPORT9 vp;
	vp.X		= 0;
	vp.Y		= 0;
	vp.Width	= g_D3DPP[0].BackBufferWidth;
	vp.Height	= g_D3DPP[0].BackBufferHeight;
	vp.MinZ		= 0.0f;
	vp.MaxZ		= 1.0f;
	hr = g_pD3DDevice->SetViewport(&vp);
	if (FAILED(hr))
	{
		for(int i = 0; i < g_hWindow.size(); i++ )
		{
			DestroyWindow(g_hWindow[i]);
		}
		ERR_MSGBOX(L"[ChangeWindowSize] SetViewport failed.", hr);
	}
	return hr;
}

/*-------------------------------------------

--------------------------------------------*/
bool CleanupDXGraphics(void)
{
	SAFE_RELEASE(g_pD3DXSprite);
	SAFE_RELEASE(g_pD3DTexture);

	SAFE_RELEASE(g_pD3DDevice);
	SAFE_RELEASE(g_pD3D);

	return true;
}

/*-------------------------------------------

--------------------------------------------*/
bool CleanupApp(void)
{
	UnregisterClass(g_szWndClass, g_hInstance);
	return true;
}

/*-------------------------------------------

--------------------------------------------*/
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, UINT wParam, LONG lParam)
{
	HRESULT hr = S_OK;

	switch(msg)
	{
	case WM_ACTIVATE:
		g_bActive = (LOWORD(wParam) != 0);
		break;

	case WM_DESTROY:
		CleanupD3DObject();
		CleanupDXGraphics();
		PostQuitMessage(0);
		for(int i = 0; i < g_hWindow.size(); i++ )
		{
			g_hWindow[i] = NULL;
		}
		return 0;

	case WM_SIZE:
		if (!g_pD3DDevice || wParam == SIZE_MINIMIZED)
			break;
		g_D3DPP[0].BackBufferWidth  = LOWORD(lParam);
		g_D3DPP[0].BackBufferHeight = HIWORD(lParam);
		if(g_bDeviceLost)
			break;
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
			ChangeWindowSize();
		break;

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

/*--------------------------------------------

--------------------------------------------*/
bool AppIdle(void)
{
	if (!g_pD3D || !g_pD3DDevice)
		return false;

	if (!g_bActive)
		return true;

	HRESULT hr;
	if (g_bDeviceLost)
	{
		Sleep(100);

		hr  = g_pD3DDevice->TestCooperativeLevel();
		if (FAILED(hr))
		{
			if (hr == D3DERR_DEVICELOST)
				return true;

			if (hr != D3DERR_DEVICENOTRESET)
				return false;

			D3DDISPLAYMODEEX *dm = SetupDisplayModeEx(&g_D3DPP[0], static_cast<UINT>(g_D3DPP.size()));
			CleanupD3DObject();
			hr = g_pD3DDevice->ResetEx(&g_D3DPP[0], dm);
			if (FAILED(hr))
			{
				if (hr == D3DERR_DEVICELOST)
					return true;

				ERR_MSGBOX(L"[AppIdle] ResetEx failed.", hr);
				return false;
			}
			hr = InitD3DObject();
			if (FAILED(hr))
			{
				ERR_MSGBOX(L"[AppIdle] InitD3DObject failed.", hr);
				return false;
			}
		}
		g_bDeviceLost = false;
	}

	hr = Render();
	if (hr == D3DERR_DEVICELOST)
		g_bDeviceLost = true;
	else if (FAILED(hr))
		return false;

	return true;
}

/*--------------------------------------------

---------------------------------------------*/
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

	HRESULT hr = InitApp(hInst);
	if (FAILED(hr))
	{
		ERR_MSGBOX(L"[WinMain] InitApp failed.", hr);
		return 0;
	}

	hr = InitDXGraphics();
	if (FAILED(hr))
		ERR_MSGBOX(L"[WinMain] InitDXGraphics failed.", hr);
	else
	{
		hr = InitD3DObject();
		if (FAILED(hr))
		{
			ERR_MSGBOX(L"[WinMain] InitD3DObject failed.", hr);
			for(int i = 0; i < g_hWindow.size(); i++ )
			{
				DestroyWindow(g_hWindow[i]);
			}
		}
	}

	MSG msg;
	do
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (!AppIdle())
			{
				for(int i = 0; i < g_hWindow.size(); i++ )
				{
					DestroyWindow(g_hWindow[i]);
				}
			}
		}
	} while (msg.message != WM_QUIT);

	CleanupApp();

	return (int)msg.wParam;
}
