#pragma once

#include "Models/SolidSkinnedMeshActor.h"

class NathanWalker : public SolidSkinnedMeshActor
{
protected:
	const SolidSkinnedMeshActorDefinition& GetSolidSkinnedMeshDefinition() const override;
};
