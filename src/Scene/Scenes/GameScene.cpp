#include "Scene/Scenes/GameScene.h"

#include "Audio/IAudioService.h"
#include "Gameplay/AnimatedCubeObject.h"
#include "Gameplay/CollisionBody.h"
#include "Gameplay/DavenPlayer.h"
#include "Rendering/Core/Dx12Renderer.h"
#include "Gameplay/ForestGoddessPlayer.h"
#include "Gameplay/NathanWalker.h"
#include "Gameplay/OrcPlayer.h"
#include "Gameplay/Player.h"
#include "Models/SkinnedMeshActor.h"
#include "Rendering/Sprites/SpriteShapeFactory.h"

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
	constexpr const char* EffekseerSampleEffectPath = "Content\\Effects\\Effekseer\\Samples\\Laser01.efkefc";
	constexpr XMFLOAT3 PlatformCubePosition{ 0.0f, 0.5f, 2.0f };
	constexpr XMFLOAT4 RectColor{ 0.35f, 0.86f, 1.0f, 0.88f };
	constexpr XMFLOAT4 SquareColor{ 0.92f, 0.36f, 0.78f, 0.88f };
	constexpr XMFLOAT4 TriangleColor{ 1.0f, 0.76f, 0.24f, 0.90f };
}

void GameScene::Load(const SceneLoadContext& context)
{
	m_audio = context.audio;
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
	m_primitiveSprites[0] = SpriteShapeFactory::CreateRect(
		context.device,
		310.0f,
		28.0f,
		72.0f,
		72.0f,
		SquareColor);
	m_primitiveSprites[1] = SpriteShapeFactory::CreateRect(
		context.device,
		394.0f,
		42.0f,
		116.0f,
		44.0f,
		RectColor);
	m_primitiveSprites[2] = SpriteShapeFactory::CreateTriangle(
		context.device,
		526.0f,
		28.0f,
		72.0f,
		72.0f,
		TriangleColor);
	m_generatedImageSprite.InitializeTexture(
		context.device,
		GeneratedImageTexturePath,
		42.0f,
		static_cast<float>(context.height) - 202.0f,
		160.0f,
		160.0f);
	m_effekseerEffects.Initialize(context.device, context.commandQueue);
	m_effekseerEffects.LoadSampleEffect(EffekseerSampleEffectPath);
	m_jumpParticles.Initialize(context.device);
	m_hudOverlay.Initialize(context.device, context.width, context.height);

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
	if (!m_bgmStarted && m_audio != nullptr)
	{
		m_audio->PlayBgm(BgmId::Game);
		m_bgmStarted = true;
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
	}
	m_jumpParticles.Update(deltaTime);
	m_effekseerEffects.Update(deltaTime);
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
	if (m_followCamera != nullptr)
	{
		m_effekseerEffects.Render(renderer, m_followCamera->GetViewMatrix(), m_followCamera->GetProjectionMatrix());
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
