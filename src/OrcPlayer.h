#pragma once

#include "Player.h"

class OrcPlayer : public Player
{
protected:
	const PlayerModelDefinition& GetModelDefinition() const override;
};
