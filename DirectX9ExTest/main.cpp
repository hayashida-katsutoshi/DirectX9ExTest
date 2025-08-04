
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
#include <mmsystem.h>

// Link required libraries.
#pragma comment( lib, "d3d9.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif
#pragma comment( lib, "dxerr.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "winmm.lib")

#define STR_BUF_SIZE	256

/*-------------------------------------------
	Global variables(Application)
--------------------------------------------*/
HINSTANCE	g_hInstance		= NULL;
std::vector<HWND>	g_hWindow;

WCHAR		g_szAppTitle[]	= L"Direct3D 9 Sample01";
WCHAR		g_szWndClass[]	= L"D3D9S01";

typedef struct _DisplayInfo
{
	SIZE size;
	MONITORINFOEX monitor;
	std::string deviceName;
}DisplayInfo;
std::vector<DisplayInfo> g_screens;

D3DFORMAT	g_formatFull	= D3DFMT_X8R8G8B8;		// Back buffer format

bool		g_bActive		= false;

D3DMULTISAMPLE_TYPE g_multiSampleType = D3DMULTISAMPLE_NONE;

bool		g_noWaitMode = false;

UINT		g_rebootSec = 0;
UINT		g_rebootCount = 0;
bool		g_rebootTriggered = false;

UINT		g_frameCount = 0;

int			g_fps = 0;

int			g_objCount = 0;

int			g_need_foreground = 2;

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

// TestTexture
LPDIRECT3DTEXTURE9		g_pBgTexture = NULL;
WCHAR g_szBgTextureFile[] = L".\\data\\SL_BlindPanel_en_r001.dds";

// TestTexture
LPDIRECT3DTEXTURE9		g_pTestTexture = NULL;
WCHAR g_szTestTextureFile[] = L".\\data\\font8x8.tga";
SIZE g_testTextureSize;

UINT g_backBufferCount = 2;

// Font
ID3DXFont* g_pFont = NULL; 

/*-------------------------------------------

--------------------------------------------*/
HRESULT CleanupD3DObject(void);
bool CleanupDXGraphics(void);
bool CleanupApp(void);

#define Exception(str, hr)	\
{	\
	CleanupD3DObject();	\
	CleanupDXGraphics();	\
	for (int i = 0; i < g_hWindow.size(); i++)	\
	{	\
		ShowWindow(g_hWindow[i], SW_HIDE);	\
		UpdateWindow(g_hWindow[i]);	\
	}	\
	DXTrace(__FILE__, (DWORD)__LINE__, hr, str, TRUE);	\
	CleanupApp();	\
	exit(1);	\
}

/*-------------------------------------------

--------------------------------------------*/
LRESULT CALLBACK MainWndProc(HWND hWnd,UINT msg,UINT wParam,LONG lParam);
int RebootProcess();
void RebootTriggered();
void MeasureFramePerSecond();

void resetSync();
bool preSync();
void postSync();

void DrawRotationBox(int x, int y, int w, int h, DWORD color);

void SetAbsoluteForegroundWindow(HWND hWnd);

void DrawTestTexture(int x, int y);

/*-------------------------------------------

--------------------------------------------*/
std::string WStringToString(std::wstring oWString)
{
	int iBufferSize = WideCharToMultiByte(CP_OEMCP, 0, oWString.c_str(), -1, (char*)NULL, 0, NULL, NULL);

	CHAR* cpMultiByte = new CHAR[iBufferSize];

	WideCharToMultiByte(CP_OEMCP, 0, oWString.c_str(), -1, cpMultiByte, iBufferSize, NULL, NULL);

	std::string oRet(cpMultiByte, cpMultiByte + iBufferSize - 1);

	delete[] cpMultiByte;

	return(oRet);
}

std::vector<std::string> split(const std::string& s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (getline(ss, item, delim)) {
		if (!item.empty()) {
			elems.push_back(item);
		}
	}
	return elems;
}

SIZE GetResolutionFromString(std::wstring str)
{
	std::string val = WStringToString(str);
	std::vector<std::string> params = split(val, 'x');

	SIZE resolution;
	resolution.cx = stoi(params[0]);
	resolution.cy = stoi(params[1]);
	return resolution;
}

