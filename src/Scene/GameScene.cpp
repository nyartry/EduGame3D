#include "Scene/GameScene.h"

#include "Gameplay/DavenPlayer.h"
#include "Rendering/Dx12Renderer.h"
#include "Gameplay/ForestGoddessPlayer.h"
#include "Gameplay/NathanWalker.h"
#include "Gameplay/OrcPlayer.h"
#include "Models/SkinnedMeshActor.h"

#include <memory>

using namespace DirectX;

namespace
{
	constexpr float CameraFovYDegrees = 55.0f;
	//constexpr float CameraNearZ = 0.1f;
	//constexpr float CameraFarZ = 100.0f;
	constexpr float CameraNearZ = 0.5f;
	constexpr float CameraFarZ = 50.0f;
}

void GameScene::Initialize(ID3D12Device* device, UINT width, UINT height)
{
	const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	auto followCamera = std::make_unique<FollowCamera>();
	followCamera->SetLens(XMConvertToRadians(CameraFovYDegrees), aspectRatio, CameraNearZ, CameraFarZ);
	m_followCamera = followCamera.get();
	m_camera = std::move(followCamera);

	m_ground.Initialize(device);
	m_originCube.SetPosition(0.0f, 0.0f, 0.0f);
	m_originCube.Initialize(device);

	m_actors.clear();
	auto player = std::make_unique<OrcPlayer>();
	m_followTarget = player.get();
	m_actors.push_back(std::move(player));
	m_actors.push_back(std::make_unique<DavenPlayer>());
	m_actors.push_back(std::make_unique<NathanWalker>());
	//m_actors.push_back(std::make_unique<ForestGoddessPlayer>());
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Initialize(device);
	}
}

void GameScene::Update(float deltaTime, const Input& input)
{
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Update(deltaTime, input);
	}

	if (m_followCamera != nullptr && m_followTarget != nullptr)
	{
		m_followCamera->SetFollowTarget(m_followTarget->GetPosition(), m_followTarget->GetRotationY());
	}
	m_camera->Update(deltaTime, input);
}

void GameScene::Render(Dx12Renderer& renderer) const
{
	m_ground.Draw(renderer);
	m_originCube.Draw(renderer);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Draw(renderer);
	}
}

XMMATRIX GameScene::GetViewProjectionMatrix() const
{
	return m_camera->GetViewProjectionMatrix();
}
