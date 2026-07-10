#include "Framework/Models/SkinnedMeshActor.h"

#include "Framework/Rendering/Core/Dx12Renderer.h"

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
	m_rootMotionMode = definition.rootMotionMode;
	m_rootMotionVerticalMode = definition.rootMotionVerticalMode;
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
	const RootMotionDelta rootMotionDelta = m_model.Update(deltaTime);
	if (m_rootMotionMode == RootMotionMode::Apply)
	{
		const DirectX::XMFLOAT3 displacement = TransformRootMotionToWorld(rootMotionDelta.translation);
		m_position.x += displacement.x;
		m_position.y += displacement.y;
		m_position.z += displacement.z;
	}

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

DirectX::XMFLOAT3 SkinnedMeshActor::TransformRootMotionToWorld(const DirectX::XMFLOAT3& localRootMotion) const
{
	const DirectX::XMVECTOR local = DirectX::XMLoadFloat3(&localRootMotion);
	const DirectX::XMVECTOR world = DirectX::XMVector3TransformNormal(local, DirectX::XMMatrixRotationY(m_rotationY));

	DirectX::XMFLOAT3 result{};
	DirectX::XMStoreFloat3(&result, world);
	if (m_rootMotionVerticalMode == RootMotionVerticalMode::Ignore)
	{
		result.y = 0.0f;
	}
	return result;
}
