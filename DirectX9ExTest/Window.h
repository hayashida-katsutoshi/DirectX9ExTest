#ifndef WINDOW_H_
#define WINDOW_H_

#include <Windows.h>

bool InitializeWindow(HINSTANCE app_handle, LPCWSTR title, int width, int height);

HWND GetWindowHandle();
HINSTANCE GetAppHandle();

#endif
