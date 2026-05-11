#include "ForestGoddessPlayer.h"

using namespace DirectX;

const SkinnedMeshActorDefinition& ForestGoddessPlayer::GetSkinnedMeshDefinition() const
{
	static const SkinnedMeshActorDefinition definition
	{
		"Content\\Models\\forest_goddess\\forest_goddess.fbx",
		{},
		1.8f,
		XMFLOAT3{ 0.0f, 0.0f, 0.0f },
		0.0f,
		SkinningMode::Gpu
	};
	return definition;
}
