#include "DavenPlayer.h"

using namespace DirectX;

const SkinnedMeshActorDefinition& DavenPlayer::GetSkinnedMeshDefinition() const
{
	static const SkinnedMeshActorDefinition definition
	{
		"Content\\Models\\Daven\\t-pose.fbx",
		{},
		1.8f,
		XMFLOAT3{ 0.0f, 0.0f, 0.0f },
		0.0f,
		SkinningMode::Gpu
	};
	return definition;
}
