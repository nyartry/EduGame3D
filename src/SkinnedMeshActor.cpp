#include "SkinnedMeshActor.h"

#include "Dx12Renderer.h"

#include <string>

namespace
{
	constexpr const char* IdleAnimationName = "Idle";
}

void SkinnedMeshActor::Initialize(ID3D12Device* device)
{
	const SkinnedMeshActorDefinition& definition = GetSkinnedMeshDefinition();
	m_position = definition.initialPosition;
	m_rotationY = definition.initialRotationY;
	m_hasIdleAnimation = !definition.idleAnimationPath.empty();

	m_model.Initialize(
		device,
		std::string(definition.modelPath),
		ModelScaleSettings::NormalizeToHeight(definition.height),
		definition.skinningMode);
	if (m_hasIdleAnimation)
	{
		m_model.AddAnimation(IdleAnimationName, std::string(definition.idleAnimationPath));
		m_model.PlayAnimation(IdleAnimationName);
	}

	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
	m_model.SetRotationY(m_rotationY);
}

void SkinnedMeshActor::Update(float deltaTime, const Input&)
{
	m_model.Update(deltaTime);
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
}

void SkinnedMeshActor::Draw(Dx12Renderer& renderer) const
{
	m_model.Draw(renderer);
}

SkinnedModel& SkinnedMeshActor::GetModel()
{
	return m_model;
}

const SkinnedModel& SkinnedMeshActor::GetModel() const
{
	return m_model;
}

const DirectX::XMFLOAT3& SkinnedMeshActor::GetPosition() const
{
	return m_position;
}

void SkinnedMeshActor::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_position = position;
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
}

float SkinnedMeshActor::GetRotationY() const
{
	return m_rotationY;
}

void SkinnedMeshActor::SetRotationY(float radians)
{
	m_rotationY = radians;
	m_model.SetRotationY(m_rotationY);
}
