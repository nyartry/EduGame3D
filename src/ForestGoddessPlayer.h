#pragma once

#include "Player.h"

class ForestGoddessPlayer : public Player
{
protected:
	const PlayerModelDefinition& GetModelDefinition() const override;
};
