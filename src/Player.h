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

private:
	enum class AnimationState
	{
		Idle,
		Jogging,
	};

	void SetAnimationState(AnimationState state);

	SkinnedModel m_model;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	AnimationState m_animationState{ AnimationState::Idle };
};
