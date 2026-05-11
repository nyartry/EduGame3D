#include "DavenPlayer.h"

using namespace DirectX;

const StaticMeshActorDefinition& DavenPlayer::GetStaticMeshDefinition() const
{
	static const StaticMeshActorDefinition definition
	{
		"Content\\Models\\Daven\\t-pose.fbx",
		1.8f,
		XMFLOAT3{ 0.0f, 0.0f, 0.0f }
	};
	return definition;
}
