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
#include "Framework/Models/SkinnedMeshActor.h"
#include "Framework/Rendering/Sprites/SpriteShapeFactory.h"
#include "Framework/Scene/Cameras/Camera.h"
#include "Framework/Scene/Cameras/FollowCamera.h"
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

void GameScene::Load(const SceneLoadContext& context)
{
	m_audio = context.audio;
	m_effects = context.effects;
	const float aspectRatio = static_cast<float>(context.width) / static_cast<float>(context.height);

	auto followCamera = std::make_unique<FollowCamera>();
	followCamera->SetLens(XMConvertToRadians(CameraFovYDegrees), aspectRatio, CameraNearZ, CameraFarZ);
	m_followCamera = followCamera.get();
	m_camera = std::move(followCamera);

	m_ground.Initialize(*context.renderDevice);
	m_originCube.SetPosition(PlatformCubePosition.x, PlatformCubePosition.y, PlatformCubePosition.z);
	m_originCube.SetGround(&m_ground);
	m_originCube.SetSurfaceCollisionEnabled(true);
	m_originCube.SetGroundCollisionEnabled(false);
	m_originCube.SetGravityEnabled(false);
	m_originCube.Initialize(*context.renderDevice);
	m_primitiveSprites[0] = SpriteShapeFactory::CreateRect(
		*context.renderDevice,
		310.0f,
		28.0f,
		72.0f,
		72.0f,
		SquareColor);
	m_primitiveSprites[1] = SpriteShapeFactory::CreateRect(
		*context.renderDevice,
		394.0f,
		42.0f,
		116.0f,
		44.0f,
		RectColor);
	m_primitiveSprites[2] = SpriteShapeFactory::CreateTriangle(
		*context.renderDevice,
		526.0f,
		28.0f,
		72.0f,
		72.0f,
		TriangleColor);
	m_generatedImageSprite.InitializeTexture(
		*context.renderDevice,
		GeneratedImageTexturePath,
		42.0f,
		static_cast<float>(context.height) - 202.0f,
		160.0f,
		160.0f);
	m_jumpParticles.Initialize(*context.renderDevice);
	m_hudOverlay.Initialize(*context.renderDevice, context.width, context.height);

	m_actors.clear();
	auto player = std::make_unique<OrcPlayer>();
	player->SetGround(&m_ground);
	player->AddLandingSurface(&m_originCube);
	m_player = player.get();
	m_followTarget = player.get();
	m_actors.push_back(std::move(player));
	m_actors.push_back(std::make_unique<AnimatedCubeObject>());
	m_actors.push_back(std::make_unique<DavenPlayer>());
	m_actors.push_back(std::make_unique<NathanWalker>());
	//m_actors.push_back(std::make_unique<ForestGoddessPlayer>());
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Initialize(*context.renderDevice);
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
	if (m_audio != nullptr)
	{
		m_audio->PlayBgm(GameContent::GameBgm);
	}

	if (m_player != nullptr)
	{
		m_player->SetMovementForward(m_camera->GetForwardXZ());
	}

	m_originCube.Update(deltaTime);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Update(deltaTime, input);
	}
	ResolvePlayerBodyCollisions();
	if (m_player != nullptr && m_player->DidStartJumpThisFrame())
	{
		XMFLOAT3 effectPosition = m_player->GetLastJumpStartPosition();
		effectPosition.y += 0.02f;
		m_jumpParticles.Emit(effectPosition);
		if (m_effects != nullptr)
		{
			m_effects->Play(GameContent::JumpEffect, effectPosition, 0.35f);
		}
	}
	m_jumpParticles.Update(deltaTime);
	if (m_effects != nullptr)
	{
		m_effects->Update(deltaTime);
	}
	m_hudOverlay.Update(deltaTime);

	if (m_followCamera != nullptr && m_followTarget != nullptr)
	{
		m_followCamera->SetTarget(GetCameraFollowPosition(), m_followTarget->GetRotationY());
	}
	m_camera->Update(deltaTime, input);
}

void GameScene::Render(IRenderer& renderer) const
{
	m_ground.Draw(renderer);
	m_originCube.Draw(renderer);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Draw(renderer);
	}
	if (m_followCamera != nullptr)
	{
		if (m_effects != nullptr)
		{
			m_effects->Render(renderer, m_followCamera->GetViewMatrix(), m_followCamera->GetProjectionMatrix());
		}
	}
	m_jumpParticles.Render(renderer);
	for (const Sprite& primitiveSprite : m_primitiveSprites)
	{
		primitiveSprite.Render(renderer);
	}
	m_generatedImageSprite.Render(renderer);
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

void GameScene::ResolvePlayerBodyCollisions()
{
	if (m_player == nullptr)
	{
		return;
	}

	constexpr int SolverPassCount = 3;
	CollisionBody& playerBody = *m_player;
	for (int pass = 0; pass < SolverPassCount; ++pass)
	{
		bool resolvedAny = false;
		resolvedAny |= ResolveCollisionBodyAgainst(playerBody, m_originCube);

		for (const std::unique_ptr<Actor>& actor : m_actors)
		{
			if (actor.get() == m_player)
			{
				continue;
			}

			const CollisionBody* body = dynamic_cast<const CollisionBody*>(actor.get());
			if (body == nullptr)
			{
				continue;
			}

			resolvedAny |= ResolveCollisionBodyAgainst(playerBody, *body);
		}

		if (resolvedAny)
		{
			m_player->ResolveWallCollision();
		}
		else
		{
			break;
		}
	}
}
