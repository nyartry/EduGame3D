#pragma once

#include "PrimitiveObject.h"

class Cube : public PrimitiveObject
{
protected:
	std::vector<Vertex> BuildVertices() const override;
};
