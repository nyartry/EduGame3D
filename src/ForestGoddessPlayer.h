#pragma once

#include "SkinnedMeshActor.h"

class ForestGoddessPlayer : public SkinnedMeshActor
{
protected:
	const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const override;
};
