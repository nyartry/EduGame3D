#pragma once

#include "Game/Gameplay/Cube.h"
#include "Game/Gameplay/Ground.h"
#include "Game/Effects/Particles/SimpleJumpParticleSystem.h"
#include "Framework/Scene/Core/IScene.h"
#include "Framework/Scene/Input/Input.h"
#include "Framework/Gameplay/Actor.h"
#include "Framework/Gameplay/CollisionBody.h"
#include "Framework/Scene/Cameras/CameraFollowHeightLock.h"
#include "Game/UI/HudOverlay.h"
#include "Framework/Scene/Cameras/ICamera.h"
#include "Framework/Rendering/Sprites/Sprite.h"

#include <DirectXMath.h>
#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <vector>

class IAudioService;
class IEffectPlayer;
class IRenderDevice;
class Player;
class SkinnedMeshActor;

class GameScene : public IScene
{
public:
	GameScene(
		IRenderDevice& renderDevice,
		IAudioService& audio,
		IEffectPlayer& effects,
		std::uint32_t width,
		std::uint32_t height);

	void Activate() override;
	void Unload() override;
	void Update(float deltaTime, const Input& input) override;
	void RenderWorld(IRenderer& renderer) const override;
	void RenderOverlay(IRenderer& renderer) const override;

	RenderView GetRenderView() const override;

private:
	template <typename TActor>
		requires std::derived_from<TActor, Actor>
	TActor& AddActor(std::unique_ptr<TActor> actor)
	{
		TActor& actorReference = *actor;
		if constexpr (std::derived_from<TActor, CollisionBody>)
		{
			m_collisionBodies.push_back(&actorReference);
		}
		m_actors.push_back(std::move(actor));
		return actorReference;
	}

	DirectX::XMFLOAT3 GetCameraFollowPosition();
	void ResolvePlayerBodyCollisions();

	std::unique_ptr<ICamera> m_camera;
	IRenderDevice& m_renderDevice;
	IAudioService& m_audio;
	IEffectPlayer& m_effects;
	std::uint32_t m_width{};
	std::uint32_t m_height{};
	ICamera* m_followCamera{};
	Player* m_player{};
	const SkinnedMeshActor* m_followTarget{};
	CameraFollowHeightLock m_cameraFollowHeightLock;
	Ground m_ground;
	Cube m_originCube;
	SimpleJumpParticleSystem m_jumpParticles;
	Sprite m_generatedImageSprite;
	std::array<Sprite, 3> m_primitiveSprites;
	HudOverlay m_hudOverlay;
	std::vector<std::unique_ptr<Actor>> m_actors;
	std::vector<CollisionBody*> m_collisionBodies;
};
