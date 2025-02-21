#include "Window.h"
#include "DirectX.h"
#include <vector>

void FinishApp()
{
	ReleaseAllTexture();
	ReleaseDirectX();
}

bool LoadTexture()
{
	static std::vector<const char*> file_list;
	if (file_list.empty())
	{
		file_list.push_back("data/2d/Cord.dds");
		file_list.push_back("data/2d/Mask.dds");
	}

	for (auto itr = file_list.begin(); itr != file_list.end(); itr++)
	{
		if (LoadingTexture((*itr)) == false)
		{
			FinishApp();
			return false;
		}
	}
	return true;
}

void Render()
{
	StartRendering();

	SetPixelShader(FVF_XYZ_C);

	int level = 1, x = 0, y = 0, width = 0, height = 0;
	GetTextureSize("data/2d/Mask.dds", width, height);

	SetMask(MASK_START_MASK, level, x, y, width, height);
	DrawPrimitive(x, y, width, height);

	SetTexture("data/2d/Cord.dds");
	SetPixelShader(FVF_XYZ_C_TEX);
	SetMask(MASK_START_COLOR, level, x, y, width, height);
	DrawPrimitive(0, 0);

	SetPixelShader(FVF_CONSTANT_C_TEX);
	SetMask(MASK_END, level, x, y, width, height);

	FinishRendering();
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmpLine,
	INT       nCmdShow)
{
	if (InitializeWindow(hInstance, L"Stencil", 512, 512) == false)
		return 0;

	if (InitializeDirectX() == false)
		return 0;

	if (LoadTexture() == false)
		return 0;

	bool is_game_end = false;
	while (is_game_end == false)
	{
		MSG msg;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
			else 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else 
		{
			Render();
		}
	}

	FinishApp();

	return 0;
}
