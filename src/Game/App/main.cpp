#include "Framework/Common/Common.h"
#include "Game/App/Win32Application.h"

#include <exception>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
	try
	{
		return Win32Application::Run(instance, showCommand);
	}
	catch (const std::exception& error)
	{
		MessageBox(nullptr, ToWide(error.what()).c_str(), L"Fatal error", MB_OK | MB_ICONERROR);
		return 1;
	}
}

