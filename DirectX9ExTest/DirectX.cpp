#include <vector>
#include <map>
#include <fstream>
#include "DirectX.h"

// Link required libraries.
#pragma comment( lib, "d3d9.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif
#pragma comment( lib, "dxerr.lib" )
#pragma comment( lib, "dxguid.lib" )

#define SAFE_RELEASE(x) if(x){x->Release();} 

typedef struct
{
	LPDIRECT3DTEXTURE9 texture;
	int width;
	int height;
} TEXTURE_DATA;

typedef struct
{
	D3DXVECTOR4 pos;
	DWORD color;
	D3DXVECTOR2 uv;
} CUSTOM_VERTEX;

const DWORD RenderingFVF = (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

extern LPCWSTR g_WindowClassName;

static LPDIRECT3D9EX	g_pD3DInterface;
static LPDIRECT3DDEVICE9EX g_pD3DDevice;

std::map<std::string, TEXTURE_DATA*> g_TextureList;
static const TEXTURE_DATA* g_pCurrentTexture;

LPDIRECT3DPIXELSHADER9 g_pPixelShader[FVF_NUM] = { 0 };

void StartRendering()
{
	g_pD3DDevice->BeginScene();
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
		D3DCOLOR_ARGB(0, 0, 0, 0xff),							// Clear a render target to this ARGB color.
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
}

void FinishRendering()
{
	g_pD3DDevice->EndScene();

	g_pD3DDevice->Present(nullptr, nullptr, nullptr, nullptr);
}

void SetMask(MASK_TYPE type, int level, int x, int y, int width, int height)
{
	if (type == MASK_START_MASK)
	{
		g_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILREF, 1 << (level - 1));
		g_pD3DDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 1 << (level - 1));
		g_pD3DDevice->SetRenderState(D3DRS_STENCILMASK, 1 << (level - 1));
		g_pD3DDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);

		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, 0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, 0);

		g_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		g_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
		g_pD3DDevice->SetRenderState(D3DRS_ALPHAREF, 0);
	}
	else if (type == MASK_START_COLOR)
	{
		g_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		g_pD3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

		g_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILREF, 1 << (level - 1));
		g_pD3DDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xff);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILMASK, 0xff);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);

		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, ~0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, ~0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, ~0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, ~0);

		g_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
		g_pD3DDevice->SetRenderState(D3DRS_ALPHAREF, 0);
	}
	else
	{
		g_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

		g_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILREF, 0);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xff);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILMASK, 1 << (level - 1));
		g_pD3DDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_ZERO);
		g_pD3DDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_ZERO);

		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, 0);
		g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, 0);

		DrawPrimitive(x, y, width, height);

		//g_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		//g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, ~0);
		//g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, ~0);
		//g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, ~0);
		//g_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, ~0);
	}
}

void SetPixelShader(FVF_TYPE type)
{
	g_pD3DDevice->SetPixelShader(g_pPixelShader[(int)type]);
}

void SetTexture(const char *file_name)
{
	if (file_name == 0)
	{
		g_pD3DDevice->SetTexture(0, NULL);
		g_pCurrentTexture = NULL;
	}
	else
	{
		const TEXTURE_DATA* texture_data = g_TextureList[file_name];

		if (texture_data == nullptr)
		{
			return;
		}
		g_pD3DDevice->SetTexture(0, texture_data->texture);
		g_pCurrentTexture = texture_data;
	}
}

bool GetTextureSize(const char* file_name, int& width, int& height)
{
	const TEXTURE_DATA* texture = g_TextureList[file_name];
	if (texture != nullptr)
	{
		width = texture->width;
		height = texture->height;
		return true;
	}
	return false;
}

