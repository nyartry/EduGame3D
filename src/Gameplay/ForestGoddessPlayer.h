#pragma once

#include "Models/SolidStaticMeshActor.h"

class ForestGoddessPlayer : public SolidStaticMeshActor
{
protected:
	const SolidStaticMeshActorDefinition& GetSolidStaticMeshDefinition() const override;
};
