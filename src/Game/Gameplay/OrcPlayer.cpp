#include "Game/Gameplay/OrcPlayer.h"

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
			SkinningMode::Gpu,
			RootMotionSettings
			{
				.mode = RootMotionMode::Apply,
				.verticalMode = RootMotionVerticalMode::Ignore
			}
		},
		"Content\\Models\\Player\\Jogging\\Jogging.fbx",
		"Content\\Models\\Player\\Mma Kick\\Mma Kick.fbx"
	};
	return definition;
}
