#pragma once

#include "Actor.h"
#include "SkinnedModel.h"

#include <DirectXMath.h>

#include <string_view>

struct SkinnedMeshActorDefinition
{
	std::string_view modelPath;
	std::string_view idleAnimationPath;
	float height{ 1.8f };
	DirectX::XMFLOAT3 initialPosition{ 0.0f, 0.0f, 0.0f };
	float initialRotationY{};
	SkinningMode skinningMode{ SkinningMode::Gpu };
};

class SkinnedMeshActor : public Actor
{
public:
	void Initialize(ID3D12Device* device) override;
	void Update(float deltaTime) override;
	void Draw(Dx12Renderer& renderer) const override;

protected:
	virtual const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const = 0;

	SkinnedModel& GetModel();
	const SkinnedModel& GetModel() const;
	const DirectX::XMFLOAT3& GetPosition() const;
	void SetPosition(const DirectX::XMFLOAT3& position);
	float GetRotationY() const;
	void SetRotationY(float radians);

private:
	SkinnedModel m_model;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	float m_rotationY{};
	bool m_hasIdleAnimation{};
};
