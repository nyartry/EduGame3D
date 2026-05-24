#include "Scene/GameScene.h"

#include "Gameplay/DavenPlayer.h"
#include "Rendering/Dx12Renderer.h"
#include "Gameplay/ForestGoddessPlayer.h"
#include "Gameplay/NathanWalker.h"
#include "Gameplay/OrcPlayer.h"
#include "Gameplay/Player.h"
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
	constexpr XMFLOAT3 PlatformCubePosition{ 0.0f, 0.5f, 2.0f };
}

void GameScene::Load(const SceneLoadContext& context)
{
	const float aspectRatio = static_cast<float>(context.width) / static_cast<float>(context.height);

	auto followCamera = std::make_unique<FollowCamera>();
	followCamera->SetLens(XMConvertToRadians(CameraFovYDegrees), aspectRatio, CameraNearZ, CameraFarZ);
	m_followCamera = followCamera.get();
	m_camera = std::move(followCamera);

	m_ground.Initialize(context.device);
	m_originCube.SetPosition(PlatformCubePosition.x, PlatformCubePosition.y, PlatformCubePosition.z);
	m_originCube.SetGround(&m_ground);
	m_originCube.SetSurfaceCollisionEnabled(true);
	m_originCube.SetGroundCollisionEnabled(false);
	m_originCube.SetGravityEnabled(false);
	m_originCube.Initialize(context.device);
	m_hudOverlay.Initialize(context.device, context.width, context.height);

	m_actors.clear();
	auto player = std::make_unique<OrcPlayer>();
	player->SetGround(&m_ground);
	player->AddLandingSurface(&m_originCube);
	m_player = player.get();
	m_followTarget = player.get();
	m_actors.push_back(std::move(player));
	m_actors.push_back(std::make_unique<DavenPlayer>());
	m_actors.push_back(std::make_unique<NathanWalker>());
	//m_actors.push_back(std::make_unique<ForestGoddessPlayer>());
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Initialize(context.device);
	}
}

void GameScene::Unload()
{
	m_actors.clear();
	m_camera.reset();
	m_followCamera = nullptr;
	m_player = nullptr;
	m_followTarget = nullptr;
}

void GameScene::Update(float deltaTime, const Input& input)
{
	if (m_player != nullptr)
	{
		m_player->SetMovementForward(m_camera->GetForwardXZ());
	}

	m_originCube.Update(deltaTime);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Update(deltaTime, input);
	}
	m_hudOverlay.Update(deltaTime);

	if (m_followCamera != nullptr && m_followTarget != nullptr)
	{
		m_followCamera->SetFollowTarget(GetCameraFollowPosition(), m_followTarget->GetRotationY());
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
	m_hudOverlay.Render(renderer);
}

XMMATRIX GameScene::GetViewProjectionMatrix() const
{
	return m_camera->GetViewProjectionMatrix();
}

XMFLOAT3 GameScene::GetCameraFollowPosition()
{
	const bool shouldUpdateHeight = m_player == nullptr || m_player->IsGrounded();
	return m_cameraFollowHeightLock.ResolveFollowPosition(m_followTarget->GetPosition(), shouldUpdateHeight);
}
