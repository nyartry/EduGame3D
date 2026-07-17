#pragma once

#include "Game/Gameplay/Cube.h"
#include "Game/Gameplay/Ground.h"
#include "Framework/Effects/Effekseer/EffekseerEffectSystem.h"
#include "Game/Effects/Particles/SimpleJumpParticleSystem.h"
#include "Framework/Scene/Core/IScene.h"
#include "Framework/Scene/Input/Input.h"
#include "Framework/Gameplay/Actor.h"
#include "Framework/Scene/Cameras/CameraFollowHeightLock.h"
#include "Game/UI/HudOverlay.h"
#include "Framework/Scene/Cameras/ICamera.h"
#include "Framework/Rendering/Sprites/Sprite.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <array>
#include <memory>
#include <vector>

class Dx12Renderer;
class IAudioService;
class Player;
class SkinnedMeshActor;
struct ID3D12Device;

class GameScene : public IScene
{
public:
	void Load(const SceneLoadContext& context) override;
	void Unload() override;
	void Update(float deltaTime, const Input& input) override;
	void Render(Dx12Renderer& renderer) const override;

	DirectX::XMMATRIX GetViewProjectionMatrix() const override;

private:
	DirectX::XMFLOAT3 GetCameraFollowPosition();
	void ResolvePlayerBodyCollisions();

	std::unique_ptr<ICamera> m_camera;
	IAudioService* m_audio{};
	ICamera* m_followCamera{};
	Player* m_player{};
	const SkinnedMeshActor* m_followTarget{};
	CameraFollowHeightLock m_cameraFollowHeightLock;
	Ground m_ground;
	Cube m_originCube;
	mutable EffekseerEffectSystem m_effekseerEffects;
	SimpleJumpParticleSystem m_jumpParticles;
	Sprite m_generatedImageSprite;
	std::array<Sprite, 3> m_primitiveSprites;
	HudOverlay m_hudOverlay;
	std::vector<std::unique_ptr<Actor>> m_actors;
};
