#pragma once

#include "StaticMeshActor.h"

class DavenPlayer : public StaticMeshActor
{
protected:
	const StaticMeshActorDefinition& GetStaticMeshDefinition() const override;
};
