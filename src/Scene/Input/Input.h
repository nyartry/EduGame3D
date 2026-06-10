#pragma once

#include <Windows.h>

#include <array>

enum class InputKey
{
	Left,
	Right,
	Up,
	Down,
	W,
	A,
	S,
	D,
	X,
	Z,
	Shift,
	Space,
	Enter,
	Escape,
};

class Input
{
public:
	void Update(HWND hwnd);

	bool IsDown(InputKey key) const;
	bool WasPressed(InputKey key) const;
	bool WasReleased(InputKey key) const;
	bool WasAnyPressed() const;
	bool IsLeftMouseDown() const;
	bool WasLeftMousePressed() const;
	bool WasLeftMouseReleased() const;
	bool IsMouseInsideClient() const;
	int GetMouseX() const;
	int GetMouseY() const;

private:
	static int ToVirtualKey(InputKey key);

	std::array<bool, 256> m_currentKeys{};
	std::array<bool, 256> m_previousKeys{};
	bool m_currentLeftMouseDown{};
	bool m_previousLeftMouseDown{};
	bool m_mouseInsideClient{};
	int m_mouseX{};
	int m_mouseY{};
};
