#include "ForestGoddessPlayer.h"

using namespace DirectX;

const PlayerModelDefinition& ForestGoddessPlayer::GetModelDefinition() const
{
	static const PlayerModelDefinition definition
	{
		"Content\\Models\\forest_goddess\\forest_goddess.fbx",
		{},
		{},
		1.8f,
		XMFLOAT3{ 0.0f, 0.0f, 0.0f },
		0.0f,
		RootMotionMode::Ignore,
		SkinningMode::Gpu
	};
	return definition;
}
