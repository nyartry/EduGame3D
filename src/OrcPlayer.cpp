#include "OrcPlayer.h"

using namespace DirectX;

const PlayerModelDefinition& OrcPlayer::GetModelDefinition() const
{
	static const PlayerModelDefinition definition
	{
		"Content\\Models\\Player\\Orc Idle\\Orc Idle.fbx",
		"Content\\Models\\Player\\Orc Idle\\Orc Idle.fbx",
		"Content\\Models\\Player\\Jogging\\Jogging.fbx",
		1.8f,
		XMFLOAT3{ 0.0f, 0.0f, 0.0f },
		0.0f,
		RootMotionMode::Apply,
		SkinningMode::Gpu
	};
	return definition;
}