void SetCommandLineArgs()
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	int primaryWidth = 0, primaryHeight = 0;
	int secondaryWidth = 0, secondaryHeight = 0;
	for (int i = 0; i < argc; i++)
	{
		std::string opt = WStringToString(argv[i]);
		if (opt.compare("--primary") == 0)
		{
			if (g_screens.size() > 0)
			{
				g_screens[0].size = GetResolutionFromString(argv[i + 1]);
			}
		}
		else if (opt.compare("--secondary") == 0)
		{
			if (g_screens.size() > 1)
			{
				g_screens[1].size = GetResolutionFromString(argv[i + 1]);
			}
		}
		else if (opt.compare("--tertiary") == 0)
		{
			if (g_screens.size() > 2)
			{
				g_screens[2].size = GetResolutionFromString(argv[i + 1]);
			}
		}
		else if (opt.compare("--msaa") == 0)
		{
			g_multiSampleType = D3DMULTISAMPLE_4_SAMPLES;
		}
		else if (opt.compare("--nowait") == 0)
		{
			g_noWaitMode = true;
		}
		else if (opt.compare("--bbcount") == 0)
		{
			LPWSTR optval = argv[i + 1];
			try
			{
				if (optval == NULL)
					throw std::runtime_error("The option value is null.");

				UINT val = std::stoi(optval);
				if (val > D3DPRESENT_BACK_BUFFERS_MAX_EX)
				{
					val = D3DPRESENT_BACK_BUFFERS_MAX_EX;
				}
				g_backBufferCount = val;
			}
			catch (std::exception e)
			{
				std::wstringstream	ss;
				ss << e.what() << "\n" << "Invalid parameter (" << (optval == NULL ? L"0" : optval) << ") for --bbcount option.";
				Exception(ss.str().c_str(), S_FALSE);
			}
		}
		else if (opt.compare("--reboot") == 0)
		{
			LPWSTR optval = argv[i + 1];
			g_rebootSec = std::stoi(optval);
		}
		else if (opt.compare("--rebootCount") == 0)
		{
			LPWSTR optval = argv[i + 1];
			g_rebootCount = std::stoi(optval);
		}
	}
}

/*-------------------------------------------

--------------------------------------------*/
bool GetDisplaySettings(DEVMODE& mode, std::wstring deviceName, int x, int y, int w, int h)
{
	EnumDisplaySettings(deviceName.c_str(), ENUM_CURRENT_SETTINGS, &mode);

	if ((mode.dmPosition.x == x) &&
		(mode.dmPosition.y == y) &&
		(mode.dmPelsWidth == static_cast<DWORD>(w)) &&
		(mode.dmPelsHeight == static_cast<DWORD>(h)))
	{
		return false;
	}


	mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;
	mode.dmPelsWidth = static_cast<DWORD>(w);
	mode.dmPelsHeight = static_cast<DWORD>(h);
	mode.dmPosition.x = x;
	mode.dmPosition.y = y;

	switch (mode.dmDisplayOrientation)
	{
	case DMDO_DEFAULT:
	case DMDO_180:
		if (mode.dmPelsWidth < mode.dmPelsHeight)
		{
			mode.dmDisplayOrientation = DMDO_90;
			mode.dmFields |= DM_DISPLAYORIENTATION;
		}
		break;
	case DMDO_90:
	case DMDO_270:
		if (mode.dmPelsWidth > mode.dmPelsHeight)
		{
			mode.dmDisplayOrientation = DMDO_DEFAULT;
			mode.dmFields |= DM_DISPLAYORIENTATION;
		}
		break;
	}

	return true;
}

LONG ChangeDisplayResolution(std::wstring deviceName, int x, int y, int w, int h, bool test)
{
	LONG lStatus = DISP_CHANGE_SUCCESSFUL;

	DEVMODE	mode;
	if (GetDisplaySettings(mode, deviceName, x, y, w, h))
	{
		DWORD dwflags = test ? CDS_TEST : CDS_UPDATEREGISTRY;
		lStatus = ChangeDisplaySettingsEx(deviceName.c_str(), &mode, NULL, dwflags, NULL);
	}

	return lStatus;
}

