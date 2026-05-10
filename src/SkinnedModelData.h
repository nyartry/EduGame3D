#pragma once

#include "TexturedVertex.h"

#include <DirectXMath.h>
#include <string>
#include <vector>

struct SkinnedVertex
{
	TexturedVertex vertex;
	int boneIndices[4]{ -1, -1, -1, -1 };
	float boneWeights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
};

struct SkinnedMeshData
{
	std::vector<SkinnedVertex> vertices;
	std::string baseColorTexturePath;
	std::string opacityTexturePath;
	std::string normalTexturePath;
};

struct BoneData
{
	std::string name;
	int parentIndex{ -1 };
	DirectX::XMFLOAT4X4 offsetMatrix{};
	DirectX::XMFLOAT4X4 localBindTransform{};
};

struct AnimationKey
{
	double time{};
	DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
};

struct BoneAnimation
{
	int boneIndex{ -1 };
	std::vector<AnimationKey> keys;
};

struct AnimationClip
{
	double durationTicks{};
	double ticksPerSecond{ 30.0 };
	std::vector<BoneAnimation> boneAnimations;
};

struct SkinnedModelData
{
	std::vector<SkinnedMeshData> meshes;
	std::vector<BoneData> bones;
	std::vector<AnimationClip> animations;
};
