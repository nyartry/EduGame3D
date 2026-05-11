#include "ForestGoddessPlayer.h"

using namespace DirectX;

const StaticMeshActorDefinition& ForestGoddessPlayer::GetStaticMeshDefinition() const
{
	static const StaticMeshActorDefinition definition
	{
		"Content\\Models\\forest_goddess\\forest_goddess.fbx",
		1.8f,
		XMFLOAT3{ 0.0f, 0.0f, 0.0f }
	};
	return definition;
}