HRESULT ChangeDisplayResolution()
{
	for(int i = 0; i < g_screens.size(); i++)
	{ 
		DEVMODE devmode;
		ZeroMemory(&devmode, sizeof(devmode));
		DisplayInfo& display = g_screens[i];
		BOOL ret = EnumDisplaySettings(display.monitor.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
		if (ret)
		{
			int width = display.size.cx;
			int height = display.size.cy;
			LONG test = ChangeDisplayResolution(display.monitor.szDevice, devmode.dmPosition.x, devmode.dmPosition.y, width, height, true);
			if (test == DISP_CHANGE_SUCCESSFUL)
			{
				test = ChangeDisplayResolution(display.monitor.szDevice, devmode.dmPosition.x, devmode.dmPosition.y, width, height, false);
				if (test != DISP_CHANGE_SUCCESSFUL)
				{
					wchar_t stringBuf[STR_BUF_SIZE];
					swprintf(stringBuf, STR_BUF_SIZE, L"Change display resolution failed.\n%s : width=%d height=%d", display.monitor.szDevice, width, height);
					Exception(stringBuf, S_FALSE);
				}
			}
			else
			{
				wchar_t stringBuf[STR_BUF_SIZE];
				swprintf(stringBuf, STR_BUF_SIZE, L"Does not change to the display resolution.\n%s : width=%d height=%d", display.monitor.szDevice, width, height);
				Exception(stringBuf, S_FALSE);
			}
		}
	}
	return S_OK;
}

/*-------------------------------------------
	
--------------------------------------------*/
static int FindAdapter()
{
	HRESULT hr = E_FAIL;

	IDirect3D9Ex* dd = 0;
	if (FAILED(hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &dd)))
		Exception(L"[FindAdapter] Direct3DCreate9 failed.", hr);

	if (dd == NULL)
		Exception(L"[FindAdapter] Direct3DCreate9 D3D=NULL.", hr);

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

static D3DCOLOR GetClearColor(UINT index)
{
	D3DCOLOR ClearColor[] =
	{
		D3DCOLOR_XRGB(0, 0, 255),
		D3DCOLOR_XRGB(0, 255, 0),
		D3DCOLOR_XRGB(255, 0, 0),
		D3DCOLOR_XRGB(255, 255, 0),
	};

	int num = _countof(ClearColor);
	return ClearColor[index % num];
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

	D3DXVECTOR3* pos = &sPos[index];
	pos->x += velocity;
	if (pos->x > g_screens[index].size.cx)
	{
		D3DSURFACE_DESC desc;
		if (tex && SUCCEEDED(tex->GetLevelDesc(0, &desc)))
		{
			pos->x = (float)(desc.Width) * -1.0f;
		}
	}

	if (pos->y == 0)
	{
		D3DSURFACE_DESC desc;
		if (tex && SUCCEEDED(tex->GetLevelDesc(0, &desc)))
		{
			pos->y = (float)(g_screens[index].size.cy / 2 - desc.Height / 2);
		}
	}

	return *pos;
}

/*-------------------------------------------
	
--------------------------------------------*/
static D3DDISPLAYMODEEX *SetupDisplayModeEx(D3DPRESENT_PARAMETERS *pdpp, UINT numberOfAdaptersInGroup)
{
	if (numberOfAdaptersInGroup == 0 || pdpp == nullptr)
		return nullptr;

	for (UINT i = 0; i < numberOfAdaptersInGroup; ++i)
	{
		if (pdpp[i].Windowed == TRUE)
			return nullptr;
	}

	D3DDISPLAYMODEEX* dm = new D3DDISPLAYMODEEX[numberOfAdaptersInGroup];
	ZeroMemory(dm, sizeof(D3DDISPLAYMODEEX) * numberOfAdaptersInGroup);

	for( UINT i = 0; i < numberOfAdaptersInGroup; ++i )
	{
		D3DDISPLAYMODEEX *p = &dm[i];
		p->Size = sizeof(D3DDISPLAYMODEEX);
		p->Format = pdpp[i].BackBufferFormat;
		p->Height = pdpp[i].BackBufferHeight;
		p->Width = pdpp[i].BackBufferWidth;
		p->ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
		p->RefreshRate = pdpp[i].FullScreen_RefreshRateInHz;
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

	DisplayInfo* info = 0;
	for (int i = 0; i < g_screens.size(); i++)
	{
		std::string szDevice = WStringToString(monitorInfo.szDevice);
		if (g_screens[i].deviceName.compare(szDevice) == 0)
		{
			info = &g_screens[i];
		}
	}
	if (info != 0)
	{
		info->size.cx = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
		info->size.cy = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
		info->monitor = monitorInfo;
	}

	return TRUE;	// Enumerate next monitor if TRUE.
}

/*-------------------------------------------

--------------------------------------------*/
void CreateDisplayInfo()
{
	IDirect3D9Ex* dd = 0;
	Direct3DCreate9Ex(D3D_SDK_VERSION, &dd);
	UINT count = dd->GetAdapterCount();
	int masterAdapter = FindAdapter();
	for (UINT adapter = 0; adapter < count; adapter++)
	{
		D3DCAPS9 pCaps;
		dd->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &pCaps);
		if (masterAdapter != pCaps.MasterAdapterOrdinal)
			continue;

		D3DADAPTER_IDENTIFIER9 id;
		if (dd->GetAdapterIdentifier(pCaps.AdapterOrdinalInGroup, 0, &id) == D3D_OK)
		{
			DisplayInfo info;
			info.deviceName = id.DeviceName;
			g_screens.push_back(info);
		}
	}
	dd->Release();

	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

	for (int i = 0; i < g_screens.size(); i++)
	{
		if (g_screens[i].size.cx == 0 && g_screens[i].size.cy == 0)
		{
			wchar_t stringBuf[STR_BUF_SIZE];
			swprintf(stringBuf, STR_BUF_SIZE, L"[CreateDisplayInfo] Invalid screen : %s", g_screens[i].deviceName);
			Exception(stringBuf, S_FALSE);
		}
	}
}

/*-------------------------------------------

--------------------------------------------*/
void InitFont(LPDIRECT3DDEVICE9 pDevice)
{
	D3DXCreateFont(pDevice, 24, 0, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL, &g_pFont);
}
void DrawText(LPCWSTR text, int x, int y, int w, int h)
{
	RECT rect = { x, y, x + w, y + h };
	g_pFont->DrawText(NULL, text, -1, &rect, DT_LEFT | DT_TOP, D3DCOLOR_ARGB(255, 255, 255, 255));
}
void ReleaseFont()
{
	if (g_pFont) {
		g_pFont->Release();
		g_pFont = NULL;
	}
}

void DrawDebugInfo()
{
	#define ENDL std::endl; line++;

	std::wstringstream wss;
	int line = 0;

	wss << L"Build : " << __DATE__ << " " << __TIME__ << ENDL;
	wss << L"FPS : " << g_fps << ENDL
	wss << L"Objects : " << g_objCount << ENDL
	if (g_rebootSec > 0)
	{
		wss << L"Reboot Count : " << g_rebootCount << ENDL
	}
	for (int i = 0; i < g_screens.size(); i++)
	{
		wss << L"Screen " << i << L" : " << g_screens[i].size.cx << L"x" << g_screens[i].size.cy << ENDL
	}
	if (g_backBufferCount > 0)
	{
		wss << L"Back Buffer : " << g_backBufferCount << ENDL
	}
	if (g_multiSampleType > 0)
	{
		wss << L"MSAA : " << g_multiSampleType << ENDL
	}
	if (g_noWaitMode)
	{
		wss << L"No Wait Mode" << ENDL
	}
	DrawText(wss.str().c_str(), 30, 30, 300, (30 * line));
}

/*-------------------------------------------

--------------------------------------------*/
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
		Exception(L"[InitApp] RegisterClass failed.", GetLastError());

	CreateDisplayInfo();

	SetCommandLineArgs();

	if (ChangeDisplayResolution() != S_OK)
		Exception(L"[InitApp] ChangeDisplayResolution failed.", S_FALSE);

	// Create main window.
	g_hWindow.resize(g_screens.size());
	for(int i = 0; i < g_hWindow.size(); i++ )
	{
		g_hWindow[i] = CreateWindow(g_szWndClass, g_szAppTitle,
				WS_POPUP,
				0, 0, g_screens[i].size.cx, g_screens[i].size.cy,
				NULL, NULL, hInst, NULL);
		if (g_hWindow[i] == NULL)
			Exception(L"[InitApp] g_hWindow == NULL.", GetLastError());

		ShowWindow(g_hWindow[i], SW_SHOWNORMAL);
		UpdateWindow(g_hWindow[i]);
	}

	resetSync();

	return S_OK;
}

