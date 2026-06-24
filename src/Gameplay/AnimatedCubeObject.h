#pragma once

#include "Models/SolidSkinnedMeshActor.h"

class AnimatedCubeObject : public SolidSkinnedMeshActor
{
protected:
	const SolidSkinnedMeshActorDefinition& GetSolidSkinnedMeshDefinition() const override;
};
