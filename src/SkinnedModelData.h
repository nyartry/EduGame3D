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

struct VectorAnimationKey
{
	double time{};
	DirectX::XMFLOAT3 value{ 0.0f, 0.0f, 0.0f };
};

struct QuaternionAnimationKey
{
	double time{};
	DirectX::XMFLOAT4 value{ 0.0f, 0.0f, 0.0f, 1.0f };
};

struct BoneAnimation
{
	int boneIndex{ -1 };
	std::string boneName;
	std::vector<VectorAnimationKey> translations;
	std::vector<QuaternionAnimationKey> rotations;
	std::vector<VectorAnimationKey> scales;
};

struct AnimationClip
{
	std::string name;
	double durationTicks{};
	double ticksPerSecond{ 30.0 };
	std::vector<BoneAnimation> boneAnimations;
};

struct SkinnedModelData
{
	std::vector<SkinnedMeshData> meshes;
	std::vector<BoneData> bones;
	std::vector<AnimationClip> animations;
	DirectX::XMFLOAT4X4 rootInverseTransform{};
};
