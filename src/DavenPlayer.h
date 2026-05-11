#pragma once

#include "Player.h"

class DavenPlayer : public Player
{
protected:
	const PlayerModelDefinition& GetModelDefinition() const override;
};
