#pragma once

class Input;

enum class GameAction
{
	MoveForward,
	MoveBackward,
	MoveLeft,
	MoveRight,
	Jump,
	Attack,
	Confirm
};

namespace GameActions
{
	bool IsDown(const Input& input, GameAction action);
	bool WasPressed(const Input& input, GameAction action);
}
