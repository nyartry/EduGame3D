#include "Framework/Platform/Win32/Win32InputBackend.h"

#include "Framework/Scene/Input/Input.h"
#include "Framework/Scene/Input/InputWriter.h"

namespace
{
	int ToVirtualKey(InputKey key)
	{
		switch (key)
		{
		case InputKey::Left: return VK_LEFT;
		case InputKey::Right: return VK_RIGHT;
		case InputKey::Up: return VK_UP;
		case InputKey::Down: return VK_DOWN;
		case InputKey::W: return 'W';
		case InputKey::A: return 'A';
		case InputKey::S: return 'S';
		case InputKey::D: return 'D';
		case InputKey::X: return 'X';
		case InputKey::Z: return 'Z';
		case InputKey::Shift: return VK_SHIFT;
		case InputKey::Space: return VK_SPACE;
		case InputKey::Enter: return VK_RETURN;
		case InputKey::Escape: return VK_ESCAPE;
		case InputKey::Count: break;
		}
		return 0;
	}
}

void Win32InputBackend::Update(HWND window, Input& input) const
{
	InputWriter::BeginFrame(input);
	for (std::size_t index = 0; index < static_cast<std::size_t>(InputKey::Count); ++index)
	{
		const InputKey key = static_cast<InputKey>(index);
		InputWriter::SetKey(input, key, (GetAsyncKeyState(ToVirtualKey(key)) & 0x8000) != 0);
	}

	POINT cursorPosition{};
	bool insideClient = false;
	if (GetCursorPos(&cursorPosition) && window != nullptr)
	{
		ScreenToClient(window, &cursorPosition);
		RECT clientRect{};
		if (GetClientRect(window, &clientRect))
		{
			insideClient =
				cursorPosition.x >= clientRect.left &&
				cursorPosition.y >= clientRect.top &&
				cursorPosition.x < clientRect.right &&
				cursorPosition.y < clientRect.bottom;
		}
	}

	InputWriter::SetPointer(
		input,
		(GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0,
		insideClient,
		cursorPosition.x,
		cursorPosition.y);
}
