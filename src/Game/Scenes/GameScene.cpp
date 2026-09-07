#include "Game/Scenes/GameScene.h"

#include "Framework/Audio/IAudioService.h"
#include "Framework/Effects/IEffectService.h"
#include "Game/Gameplay/AnimatedCubeObject.h"
#include "Framework/Gameplay/CollisionBody.h"
#include "Game/Gameplay/DavenPlayer.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Game/Gameplay/ForestGoddessPlayer.h"
#include "Game/Gameplay/NathanWalker.h"
#include "Game/Gameplay/OrcPlayer.h"
#include "Game/Gameplay/Player.h"
#include "Game/Input/GameActions.h"
#include "Framework/Models/SkinnedMeshActor.h"
#include "Framework/Models/ModelAssetCache.h"
#include "Framework/Assets/ImageLoader.h"
#include "Framework/Rendering/Sprites/SpriteShapeFactory.h"
#include "Game/Content/GameContent.h"
#include <memory>

using namespace DirectX;

namespace
{
	constexpr float CameraFovYDegrees = 55.0f;
	//constexpr float CameraNearZ = 0.1f;
	//constexpr float CameraFarZ = 100.0f;
	constexpr float CameraNearZ = 0.5f;
	constexpr float CameraFarZ = 50.0f;
	constexpr const char* GeneratedImageTexturePath = "Content\\Textures\\UI\\open_campus_crest.png";
	constexpr XMFLOAT3 PlatformCubePosition{ 0.0f, 0.5f, 2.0f };
	constexpr XMFLOAT4 RectColor{ 0.35f, 0.86f, 1.0f, 0.88f };
	constexpr XMFLOAT4 SquareColor{ 0.92f, 0.36f, 0.78f, 0.88f };
	constexpr XMFLOAT4 TriangleColor{ 1.0f, 0.76f, 0.24f, 0.90f };
}

GameScene::GameScene(
	IRenderDevice& renderDevice,
	IAudioService& audio,
	IEffectPlayer& effects,
	std::uint32_t width,
	std::uint32_t height)
	: m_renderDevice(renderDevice)
	, m_audio(audio)
	, m_effects(effects)
	, m_width(width)
	, m_height(height)
{
}

void GameScene::Prepare()
{
	m_prepared = false;
	m_collisionWorld.Clear();
	m_actors.clear();
	m_player = nullptr;
	m_followTarget = nullptr;
	m_preparedImages.clear();
	m_collisionWorld.RegisterSurface(m_ground);
	m_collisionWorld.RegisterSurface(m_originCube, &m_originCube);
	auto player = std::make_unique<OrcPlayer>();
	player->SetCollisionQuery(&m_collisionWorld);
	m_player = &AddActor(std::move(player));
	m_followTarget = m_player;
	AddActor(std::make_unique<AnimatedCubeObject>());
	AddActor(std::make_unique<DavenPlayer>());
	AddActor(std::make_unique<NathanWalker>());
	//AddActor(std::make_unique<ForestGoddessPlayer>());
	ModelAssetCache assets;
	for (const auto& actor : m_actors) actor->Prepare(assets);
	m_preparedImages = assets.TakePreparedImages();
	m_preparedImages.push_back(ImageLoader::Load(GeneratedImageTexturePath));
	m_prepared = true;
}

void GameScene::Activate()
{
	if (!m_prepared) Prepare();
	const float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);

	m_camera = std::make_unique<CameraController>();
	m_camera->SetLens(XMConvertToRadians(CameraFovYDegrees), aspectRatio, CameraNearZ, CameraFarZ);

	m_ground.Initialize(m_renderDevice);
	m_originCube.SetPosition(PlatformCubePosition.x, PlatformCubePosition.y, PlatformCubePosition.z);
	m_originCube.SetCollisionQuery(&m_collisionWorld);
	m_originCube.SetSurfaceCollisionEnabled(true);
	m_originCube.SetGroundCollisionEnabled(false);
	m_originCube.SetGravityEnabled(false);
	m_originCube.Initialize(m_renderDevice);
	m_primitiveSprites[0] = SpriteShapeFactory::CreateRect(
		m_renderDevice,
		310.0f,
		28.0f,
		72.0f,
		72.0f,
		SquareColor);
	m_primitiveSprites[1] = SpriteShapeFactory::CreateRect(
		m_renderDevice,
		394.0f,
		42.0f,
		116.0f,
		44.0f,
		RectColor);
	m_primitiveSprites[2] = SpriteShapeFactory::CreateTriangle(
		m_renderDevice,
		526.0f,
		28.0f,
		72.0f,
		72.0f,
		TriangleColor);
	m_generatedImageSprite.InitializeTexture(
		m_renderDevice,
		GeneratedImageTexturePath,
		42.0f,
		static_cast<float>(m_height) - 202.0f,
		160.0f,
		160.0f);
	m_jumpParticles.Initialize(m_renderDevice);
	m_hudOverlay.Initialize(m_renderDevice, m_width, m_height);

	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Initialize(m_renderDevice);
	}
	m_preparedImages.clear();
}

