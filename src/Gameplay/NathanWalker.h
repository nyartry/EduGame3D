#pragma once

#include "Models/SkinnedMeshActor.h"

class NathanWalker : public SkinnedMeshActor
{
protected:
	const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const override;
};
