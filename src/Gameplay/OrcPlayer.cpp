#include "Gameplay/OrcPlayer.h"

using namespace DirectX;

const PlayerDefinition& OrcPlayer::GetPlayerDefinition() const
{
	static const PlayerDefinition definition
	{
		SkinnedMeshActorDefinition
		{
			"Content\\Models\\Player\\Orc Idle\\Orc Idle.fbx",
			"Content\\Models\\Player\\Orc Idle\\Orc Idle.fbx",
			1.8f,
			XMFLOAT3{ 0.0f, 0.0f, 0.0f },
			0.0f,
			SkinningMode::Gpu
		},
		"Content\\Models\\Player\\Jogging\\Jogging.fbx",
		"Content\\Models\\Player\\Mma Kick\\Mma Kick.fbx",
		RootMotionMode::Apply
	};
	return definition;
}