void GameScene::Unload()
{
	m_prepared = false;
	m_preparedImages.clear();
	m_collisionWorld.Clear();
	m_actors.clear();
	m_camera.reset();
	m_player = nullptr;
	m_followTarget = nullptr;
}

void GameScene::Update(float deltaTime, const Input& input)
{
	m_audio.PlayBgm(GameContent::GameBgm);
	UpdateCameraMode(input);

	if (m_player != nullptr && m_camera != nullptr)
	{
		m_player->SetMovementForward(m_camera->GetForwardXZ());
	}

	m_originCube.Update(deltaTime);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Update(deltaTime, input);
		if (auto* skinned = dynamic_cast<SkinnedMeshActor*>(actor.get()))
		{
			// Consume each simulation step before a catch-up step replaces events.
			// Cue names refer to the game's registered audio/effect catalog.
			for (const auto& occurrence : skinned->ConsumeAnimationEvents())
			{
				const auto& event = occurrence.event;
				if (event.cue.empty()) continue;
				if (event.type == "Footstep" || event.type == "PlaySE") m_audio.PlaySe(event.cue);
				else if (event.type == "PlayEffect") m_effects.Play(event.cue, skinned->GetAnimationEventPosition(event.bone));
			}
		}
	}
	ResolvePlayerBodyCollisions();
	if (m_player != nullptr && m_player->DidStartJumpThisFrame())
	{
		XMFLOAT3 effectPosition = m_player->GetLastJumpStartPosition();
		effectPosition.y += 0.02f;
		m_jumpParticles.Emit(effectPosition);
		m_effects.Play(GameContent::JumpEffect, effectPosition, 0.35f);
	}
	m_jumpParticles.Update(deltaTime);

	if (m_camera != nullptr && m_followTarget != nullptr)
	{
		m_camera->SetTarget(GetActiveCameraTargetPosition(), m_followTarget->GetRotationY());
	}
	if (m_camera != nullptr)
	{
		m_camera->Update(deltaTime, input);
	}
}

void GameScene::UpdateFrame(float deltaTime, const Input&)
{
	m_hudOverlay.Update(deltaTime);
}

void GameScene::RenderWorld(IRenderer& renderer) const
{
	m_ground.Draw(renderer);
	m_originCube.Draw(renderer);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Draw(renderer);
	}
	m_jumpParticles.Render(renderer);
}

void GameScene::RenderOverlay(IRenderer& renderer) const
{
	for (const Sprite& primitiveSprite : m_primitiveSprites)
	{
		primitiveSprite.Render(renderer);
	}
	m_generatedImageSprite.Render(renderer);
	m_hudOverlay.Render(renderer);
}

RenderView GameScene::GetRenderView() const
{
	if (m_camera == nullptr)
	{
		return {};
	}
	return { m_camera->GetViewMatrix(), m_camera->GetProjectionMatrix(), true };
}

XMFLOAT3 GameScene::GetCameraFollowPosition()
{
	const bool shouldUpdateHeight = m_player == nullptr || m_player->IsGrounded();
	return m_cameraFollowHeightLock.ResolveFollowPosition(m_followTarget->GetPosition(), shouldUpdateHeight);
}

XMFLOAT3 GameScene::GetActiveCameraTargetPosition()
{
	if (m_camera->GetMode() == CameraMode::Follow)
	{
		return GetCameraFollowPosition();
	}
	return m_followTarget->GetPosition();
}

void GameScene::UpdateCameraMode(const Input& input)
{
	if (m_camera == nullptr)
	{
		return;
	}

	if (GameActions::WasPressed(input, GameAction::SelectFollowCamera))
	{
		m_camera->SetMode(CameraMode::Follow);
	}
	else if (GameActions::WasPressed(input, GameAction::SelectSpringFollowCamera))
	{
		m_camera->SetMode(CameraMode::SpringFollow);
	}
	else if (GameActions::WasPressed(input, GameAction::SelectFirstPersonCamera))
	{
		m_camera->SetMode(CameraMode::FirstPerson);
	}
	else if (GameActions::WasPressed(input, GameAction::SelectOrbitCamera))
	{
		m_camera->SetMode(CameraMode::Orbit);
	}
	else if (GameActions::WasPressed(input, GameAction::SelectSplineCamera))
	{
		m_camera->SetMode(CameraMode::Spline);
	}
}

void GameScene::ResolvePlayerBodyCollisions()
{
	if (m_player == nullptr)
	{
		return;
	}

	m_collisionWorld.ResolveBodyCollisions(*m_player);
}
