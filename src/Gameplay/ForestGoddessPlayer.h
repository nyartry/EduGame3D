#pragma once

#include "Models/StaticMeshActor.h"

class ForestGoddessPlayer : public StaticMeshActor
{
protected:
	const StaticMeshActorDefinition& GetStaticMeshDefinition() const override;
};
