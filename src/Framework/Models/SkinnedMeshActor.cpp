#include "Framework/Models/SkinnedMeshActor.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"

#include <string>

namespace
{
	constexpr const char* IdleAnimationName = "Idle";
}

void SkinnedMeshActor::Initialize(IRenderDevice& device)
{
	const SkinnedMeshActorDefinition& definition = GetSkinnedMeshDefinition();
	m_position = definition.initialPosition;
	m_rotationY = definition.initialRotationY;
	SetRootMotionSettings(definition.rootMotion);
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
	const DirectX::XMFLOAT3 displacement = ResolveMovement({}, rootMotionDelta);
	m_position.x += displacement.x;
	m_position.y += displacement.y;
	m_position.z += displacement.z;

	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
}

void SkinnedMeshActor::Draw(IRenderer& renderer) const
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

void SkinnedMeshActor::SetRootMotionSettings(const RootMotionSettings& settings)
{
	m_rootMotion = settings;
}

const RootMotionSettings& SkinnedMeshActor::GetRootMotionSettings() const
{
	return m_rootMotion;
}

void SkinnedMeshActor::SetRootMotionMode(RootMotionMode mode)
{
	m_rootMotion.mode = mode;
}

void SkinnedMeshActor::SetRootMotionVerticalMode(RootMotionVerticalMode mode)
{
	m_rootMotion.verticalMode = mode;
}

void SkinnedMeshActor::SetRootMotionBlendWeight(float weight)
{
	m_rootMotion.blendWeight = weight;
}

DirectX::XMFLOAT3 SkinnedMeshActor::ResolveMovement(
	const DirectX::XMFLOAT3& programDisplacement,
	const RootMotionDelta& rootMotion) const
{
	return ResolveRootMotionDisplacement(programDisplacement, rootMotion, m_rotationY, m_rootMotion);
}
