#include "Input.h"

#include <Windows.h>

void Input::Update()
{
	m_previousKeys = m_currentKeys;

	for (int key = 0; key < static_cast<int>(m_currentKeys.size()); ++key)
	{
		m_currentKeys[static_cast<size_t>(key)] = (GetAsyncKeyState(key) & 0x8000) != 0;
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
	case InputKey::Space:
		return VK_SPACE;
	case InputKey::Escape:
		return VK_ESCAPE;
	default:
		return 0;
	}
}
