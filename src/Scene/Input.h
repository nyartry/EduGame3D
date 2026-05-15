#pragma once

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
	Z,
	Space,
	Escape,
};

class Input
{
public:
	void Update();

	bool IsDown(InputKey key) const;
	bool WasPressed(InputKey key) const;
	bool WasReleased(InputKey key) const;

private:
	static int ToVirtualKey(InputKey key);

	std::array<bool, 256> m_currentKeys{};
	std::array<bool, 256> m_previousKeys{};
};
