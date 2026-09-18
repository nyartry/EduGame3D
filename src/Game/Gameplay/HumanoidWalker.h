#pragma once

#include "Framework/Models/SolidSkinnedMeshActor.h"

class HumanoidWalker : public SolidSkinnedMeshActor
{
protected:
	const SolidSkinnedMeshActorDefinition& GetSolidSkinnedMeshDefinition() const override;
};
