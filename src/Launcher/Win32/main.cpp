#include "Framework/Common/Common.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"
#include "Framework/Core/Diagnostics/ExceptionUtils.h"
#include "Launcher/Win32/Win32Application.h"

int WINAPI wWinMain(
	_In_ HINSTANCE instance,
	_In_opt_ HINSTANCE,
	_In_ PWSTR,
	_In_ int showCommand)
{
	try
	{
		return Win32Application::Run(instance, showCommand);
	}
	catch (...)
	{
		const auto message = DescribeException();
		Diagnostics::Write(message);
		MessageBoxW(nullptr, ToWide(message.c_str()).c_str(), L"Fatal error", MB_OK | MB_ICONERROR);
		return 1;
	}
}