/*-------------------------------------------

--------------------------------------------*/
HRESULT InitDXGraphics()
{
	HRESULT hr = E_FAIL;
	if (FAILED(hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3D)))
		Exception(L"[InitDXGraphics] Direct3DCreate9 failed.", hr);

	if (g_pD3D == NULL)
		Exception(L"[InitDXGraphics] Direct3DCreate9 D3D=NULL.", hr);

	D3DPRESENT_PARAMETERS val = { 0 };
	g_D3DPP.resize(g_screens.size(), val);

	DWORD BehaviorFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
	BehaviorFlags |= D3DCREATE_MULTITHREADED;
	//BehaviorFlags |= D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX;
	if (g_screens.size() >= 2)
	{
		BehaviorFlags |= D3DCREATE_ADAPTERGROUP_DEVICE;
	}

	UINT depthWidth = 0, depthHeight = 0;
	for( int i = 0; i < g_D3DPP.size(); i++ )
	{
		g_D3DPP[i].BackBufferWidth				= g_screens[i].size.cx;
		g_D3DPP[i].BackBufferHeight				= g_screens[i].size.cy;
		g_D3DPP[i].BackBufferFormat				= g_formatFull;
		g_D3DPP[i].BackBufferCount				= g_backBufferCount;
		g_D3DPP[i].MultiSampleType				= g_multiSampleType;
		g_D3DPP[i].MultiSampleQuality			= 0;
		g_D3DPP[i].hDeviceWindow				= g_hWindow[i];
		g_D3DPP[i].SwapEffect					= D3DSWAPEFFECT_DISCARD;
		g_D3DPP[i].Windowed                     = FALSE;
		g_D3DPP[i].FullScreen_RefreshRateInHz	= 60;
//		g_D3DPP[i].FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
		{
			// Multihead (Direct3D 9)
			// https://learn.microsoft.com/en-us/windows/win32/direct3d9/multihead
			// if EnableAutoDepthStencil is TRUE, then each of the following fields must have the same value for each D3DPRESENT_PARAMETERS:
			//		AutoDepthStencilFormat
			//		BackBufferWidth
			//		BackBufferHeight
			//		BackBufferFormat
			g_D3DPP[i].EnableAutoDepthStencil		= FALSE;
			g_D3DPP[i].AutoDepthStencilFormat		= D3DFMT_D24S8;
		}
		g_D3DPP[i].Flags						= 0;
//		g_D3DPP[i].PresentationInterval			= D3DPRESENT_INTERVAL_IMMEDIATE;
		g_D3DPP[i].PresentationInterval			= D3DPRESENT_INTERVAL_ONE;

		depthWidth = max(depthWidth, g_D3DPP[i].BackBufferWidth);
		depthHeight = max(depthHeight, g_D3DPP[i].BackBufferHeight);
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
			if (dm)
			{
				delete[] dm;
			}
			Exception(L"[InitDXGraphics] CreateDevice failed.", hr);
		}
	}

	if(dm)
	{
		free(dm);
	}

	// Create Z Buffer and Stencil Buffer manually.
	IDirect3DSurface9* zStencilSurface = nullptr;
	g_pD3DDevice->CreateDepthStencilSurface(
		depthWidth, depthHeight,
		D3DFMT_D24S8, // 24bit Z + 8bit Stencil
		D3DMULTISAMPLE_NONE,
		0,
		TRUE,
		&zStencilSurface,
		nullptr);

	g_pD3DDevice->SetDepthStencilSurface(zStencilSurface);


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
		Exception(L"InitDXGraphics SetViewport", hr);

	// Create a sprite.
	hr = D3DXCreateTextureFromFile(g_pD3DDevice, g_szSpriteFile, &g_pD3DTexture);
	if (FAILED(hr))
		Exception(L"[InitDXGraphics] D3DXCreateTextureFromFile failed.", hr);

	hr = D3DXCreateTextureFromFile(g_pD3DDevice, g_szBgTextureFile, &g_pBgTexture);
	if (FAILED(hr))
		Exception(L"[InitDXGraphics] D3DXCreateTextureFromFile(BgTexture) failed.", hr);

	hr = D3DXCreateTextureFromFile(g_pD3DDevice, g_szTestTextureFile, &g_pTestTexture);
	if (FAILED(hr))
		Exception(L"[InitDXGraphics] D3DXCreateTextureFromFile(FontTexture) failed.", hr);
	{
		D3DSURFACE_DESC desc;
		if (SUCCEEDED(g_pTestTexture->GetLevelDesc(0, &desc))) {
			g_testTextureSize.cx = desc.Width;
			g_testTextureSize.cy = desc.Height;
		}
	}

	hr = D3DXCreateSprite(g_pD3DDevice, &g_pD3DXSprite);
	if (FAILED(hr))
		Exception(L"[InitDXGraphics] D3DXCreateSprite failed.", hr);

	return S_OK;
}

