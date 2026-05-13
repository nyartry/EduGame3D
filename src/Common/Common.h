#pragma once

#include <Windows.h>

#include <stdexcept>
#include <string>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::runtime_error("HRESULT failure");
	}
}

inline std::wstring ToWide(const char* text)
{
	const int size = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
	std::wstring result(static_cast<size_t>(size), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), size);
	if (!result.empty() && result.back() == L'\0')
	{
		result.pop_back();
	}
	return result;
}

