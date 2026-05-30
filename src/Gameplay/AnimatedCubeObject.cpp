#include "Gameplay/AnimatedCubeObject.h"

using namespace DirectX;

const SkinnedMeshActorDefinition& AnimatedCubeObject::GetSkinnedMeshDefinition() const
{
	static const SkinnedMeshActorDefinition definition
	{
		"Content\\Models\\Untitled\\Untitled.fbx",
		"Content\\Models\\Untitled\\Untitled.fbx",
		0.01f,
		XMFLOAT3{ 0.0f, 0.0f, -2.0f },
		0.0f,
		SkinningMode::Gpu
	};
	return definition;
}