/*-------------------------------------------

--------------------------------------------*/
HRESULT InitD3DObject(void)
{
	InitFont(g_pD3DDevice);

	if (g_pD3DXSprite)
		g_pD3DXSprite->OnResetDevice();

	return S_OK;
}

/*--------------------------------------------

--------------------------------------------*/
BOOL BeginScene(int index)
{
	if (SUCCEEDED(g_pD3DDevice->BeginScene()))
	{
		g_pD3DDevice->SetTexture(0, NULL);
		g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, ~0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, ~0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, ~0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, ~0);
		g_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		g_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		g_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
		g_pD3DDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);

		g_pD3DDevice->Clear(
			0,
			nullptr,
			D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER,	// Clear buffer types.
			GetClearColor(index),									// Clear a render target to this ARGB color.
			1.0f,													// Clear the depth buffer to this new z value which ranges from 0 to 1.
			0);														// Clear the stencil buffer to this new value which ranges from 0 to 2ⁿ-1 (n is the bit depth of the stencil buffer).

		g_pD3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		g_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		g_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		g_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		g_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		g_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		g_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

		g_pD3DDevice->SetStreamSource(0, NULL, 0, 0);

		g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		g_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

		return TRUE;
	}
	return FALSE;
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
				Exception(L"GetSwapChain() failed.", hr);
			}
		}
		IDirect3DSurface9 *pBackBuffer = NULL;
		{
			HRESULT hr = pChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
			if (FAILED(hr))
			{
				Exception(L"GetRenderTarget() failed.", hr);
			}
		}

		HRESULT hr = g_pD3DDevice->SetRenderTarget(0, pBackBuffer);
		if (FAILED(hr))
		{
			Exception(L"SetRenderTarget() failed.", hr);
		}

		// Draw scene
		if (BeginScene(i))
		{
			// Draw rotation box
			int x = g_D3DPP[i].BackBufferWidth / 2;
			int y = g_D3DPP[i].BackBufferHeight / 2;
			int h = (x > y ? x : y) * 2;
			DrawRotationBox(x, y, 100, h, GetClearColor(i - 1));

			if (i == 0)
			{
				DrawTestTexture(1920/2, 1080/2);
			}

			// Draw sprite
			if( g_pD3DXSprite )
			{
				g_pD3DXSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DONOTSAVESTATE);
				if (i == 0)
				{
					static D3DXVECTOR3* pos = new D3DXVECTOR3(0.0, 0.0, 0.0);
					for (int num = 0; num < g_objCount; num++)
					{
						pos->x = (float)(num % 150) * 10;
						pos->y = (float)(num / 150) * 10;
						g_pD3DXSprite->Draw(g_pBgTexture, NULL, NULL, pos, D3DCOLOR_ARGB(255, 255, 255, 255));
					}
				}
				g_pD3DXSprite->Draw(g_pD3DTexture, NULL, NULL, &GetScrollPos(i, 5.0f, g_pD3DTexture), D3DCOLOR_ARGB(255, 255, 255, 255));
				g_pD3DXSprite->End();
			}

			// Draw debug info.
			if (i == 0)
			{
				DrawDebugInfo();
			}

			g_pD3DDevice->EndScene();
		}
	}

	bool needSync = preSync();

	if (needSync && FAILED(g_pD3DDevice->Present(NULL, NULL, NULL, NULL)))
	{
		return S_FALSE;
	}

	postSync();

	if (g_need_foreground) {
		SetAbsoluteForegroundWindow(g_hWindow[0]);
		g_need_foreground--;
	}

	return S_OK;
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
			Exception(L"[ChangeWindowSize] ResetEx() failed.", hr);
			for(int i = 0; i < g_hWindow.size(); i++ )
			{
				DestroyWindow(g_hWindow[i]);
			}
		}
	}
	hr = InitD3DObject();
	if (FAILED(hr))
	{
		Exception(L"[ChangeWindowSize] InitD3DObject failed.", hr);
		for(int i = 0; i < g_hWindow.size(); i++ )
		{
			DestroyWindow(g_hWindow[i]);
		}
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
		Exception(L"[ChangeWindowSize] SetViewport failed.", hr);
	}
	return hr;
}

