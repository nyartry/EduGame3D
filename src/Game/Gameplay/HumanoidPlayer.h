#pragma once

#include "Game/Gameplay/Player.h"

class HumanoidPlayer : public Player
{
protected:
	const PlayerDefinition& GetPlayerDefinition() const override;
};
