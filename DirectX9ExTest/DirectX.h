#ifndef DIRECT_X_H_
#define DIRECT_X_H_

#include <d3d9.h>
#include <d3dx9.h>
#include <string>

bool InitializeDirectX();
void ReleaseDirectX();

void StartRendering();
void FinishRendering();

bool LoadingTexture(const char* file_name);
void ReleaseTexture(const char* file_name);
void ReleaseAllTexture();

void DrawPrimitive(int x, int y, int width, int height, DWORD color = 0xffffffff);
void DrawPrimitive(int x, int y, DWORD color = 0xffffffff);

enum MASK_TYPE
{
	MASK_START_MASK = 0,
	MASK_START_COLOR,
	MASK_END,
};
void SetMask(MASK_TYPE type, int level, int x, int y, int width, int height);

enum FVF_TYPE
{
	FVF_CONSTANT_C = 0,
	FVF_CONSTANT_C_TEX = 0,

	FVF_XYZ_C,
	FVF_XYZ_C_TEX,

	FVF_NUM,
};
void SetPixelShader(FVF_TYPE type);

void SetTexture(const char* file_name);
bool GetTextureSize(const char* file_name, int& width, int& height);

#endif