/*-------------------------------------------

--------------------------------------------*/
bool CleanupDXGraphics(void)
{
	ReleaseFont();

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

	for (int i = 0; i < g_hWindow.size(); i++)
	{
		DestroyWindow(g_hWindow[i]);
	}

	return true;
}

/*-------------------------------------------

--------------------------------------------*/
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, UINT wParam, LONG lParam)
{
	HRESULT hr = S_OK;

	switch(msg)
	{
	case WM_CREATE:
		SetFocus(hWnd);
		SetForegroundWindow(hWnd);
		break;

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

		case VK_LEFT:
			g_objCount = max(0, g_objCount - 1);
			break;
		case VK_RIGHT:
			g_objCount = min(g_objCount + 1, INT_MAX);
			break;
		case VK_UP:
			g_objCount = min(g_objCount + 10, INT_MAX);
			break;
		case VK_DOWN:
			g_objCount = max(0, g_objCount - 10);
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

				Exception(L"[AppIdle] ResetEx failed.", hr);
				return false;
			}
			hr = InitD3DObject();
			if (FAILED(hr))
			{
				Exception(L"[AppIdle] InitD3DObject failed.", hr);
				return false;
			}
		}
		g_bDeviceLost = false;
	}

	MeasureFramePerSecond();

	hr = Render();
	if (hr == D3DERR_DEVICELOST)
		g_bDeviceLost = true;
	else if (FAILED(hr))
		return false;

	if (g_rebootSec > 0 && !IsDebuggerPresent())
	{
		if (g_frameCount == 60 * g_rebootSec)
		{
			RebootTriggered();
		}
	}

	++g_frameCount;

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
		Exception(L"[WinMain] InitApp failed.", hr);
	}

	hr = InitDXGraphics();
	if (FAILED(hr))
	{
		Exception(L"[WinMain] InitDXGraphics failed.", hr);
	}
	else
	{
		hr = InitD3DObject();
		if (FAILED(hr))
		{
			Exception(L"[WinMain] InitD3DObject failed.", hr);
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
				break;
			}
		}
	} while (msg.message != WM_QUIT);

	CleanupApp();

	if (g_rebootTriggered)
	{
		RebootProcess();
	}

	return (int)msg.wParam;
}

