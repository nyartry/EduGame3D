#include "Framework/Models/SkinnedMeshActor.h"
#include "Framework/Models/ModelAssetCache.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"

#include <string>

namespace
{
	constexpr const char* IdleAnimationName = "Idle";
}

void SkinnedMeshActor::Prepare(ModelAssetCache& assets)
{
	m_prepared = false;
	const SkinnedMeshActorDefinition& definition = GetSkinnedMeshDefinition();
	m_transform = Transform{};
	m_transform.position = definition.initialPosition;
	m_transform.rotationRadians.y = definition.initialRotationY;
	SetRootMotionSettings(definition.rootMotion);
	m_hasIdleAnimation = !definition.idleAnimationPath.empty();

	m_model.Prepare(
		assets,
		std::string(definition.modelPath),
		ModelScaleSettings::NormalizeToHeight(definition.height));
	if (m_hasIdleAnimation)
	{
		m_model.PrepareAnimation(assets, IdleAnimationName, std::string(definition.idleAnimationPath));
	}
	m_prepared = true;
}

void SkinnedMeshActor::Initialize(IRenderDevice& device)
{
	ModelAssetCache assets;
	if (!m_prepared) Prepare(assets);
	m_model.Activate(device, GetSkinnedMeshDefinition().skinningMode);
	if (m_hasIdleAnimation)
	{
		m_model.PlayAnimation(IdleAnimationName);
	}
}

void SkinnedMeshActor::Update(float deltaTime, const Input&)
{
	const RootMotionDelta rootMotionDelta = m_model.Update(deltaTime);
	const DirectX::XMFLOAT3 displacement = ResolveMovement({}, rootMotionDelta);
	m_transform.position.x += displacement.x;
	m_transform.position.y += displacement.y;
	m_transform.position.z += displacement.z;
}

void SkinnedMeshActor::Draw(IRenderer& renderer) const
{
	m_model.Draw(renderer, m_transform.ToMatrix());
}

SkinnedModel& SkinnedMeshActor::GetModel()
{
	return m_model;
}

std::vector<AnimationEvents::Occurrence> SkinnedMeshActor::ConsumeAnimationEvents()
{
	return m_model.ConsumeAnimationEvents();
}

DirectX::XMFLOAT3 SkinnedMeshActor::GetAnimationEventPosition(std::string_view boneName) const
{
	DirectX::XMFLOAT3 local{};
	if (!boneName.empty() && m_model.TryGetBonePositionLocal(boneName, local))
	{
		return m_transform.TransformPoint(local);
	}
	return m_transform.position;
}

const SkinnedModel& SkinnedMeshActor::GetModel() const
{
	return m_model;
}

const DirectX::XMFLOAT3& SkinnedMeshActor::GetPosition() const
{
	return m_transform.position;
}

void SkinnedMeshActor::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_transform.position = position;
}

float SkinnedMeshActor::GetRotationY() const
{
	return m_transform.rotationRadians.y;
}

void SkinnedMeshActor::SetRotationY(float radians)
{
	m_transform.rotationRadians.y = radians;
}

const Transform& SkinnedMeshActor::GetTransform() const
{
	return m_transform;
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
	return ResolveRootMotionDisplacement(programDisplacement, rootMotion, GetRotationY(), m_rootMotion);
}
