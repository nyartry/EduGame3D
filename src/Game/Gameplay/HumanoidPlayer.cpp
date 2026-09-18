#include "Game/Gameplay/HumanoidPlayer.h"

using namespace DirectX;

const PlayerDefinition& HumanoidPlayer::GetPlayerDefinition() const
{
	static const PlayerDefinition definition
	{
		SkinnedMeshActorDefinition
		{
			"Content\\Models\\EduHuman\\EduHuman_Idle.fbx",
			"Content\\Models\\EduHuman\\EduHuman_Idle.fbx",
			1.8f,
			XMFLOAT3{ 0.0f, 0.0f, 0.0f },
			0.0f,
			SkinningMode::Gpu,
			RootMotionSettings
			{
				.mode = RootMotionMode::Ignore,
				.verticalMode = RootMotionVerticalMode::Ignore
			}
		},
		"Content\\Models\\EduHuman\\EduHuman_Jog.fbx",
		"Content\\Models\\EduHuman\\EduHuman_Kick.fbx"
	};
	return definition;
}