/*--------------------------------------------

---------------------------------------------*/
int RebootProcess()
{
	char path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);

	STARTUPINFOA si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	++g_rebootCount;
	std::string cmd = GetCommandLineA();
	std::string extra = " --rebootCount " + std::to_string((_ULonglong)g_rebootCount);
	std::string newCmd = cmd + extra;

	if (CreateProcessA(
		path,
		const_cast<LPSTR>(newCmd.c_str()),
		NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		return 0;
	}
	else {
		MessageBoxA(NULL, "Reboot failed.", "Error", MB_OK);
		return 1;
	}
}

void RebootTriggered()
{
	if (DestroyWindow(g_hWindow[0]))
	{
		g_rebootTriggered = true;
	}	
}

void MeasureFramePerSecond()
{
	// 静的変数で前回のカウンタ値を保持
	static LARGE_INTEGER s_lastCounter = { 0 };
	static LARGE_INTEGER s_frequency = { 0 };
	static int newFps = 0;
	static double mTimeCount = 0;

	// 初回のみ周波数を取得
	if (s_frequency.QuadPart == 0) {
		QueryPerformanceFrequency(&s_frequency);
		QueryPerformanceCounter(&s_lastCounter);
	}

	// 毎フレーム呼び出す
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	double deltaTime = (double)(now.QuadPart - s_lastCounter.QuadPart) * 1000.0 / s_frequency.QuadPart;
	s_lastCounter = now;

	mTimeCount += deltaTime;
	newFps++;

	if (mTimeCount >= 1000)
	{
		mTimeCount = 0;
		g_fps = newFps;
		newFps = 0;
	}
}

#define SYNC_FRAMES 16
#define TIME_RES 1
#define WAIT_SLEEP_THRESHOLD (2*TIME_RES)
#define WAIT_YIELD_THRESHOLD (1*TIME_RES)

static struct {
	ULONG syncTime[SYNC_FRAMES];
	ULONG cur;
	ULONG last;
}d;

ULONG GetTick()
{
	MMTIME time;
	timeGetSystemTime(&time, sizeof(time));
	return time.u.ms;
}


ULONG Wait(ULONG target, ULONG t0 = GetTick())
{
	if (target <= 0) {
		ULONG t1 = GetTick();
		return t1 - t0;
	}
	if (target >= 1000 * TIME_RES) {		// mistake?
		target = 1000 * TIME_RES;
	}


	ULONG diff;
	while (1) {
		ULONG t1 = GetTick();
		diff = t1 - t0;
		if (diff < 0) {
			return diff; 					// Overflow?
		}
		if (diff >= target) {				// Exit when the target time is reached.
			return diff;
		}

		if (target - diff > WAIT_SLEEP_THRESHOLD) {
			Sleep(1);						// Sleep and wait if afford it.
		}
		else if (target - diff > WAIT_YIELD_THRESHOLD) {
			Sleep(0);						// Just switching if not afford it.
		}
		else {
		}
	}
}

void resetSync()
{
	d.cur = 0;
	d.last = 0;

	for (UINT i = 0; i < SYNC_FRAMES; i++) {
		d.syncTime[i] = GetTick();
	}
}

