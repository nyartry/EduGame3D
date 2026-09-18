#include "Game/Gameplay/HumanoidFigure.h"

using namespace DirectX;

const SolidStaticMeshActorDefinition& HumanoidFigure::GetSolidStaticMeshDefinition() const
{
	static const SolidStaticMeshActorDefinition definition
	{
		StaticMeshActorDefinition
		{
			"Content\\Models\\EduHuman\\EduHuman_Idle.fbx",
			1.8f,
			XMFLOAT3{ -3.0f, 0.0f, 0.0f },
			XM_PI
		},
		CollisionBodyDefinition{ 0.4f, 1.8f, true }
	};
	return definition;
}
