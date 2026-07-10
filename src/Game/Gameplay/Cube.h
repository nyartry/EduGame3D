#pragma once

#include "Game/Gameplay/PrimitiveObject.h"

class Cube : public PrimitiveObject
{
protected:
	std::vector<Vertex> BuildVertices() const override;
};