void Sync(UINT refresh)
{
	ULONG cur = GetTick();
	ULONG diff = cur - d.syncTime[d.last];
	ULONG one = 1000 * TIME_RES / refresh;
	if (one > diff && one - diff > 2 * TIME_RES) {
		ULONG n = (one - diff - TIME_RES);
		Wait(n);
	}
}

bool preSync()
{
	if (g_noWaitMode)
		return true;

	Sync(60);
	return true;
}

void postSync()
{
	d.syncTime[d.cur] = GetTick();

	d.last = d.cur;
	d.cur = (d.cur + 1) % SYNC_FRAMES;
}

/*--------------------------------------------

--------------------------------------------*/
typedef struct
{
	D3DXVECTOR4 pos;
	DWORD color;
} CUSTOM_VERTEX;

const DWORD BOXFVF = (D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

void DrawRotationBox(int x, int y, int w, int h, DWORD color)
{
	float px = (float)(w >> 1), py = (float)(h >> 1);
	CUSTOM_VERTEX v[] =
	{
		{ D3DXVECTOR4(-px, -py, .0f, 1.0f), color},
		{ D3DXVECTOR4( px, -py, .0f, 1.0f), color},
		{ D3DXVECTOR4(-px,  py, .0f, 1.0f), color},
		{ D3DXVECTOR4(-px,  py, .0f, 1.0f), color},
		{ D3DXVECTOR4( px, -py, .0f, 1.0f), color},
		{ D3DXVECTOR4( px,  py, .0f, 1.0f), color},
	};

	float angle = D3DXToRadian(g_frameCount);
	size_t size = sizeof(v) / sizeof(v[0]);
	for (int i = 0; i < size; i++)
	{
		float cosA = std::cos(angle);
		float sinA = std::sin(angle);
		float posx = v[i].pos.x * cosA - v[i].pos.y * sinA;
		float posy = v[i].pos.x * sinA + v[i].pos.y * cosA;
		v[i].pos.x = x + posx;
		v[i].pos.y = y + posy;
	}

	g_pD3DDevice->SetFVF(BOXFVF);
	g_pD3DDevice->SetTexture(0, 0);
	g_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, v, sizeof(CUSTOM_VERTEX));
}

/*--------------------------------------------

--------------------------------------------*/
static void SetAbsoluteForegroundWindow(HWND hWnd)
{
	int nTargetID, nForegroundID;
	DWORD sp_time;

	// フォアグラウンドウィンドウを作成したスレッドのIDを取得
	nForegroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	// 目的のウィンドウを作成したスレッドのIDを取得
	nTargetID = GetWindowThreadProcessId(hWnd, NULL);
	// スレッドのインプット状態を結び付ける
	AttachThreadInput(nTargetID, nForegroundID, TRUE);	// TRUE で結び付け
	// 現在の設定を sp_time に保存
	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &sp_time, 0);
	// ウィンドウの切り替え時間を 0ms にする
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, static_cast<LPVOID>(0), 0);
	// ウィンドウをフォアグラウンドに持ってくる
	SetForegroundWindow(hWnd);
	// 設定を元に戻す
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, reinterpret_cast<PVOID>(sp_time), 0);
	// スレッドのインプット状態を切り離す
	AttachThreadInput(nTargetID, nForegroundID, FALSE);	// FALSE で切り離し
}

/*--------------------------------------------

--------------------------------------------*/
typedef struct
{
	float x, y, z, w;
	DWORD c;
	float s, t;
} TESTTEX_VERTEX;

const DWORD TestTexFVF = (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

void DrawTestTexture(int x, int y)
{	
	float fx = (float)x, fy = (float)y, fw = (float)g_testTextureSize.cx, fh = (float)g_testTextureSize.cy;
	DWORD color = 0xFFFFFFFF;
	TESTTEX_VERTEX v[4] =
	{
		{ 0.0f, 0.0f, 0.0f, 1.0f, color, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f, color, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f, color, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f, color, 1.0f, 1.0f },
	};

	v[0].x = fx - 0.5f;
	v[0].y = fy - 0.5f;
	v[1].x = fx + fw - 0.5f;
	v[1].y = fy - 0.5f;
	v[2].x = fx - 0.5f;
	v[2].y = fy + fh - 0.5f;
	v[3].x = fx + fw - 0.5f;
	v[3].y = fy + fh - 0.5f;

	g_pD3DDevice->SetFVF(TestTexFVF);
	g_pD3DDevice->SetTexture(0, g_pTestTexture);
	g_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(TESTTEX_VERTEX));
}