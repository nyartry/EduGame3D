#include "Framework/Scene/Input/Input.h"

#include <Windows.h>

void Input::Update(HWND hwnd)
{
	m_previousKeys = m_currentKeys;
	m_previousLeftMouseDown = m_currentLeftMouseDown;

	for (int key = 0; key < static_cast<int>(m_currentKeys.size()); ++key)
	{
		m_currentKeys[static_cast<size_t>(key)] = (GetAsyncKeyState(key) & 0x8000) != 0;
	}

	m_currentLeftMouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	POINT cursorPosition{};
	if (GetCursorPos(&cursorPosition) && hwnd != nullptr)
	{
		ScreenToClient(hwnd, &cursorPosition);
		m_mouseX = cursorPosition.x;
		m_mouseY = cursorPosition.y;

		RECT clientRect{};
		if (GetClientRect(hwnd, &clientRect))
		{
			m_mouseInsideClient =
				cursorPosition.x >= clientRect.left &&
				cursorPosition.y >= clientRect.top &&
				cursorPosition.x < clientRect.right &&
				cursorPosition.y < clientRect.bottom;
		}
	}
}

bool Input::IsDown(InputKey key) const
{
	return m_currentKeys[static_cast<size_t>(ToVirtualKey(key))];
}

bool Input::WasPressed(InputKey key) const
{
	const size_t virtualKey = static_cast<size_t>(ToVirtualKey(key));
	return m_currentKeys[virtualKey] && !m_previousKeys[virtualKey];
}

bool Input::WasReleased(InputKey key) const
{
	const size_t virtualKey = static_cast<size_t>(ToVirtualKey(key));
	return !m_currentKeys[virtualKey] && m_previousKeys[virtualKey];
}

bool Input::WasAnyPressed() const
{
	for (size_t key = 0; key < m_currentKeys.size(); ++key)
	{
		if (m_currentKeys[key] && !m_previousKeys[key])
		{
			return true;
		}
	}

	return false;
}

bool Input::IsLeftMouseDown() const
{
	return m_currentLeftMouseDown;
}

bool Input::WasLeftMousePressed() const
{
	return m_currentLeftMouseDown && !m_previousLeftMouseDown;
}

bool Input::WasLeftMouseReleased() const
{
	return !m_currentLeftMouseDown && m_previousLeftMouseDown;
}

bool Input::IsMouseInsideClient() const
{
	return m_mouseInsideClient;
}

int Input::GetMouseX() const
{
	return m_mouseX;
}

int Input::GetMouseY() const
{
	return m_mouseY;
}

int Input::ToVirtualKey(InputKey key)
{
	switch (key)
	{
	case InputKey::Left:
		return VK_LEFT;
	case InputKey::Right:
		return VK_RIGHT;
	case InputKey::Up:
		return VK_UP;
	case InputKey::Down:
		return VK_DOWN;
	case InputKey::W:
		return 'W';
	case InputKey::A:
		return 'A';
	case InputKey::S:
		return 'S';
	case InputKey::D:
		return 'D';
	case InputKey::X:
		return 'X';
	case InputKey::Z:
		return 'Z';
	case InputKey::Shift:
		return VK_SHIFT;
	case InputKey::Space:
		return VK_SPACE;
	case InputKey::Enter:
		return VK_RETURN;
	case InputKey::Escape:
		return VK_ESCAPE;
	default:
		return 0;
	}
}
