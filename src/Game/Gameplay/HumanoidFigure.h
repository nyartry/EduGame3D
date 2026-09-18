#pragma once

#include "Framework/Models/SolidStaticMeshActor.h"

class HumanoidFigure : public SolidStaticMeshActor
{
protected:
	const SolidStaticMeshActorDefinition& GetSolidStaticMeshDefinition() const override;
};
