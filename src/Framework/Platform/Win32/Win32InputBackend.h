#pragma once

#include <Windows.h>

class Input;

class Win32InputBackend
{
public:
	void Update(HWND window, Input& input) const;
};
