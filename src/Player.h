#pragma once

#include "Input.h"
#include "SkinnedModel.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <string>

class Dx12Renderer;
struct ID3D12Device;

class Player
{
public:
	void Initialize(ID3D12Device* device);
	void Update(float deltaTime, const Input& input);
	void Draw(Dx12Renderer& renderer) const;
	void SetRootMotionMode(RootMotionMode mode);
	void SetRootMotionVerticalMode(RootMotionVerticalMode mode);
	void SetRootMotionBlendWeight(float weight);

private:
	enum class AnimationState
	{
		Idle,
		Jogging,
	};

	void SetAnimationState(AnimationState state);
	DirectX::XMFLOAT3 ChooseDisplacement(
		const DirectX::XMFLOAT3& inputDisplacement,
		const DirectX::XMFLOAT3& rootMotionDisplacement) const;
	DirectX::XMFLOAT3 TransformRootMotionToWorld(const DirectX::XMFLOAT3& localRootMotion) const;

	SkinnedModel m_model;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	AnimationState m_animationState{ AnimationState::Idle };
	RootMotionMode m_rootMotionMode{ RootMotionMode::Apply };
	RootMotionVerticalMode m_rootMotionVerticalMode{ RootMotionVerticalMode::Ignore };
	float m_rootMotionBlendWeight{ 0.5f };
	float m_rotationY{};
};
