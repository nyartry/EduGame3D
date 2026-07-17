#pragma once

#include <array>
#include <cstddef>

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
	Count
};

// A platform-neutral snapshot. Platform adapters populate it once per frame;
// game code can read it without knowing about HWND or virtual-key codes.
class Input
{
public:
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
	friend class InputWriter;

	static constexpr std::size_t KeyCount = static_cast<std::size_t>(InputKey::Count);
	static constexpr std::size_t ToIndex(InputKey key) { return static_cast<std::size_t>(key); }

	void BeginFrame();
	void SetKey(InputKey key, bool isDown);
	void SetPointer(bool leftButtonDown, bool insideClient, int x, int y);

	std::array<bool, KeyCount> m_currentKeys{};
	std::array<bool, KeyCount> m_previousKeys{};
	bool m_currentLeftMouseDown{};
	bool m_previousLeftMouseDown{};
	bool m_mouseInsideClient{};
	int m_mouseX{};
	int m_mouseY{};
};
