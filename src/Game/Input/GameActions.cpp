#include "Game/Input/GameActions.h"

#include "Framework/Scene/Input/Input.h"

namespace
{
	InputKey GetPrimaryKey(GameAction action)
	{
		switch (action)
		{
		case GameAction::MoveForward: return InputKey::W;
		case GameAction::MoveBackward: return InputKey::S;
		case GameAction::MoveLeft: return InputKey::A;
		case GameAction::MoveRight: return InputKey::D;
		case GameAction::Jump: return InputKey::Space;
		case GameAction::Attack: return InputKey::X;
		case GameAction::Confirm: return InputKey::Enter;
		}
		return InputKey::Enter;
	}
}

bool GameActions::IsDown(const Input& input, GameAction action)
{
	return input.IsDown(GetPrimaryKey(action));
}

bool GameActions::WasPressed(const Input& input, GameAction action)
{
	if (input.WasPressed(GetPrimaryKey(action)))
	{
		return true;
	}
	return action == GameAction::Confirm && input.WasPressed(InputKey::Space);
}
