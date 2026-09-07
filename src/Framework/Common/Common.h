#pragma once

#include <Windows.h>
#include "Framework/Core/Diagnostics/Diagnostics.h"

#include <stdexcept>
#include <string>
#include <source_location>
#include <sstream>
#include <iomanip>

inline void ThrowIfFailed(HRESULT hr, std::string_view operation = {},
	const std::source_location location = std::source_location::current())
{
	if (FAILED(hr))
	{
		std::ostringstream message;
		message << (operation.empty() ? location.function_name() : operation)
			<< " failed: HRESULT 0x" << std::hex << std::uppercase << std::setw(8)
			<< std::setfill('0') << static_cast<unsigned long>(hr)
			<< " (" << location.file_name() << ':' << std::dec << location.line() << ')';
		Diagnostics::Write(message.str());
		throw std::runtime_error(message.str());
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
