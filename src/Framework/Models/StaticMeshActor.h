#pragma once

#include "Framework/Gameplay/Actor.h"
#include "Framework/Core/Math/Transform.h"
#include "Framework/Models/StaticModel.h"

#include <DirectXMath.h>

#include <string_view>

struct StaticMeshActorDefinition
{
	std::string_view modelPath;
	float height{ 1.8f };
	DirectX::XMFLOAT3 initialPosition{ 0.0f, 0.0f, 0.0f };
	float initialRotationY{};
};

class StaticMeshActor : public Actor
{
public:
	void Prepare(ModelAssetCache& assets) override;
	void Initialize(IRenderDevice& device) override;
	void Update(float deltaTime, const Input& input) override;
	void Draw(IRenderer& renderer) const override;

	const DirectX::XMFLOAT3& GetPosition() const;
	const Transform& GetTransform() const;

protected:
	virtual const StaticMeshActorDefinition& GetStaticMeshDefinition() const = 0;
	void SetPosition(const DirectX::XMFLOAT3& position);

private:
	StaticModel m_model;
	Transform m_transform;
	bool m_prepared{};
};
