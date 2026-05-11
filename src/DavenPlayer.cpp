#include "DavenPlayer.h"

using namespace DirectX;

const StaticMeshActorDefinition& DavenPlayer::GetStaticMeshDefinition() const
{
	static const StaticMeshActorDefinition definition
	{
		"Content\\Models\\Daven\\t-pose.fbx",
		1.8f,
		XMFLOAT3{ -3.0f, 0.0f, 0.0f },
		XM_PI
	};
	return definition;
}
