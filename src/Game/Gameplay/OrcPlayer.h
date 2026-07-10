#pragma once

#include "Game/Gameplay/Player.h"

class OrcPlayer : public Player
{
protected:
	const PlayerDefinition& GetPlayerDefinition() const override;
};
