#pragma once

#include "Player.h"

class OrcPlayer : public Player
{
protected:
	const PlayerDefinition& GetPlayerDefinition() const override;
};
