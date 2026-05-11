#pragma once

#include "SkinnedMeshActor.h"

class DavenPlayer : public SkinnedMeshActor
{
protected:
	const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const override;
};
