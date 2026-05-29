#pragma once

#include "Gameplay/Cube.h"
#include "Gameplay/Ground.h"
#include "Scene/IScene.h"
#include "Scene/Input.h"
#include "Gameplay/Actor.h"
#include "Scene/CameraFollowHeightLock.h"
#include "Scene/FollowCamera.h"
#include "Scene/HudOverlay.h"
#include "Scene/ICamera.h"
#include "Rendering/Sprites/Sprite.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <array>
#include <memory>
#include <vector>

class Dx12Renderer;
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

	std::unique_ptr<ICamera> m_camera;
	FollowCamera* m_followCamera{};
	Player* m_player{};
	const SkinnedMeshActor* m_followTarget{};
	CameraFollowHeightLock m_cameraFollowHeightLock;
	Ground m_ground;
	Cube m_originCube;
	std::array<Sprite, 3> m_primitiveSprites;
	HudOverlay m_hudOverlay;
	std::vector<std::unique_ptr<Actor>> m_actors;
};
