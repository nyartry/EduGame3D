#pragma once

#include "Models/SkinnedMeshActor.h"

class AnimatedCubeObject : public SkinnedMeshActor
{
protected:
	const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const override;
};
