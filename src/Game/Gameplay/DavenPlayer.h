#pragma once

#include "Framework/Models/SolidStaticMeshActor.h"

class DavenPlayer : public SolidStaticMeshActor
{
protected:
	const SolidStaticMeshActorDefinition& GetSolidStaticMeshDefinition() const override;
};