void DrawPrimitive(int x, int y, int width, int height, DWORD color)
{
	float fx = (float)x, fy = (float)y, fw = (float)width, fh = (float)height;
	CUSTOM_VERTEX v[6] = 
	{
		{ D3DXVECTOR4(fx     , fy     , .0f, 1.0f), color, D3DXVECTOR2(0.0f, 0.0f) },
		{ D3DXVECTOR4(fx + fw, fy     , .0f, 1.0f), color, D3DXVECTOR2(1.0f, 0.0f) },
		{ D3DXVECTOR4(fx     , fy + fh, .0f, 1.0f), color, D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR4(fx     , fy + fh, .0f, 1.0f), color, D3DXVECTOR2(0.0f, 1.0f) },
		{ D3DXVECTOR4(fx + fw, fy     , .0f, 1.0f), color, D3DXVECTOR2(1.0f, 0.0f) },
		{ D3DXVECTOR4(fx + fw, fy + fh, .0f, 1.0f), color, D3DXVECTOR2(1.0f, 1.0f) },
	};

	g_pD3DDevice->SetFVF(RenderingFVF);

	g_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, v, sizeof(CUSTOM_VERTEX));
}

void DrawPrimitive(int x, int y, DWORD color)
{
	DrawPrimitive(x, y, g_pCurrentTexture->width, g_pCurrentTexture->height, color);
}

bool CreateInterface()
{
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3DInterface)))
	{
		return false;
	}

	return true;
}

void SetUpPresentParameter(HWND window_handle, int widow_width, int window_height, D3DPRESENT_PARAMETERS* present_param)
{
	ZeroMemory(present_param, sizeof(D3DPRESENT_PARAMETERS));

	present_param->BackBufferCount = 1;
	present_param->BackBufferFormat = D3DFMT_X8R8G8B8;
	present_param->Windowed = TRUE;
	present_param->SwapEffect = D3DSWAPEFFECT_DISCARD;
	present_param->EnableAutoDepthStencil = TRUE;
	present_param->AutoDepthStencilFormat = D3DFMT_D24S8;
}

bool CreateGraphicsDevice(D3DPRESENT_PARAMETERS *present_param, HWND window_handle)
{
	if (FAILED(g_pD3DInterface->CreateDeviceEx(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window_handle,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
		present_param,
		NULL,
		&g_pD3DDevice)))
	{
		return false;
	}

	return true;
}

bool CreateViewPort(D3DPRESENT_PARAMETERS* present_param)
{
	D3DVIEWPORT9 view_port;

	view_port.X = 0;
	view_port.Y = 0;
	view_port.Width = present_param->BackBufferWidth;
	view_port.Height = present_param->BackBufferHeight;
	view_port.MinZ = 0.0f;
	view_port.MaxZ = 1.0f;

	if (FAILED(g_pD3DDevice->SetViewport(&view_port)))
	{
		return false;
	}

	return true;
}

HRESULT CreateShader(const char *filename, LPDIRECT3DPIXELSHADER9 *shader, LPD3DXCONSTANTTABLE *constant)
{
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (file.is_open())
	{
		file.seekg(0, std::ios_base::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios_base::beg);
		char* shaderSource = new char[size];
		file.read(shaderSource, size);
		file.close();

		HRESULT hr;
		if (constant != 0)
		{
			if (size > 16 && memchr(shaderSource, 0, 16) != 0)
			{
				const DWORD* source = (const DWORD*)(shaderSource);
				if (FAILED(hr = D3DXGetShaderConstantTable(source, constant)))
				{
					return hr;
				}
			}
		}

		if (FAILED(hr = g_pD3DDevice->CreatePixelShader((DWORD*)shaderSource, shader)))
		{
			return hr;
		}
	}

	return S_OK;
}

