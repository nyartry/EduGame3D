#include "DavenPlayer.h"

using namespace DirectX;

const PlayerModelDefinition& DavenPlayer::GetModelDefinition() const
{
	static const PlayerModelDefinition definition
	{
		"Content\\Models\\Daven\\t-pose.fbx",
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
