#pragma once

#include "Framework/Animation/BoneSkinning.h"

#include <limits>

struct SkinningTestCase
{
	const char* name;
	SkinnedVertex vertex;
	std::vector<DirectX::XMFLOAT4X4> bones;
};

inline DirectX::XMFLOAT4X4 StoreMatrix(const DirectX::XMMATRIX& matrix)
{
	DirectX::XMFLOAT4X4 result;
	DirectX::XMStoreFloat4x4(&result, matrix);
	return result;
}

inline std::vector<SkinningTestCase> CreateSkinningTestCases()
{
	using namespace DirectX;
	SkinnedVertex source;
	source.vertex.position = { 1.0f, 2.0f, 3.0f };
	source.vertex.normal = MathUtils::NormalizeOrDefault({ 1.0f, 1.0f, 0.0f });
	source.vertex.tangent = MathUtils::NormalizeOrDefault({ 1.0f, -1.0f, 0.0f });
	source.boneIndices[0] = 0;
	source.boneWeights[0] = 1.0f;
	std::vector<SkinningTestCase> cases;
	cases.push_back({ "nonuniform scale", source, { StoreMatrix(XMMatrixScaling(2.0f, 1.0f, 0.5f)) } });
	cases.push_back({ "rotation and translation", source, {
		StoreMatrix(XMMatrixScaling(2.0f, 1.0f, 0.5f) * XMMatrixRotationY(0.7f) * XMMatrixTranslation(3.0f, 2.0f, 1.0f)) } });
	cases.push_back({ "singular normal fallback", source, { StoreMatrix(XMMatrixScaling(0.0f, 2.0f, 1.0f)) } });
	source.boneIndices[1] = 1;
	source.boneWeights[0] = 0.25f;
	source.boneWeights[1] = 0.75f;
	cases.push_back({ "weighted normal blend", source, {
		StoreMatrix(XMMatrixScaling(4.0f, 1.0f, 1.0f)), StoreMatrix(XMMatrixScaling(1.0f, 2.0f, 1.0f)) } });
	source.boneWeights[0] = 0.5f;
	source.boneWeights[1] = 0.5f;
	cases.push_back({ "cancelled direction fallback", source, {
		StoreMatrix(XMMatrixIdentity()), StoreMatrix(XMMatrixScaling(-1.0f, -1.0f, 1.0f)) } });
	source.boneIndices[0] = -1;
	source.boneIndices[1] = 99;
	source.boneIndices[2] = 0;
	source.boneIndices[3] = 0;
	source.boneWeights[2] = -0.2f;
	source.boneWeights[3] = std::numeric_limits<float>::quiet_NaN();
	cases.push_back({ "invalid influences", source, { StoreMatrix(XMMatrixScaling(2.0f, 1.0f, 0.5f)) } });
	source.boneIndices[0] = 512;
	source.boneWeights[0] = 1.0f;
	source.boneWeights[1] = 0.0f;
	cases.push_back({ "shared palette limit", source,
		std::vector<XMFLOAT4X4>(513, StoreMatrix(XMMatrixScaling(2.0f, 1.0f, 0.5f))) });
	source.boneIndices[0] = 0;
	cases.push_back({ "extreme finite normal scale", source, { StoreMatrix(XMMatrixScaling(1.0e-20f, 1.0f, 1.0f)) } });
	return cases;
}