HRESULT CreatePixelShader()
{
	HRESULT hr;

	if (FAILED(hr = CreateShader("data/shader/constant_0.pso", &g_pPixelShader[FVF_CONSTANT_C], 0)))
	{
		return hr;
	}

	if (FAILED(hr = CreateShader("data/shader/constant_1.pso", &g_pPixelShader[FVF_CONSTANT_C_TEX], 0)))
	{
		return hr;
	}

	if (FAILED(hr = CreateShader("data/shader/basic_0.pso", &g_pPixelShader[FVF_XYZ_C], 0)))
	{
		return hr;
	}

	LPD3DXCONSTANTTABLE constant = 0;
	if (FAILED(hr = CreateShader("data/shader/basic_1.pso", &g_pPixelShader[FVF_XYZ_C_TEX], &constant)))
	{
		return hr;
	}

	D3DXHANDLE g_mAddColor = constant->GetConstantByName(0, "g_mAddColor");
	if (g_mAddColor == 0)
	{
		return S_FALSE;
	}
	else
	{
		const D3DXVECTOR4* col = new D3DXVECTOR4(.0f, .0f, .0f, .0f);
		constant->SetVector(g_pD3DDevice, g_mAddColor, col);
	}

	return S_OK;
}

bool InitializeDirectX()
{
	D3DPRESENT_PARAMETERS present_param;
	HWND window_handle = FindWindowW(g_WindowClassName, nullptr);
	RECT client_rect;
	GetClientRect(window_handle, &client_rect);

	SetUpPresentParameter(
			window_handle, 
			(client_rect.right - client_rect.left), 
			(client_rect.bottom - client_rect.top),
			&present_param);

	if (CreateInterface() == false)
		return false;

	if (CreateGraphicsDevice(&present_param, window_handle) == false)
		return false;

	if (CreateViewPort(&present_param) == false)
		return false;

	if (FAILED(CreatePixelShader()))
		return false;

	return true;
}

void ReleaseDirectX()
{
	if (g_pD3DDevice != nullptr)
	{
		g_pD3DDevice->Release();
		g_pD3DDevice = nullptr;
	}

	if (g_pD3DInterface != nullptr)
	{
		g_pD3DInterface->Release();
		g_pD3DInterface = nullptr;
	}
}

bool LoadingTexture(const char* file_name)
{
	if (g_TextureList.count(file_name) == 1)
	{
		return true;
	}

	g_TextureList[file_name] = new TEXTURE_DATA();

	D3DXIMAGE_INFO info;
	// 2の階乗じゃないので元のサイズを取得してD3DXCreateTextureFromFileExで使う
	D3DXGetImageInfoFromFileA(file_name, &info);
	
	if (FAILED( D3DXCreateTextureFromFileExA(g_pD3DDevice,
					file_name,
					info.Width,
					info.Height,
					1,
					0,
					D3DFMT_UNKNOWN,
					D3DPOOL_DEFAULT,
					D3DX_DEFAULT,
					D3DX_DEFAULT,
					0x0000ff00,
					nullptr,
					nullptr,
					&g_TextureList[file_name]->texture)))
	{
		return false;
	} 
	else
	{
		D3DSURFACE_DESC desc;

		if( FAILED( g_TextureList[file_name]->texture->GetLevelDesc( 0, &desc ) ))
		{
			g_TextureList[file_name]->texture->Release();
			g_TextureList[file_name]->texture = nullptr;
			return false;
		}
		g_TextureList[file_name]->width = desc.Width;
		g_TextureList[file_name]->height = desc.Height;
	}

	return true;
}

void ReleaseTexture(const char* file_name)
{
	if (g_TextureList.count(file_name) == 0)
	{
		return;
	}

	if (g_TextureList[file_name]->texture != NULL)
	{
		g_TextureList[file_name]->texture->Release();
		TEXTURE_DATA *data = g_TextureList[file_name];
		g_TextureList.erase(file_name);
		delete(data);
	}
}

void ReleaseAllTexture()
{
	for (auto itr = g_TextureList.begin(); itr != g_TextureList.end(); itr++)
	{
		if (g_TextureList[itr->first] == nullptr)
		{
			continue;
		}
		g_TextureList[itr->first]->texture->Release();
		delete(g_TextureList[itr->first]);
	}
	g_TextureList.clear();
}
