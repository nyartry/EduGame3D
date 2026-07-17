#include "Framework/Scene/Input/Input.h"

bool Input::IsDown(InputKey key) const
{
	return m_currentKeys[ToIndex(key)];
}

bool Input::WasPressed(InputKey key) const
{
	const std::size_t index = ToIndex(key);
	return m_currentKeys[index] && !m_previousKeys[index];
}

bool Input::WasReleased(InputKey key) const
{
	const std::size_t index = ToIndex(key);
	return !m_currentKeys[index] && m_previousKeys[index];
}

bool Input::WasAnyPressed() const
{
	for (std::size_t index = 0; index < KeyCount; ++index)
	{
		if (m_currentKeys[index] && !m_previousKeys[index])
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

void Input::BeginFrame()
{
	m_previousKeys = m_currentKeys;
	m_previousLeftMouseDown = m_currentLeftMouseDown;
}

void Input::SetKey(InputKey key, bool isDown)
{
	m_currentKeys[ToIndex(key)] = isDown;
}

void Input::SetPointer(bool leftButtonDown, bool insideClient, int x, int y)
{
	m_currentLeftMouseDown = leftButtonDown;
	m_mouseInsideClient = insideClient;
	m_mouseX = x;
	m_mouseY = y;
}
