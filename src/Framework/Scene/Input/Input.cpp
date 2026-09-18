#include "Framework/Scene/Input/Input.h"
#include "Framework/Scene/Input/InputWriter.h"

bool Input::IsDown(InputKey key) const
{
	return m_currentKeys[ToIndex(key)];
}

bool Input::WasPressed(InputKey key) const
{
	const std::size_t index = ToIndex(key);
	return m_pressedKeys[index];
}

bool Input::WasReleased(InputKey key) const
{
	const std::size_t index = ToIndex(key);
	return m_releasedKeys[index];
}

bool Input::WasAnyPressed() const
{
	for (std::size_t index = 0; index < KeyCount; ++index)
	{
		if (m_pressedKeys[index])
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
	return m_leftMousePressed;
}

bool Input::WasLeftMouseReleased() const
{
	return m_leftMouseReleased;
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
	m_pressedKeys.fill(false);
	m_releasedKeys.fill(false);
	m_leftMousePressed = false;
	m_leftMouseReleased = false;
}

void Input::SetKey(InputKey key, bool isDown)
{
	const auto index = ToIndex(key);
	m_pressedKeys[index] = m_pressedKeys[index] || (isDown && !m_currentKeys[index]);
	m_releasedKeys[index] = m_releasedKeys[index] || (!isDown && m_currentKeys[index]);
	m_currentKeys[index] = isDown;
}

void Input::SetPointer(bool leftButtonDown, bool insideClient, int x, int y)
{
	m_leftMousePressed = m_leftMousePressed || (leftButtonDown && !m_currentLeftMouseDown);
	m_leftMouseReleased = m_leftMouseReleased || (!leftButtonDown && m_currentLeftMouseDown);
	m_currentLeftMouseDown = leftButtonDown;
	m_mouseInsideClient = insideClient;
	m_mouseX = x;
	m_mouseY = y;
}

void InputWriter::BeginFrame(Input& input)
{
	input.BeginFrame();
}

void InputWriter::SetKey(Input& input, InputKey key, bool isDown)
{
	input.SetKey(key, isDown);
}

void InputWriter::SetPointer(Input& input, bool leftButtonDown, bool insideClient, int x, int y)
{
	input.SetPointer(leftButtonDown, insideClient, x, y);
}
