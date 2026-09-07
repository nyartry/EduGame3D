#pragma once

#include "Framework/Core/Math/MathUtils.h"
#include "Framework/Models/SkinnedModelData.h"

#include <span>

namespace BoneSkinning
{
	inline constexpr std::size_t MaxBones = 512;

	// Positions and tangents use the affine bone transform. Normals use its
	// inverse transpose, preserving perpendicularity under nonuniform scale.
	// A singular/nonfinite transform uses the shared identity normal fallback.
	inline DirectX::XMMATRIX CreateNormalMatrix(const DirectX::XMMATRIX& bone)
	{
		DirectX::XMMATRIX normal;
		MathUtils::TryCreateNormalMatrix(bone, normal);
		return normal;
	}

	inline void BuildNormalPalette(std::span<const DirectX::XMFLOAT4X4> bones,
		std::vector<DirectX::XMFLOAT4X4>& normals)
	{
		normals.resize(std::min(bones.size(), MaxBones));
		for (std::size_t index = 0; index < normals.size(); ++index)
		{
			DirectX::XMStoreFloat4x4(&normals[index], CreateNormalMatrix(DirectX::XMLoadFloat4x4(&bones[index])));
		}
	}

	inline DirectX::XMFLOAT3 NormalizeDirection(const DirectX::XMFLOAT3& value, const DirectX::XMFLOAT3& fallback)
	{
		DirectX::XMFLOAT3 normalized;
		if (MathUtils::TryNormalize(value, normalized) || MathUtils::TryNormalize(fallback, normalized)) return normalized;
		return { 0.0f, 0.0f, 1.0f };
	}

	// Blend unnormalized inverse-transpose normals with the same positive finite
	// weights as position/tangent, then normalize once. CPU and HLSL follow this
	// rule; normalizing each bone's contribution first changes blend directions.
	inline TexturedVertex DeformVertex(const SkinnedVertex& source,
		std::span<const DirectX::XMFLOAT4X4> bones,
		std::span<const DirectX::XMFLOAT4X4> normalBones)
	{
		using namespace DirectX;
		XMVECTOR position = XMVectorZero();
		XMVECTOR normal = XMVectorZero();
		XMVECTOR tangent = XMVectorZero();
		float totalWeight = 0.0f;
		const auto boneCount = std::min({ bones.size(), normalBones.size(), MaxBones });
		const XMVECTOR sourcePosition = XMLoadFloat3(&source.vertex.position);
		const XMVECTOR sourceNormal = XMLoadFloat3(&source.vertex.normal);
		const XMVECTOR sourceTangent = XMLoadFloat3(&source.vertex.tangent);
		for (int slot = 0; slot < 4; ++slot)
		{
			const int index = source.boneIndices[slot];
			const float weight = source.boneWeights[slot];
			if (index < 0 || static_cast<std::size_t>(index) >= boneCount || !std::isfinite(weight) || weight <= 0.0f) continue;
			const XMMATRIX bone = XMLoadFloat4x4(&bones[index]);
			position += XMVector3TransformCoord(sourcePosition, bone) * weight;
			normal += XMVector3TransformNormal(sourceNormal, XMLoadFloat4x4(&normalBones[index])) * weight;
			tangent += XMVector3TransformNormal(sourceTangent, bone) * weight;
			totalWeight += weight;
		}
		TexturedVertex result = source.vertex;
		if (totalWeight > 0.0f)
		{
			XMStoreFloat3(&result.position, position / totalWeight);
			XMStoreFloat3(&result.normal, normal / totalWeight);
			XMStoreFloat3(&result.tangent, tangent / totalWeight);
		}
		result.normal = NormalizeDirection(result.normal, source.vertex.normal);
		result.tangent = NormalizeDirection(result.tangent, source.vertex.tangent);
		return result;
	}
}
