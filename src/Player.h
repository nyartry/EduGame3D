#pragma once

#include "Input.h"
#include "SkinnedModel.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <string>
#include <string_view>

class Dx12Renderer;
struct ID3D12Device;

struct PlayerModelDefinition
{
	std::string_view modelPath;
	std::string_view idleAnimationPath;
	std::string_view joggingAnimationPath;
	float height{ 1.8f };
	DirectX::XMFLOAT3 initialPosition{ 0.0f, 0.0f, 0.0f };
	float initialRotationY{};
	RootMotionMode rootMotionMode{ RootMotionMode::Ignore };
	SkinningMode skinningMode{ SkinningMode::Gpu };
};

class Player
{
public:
	virtual ~Player() = default;

	void Initialize(ID3D12Device* device);
	void Update(float deltaTime, const Input& input);
	void UpdateIdle(float deltaTime);
	void Draw(Dx12Renderer& renderer) const;
	void SetRootMotionMode(RootMotionMode mode);
	void SetRootMotionVerticalMode(RootMotionVerticalMode mode);
	void SetRootMotionBlendWeight(float weight);

protected:
	virtual const PlayerModelDefinition& GetModelDefinition() const = 0;

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
	RootMotionVerticalMode m_rootMotionVerticalMode{ RootMotionVerticalMode::Apply };
	float m_rootMotionBlendWeight{ 0.5f };
	float m_rotationY{};
	bool m_hasIdleAnimation{};
	bool m_hasJoggingAnimation{};
};
