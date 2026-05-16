#pragma once

#include "Gameplay/Cube.h"
#include "Gameplay/Ground.h"
#include "Scene/Input.h"
#include "Gameplay/Actor.h"
#include "Scene/FollowCamera.h"
#include "Scene/ICamera.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <memory>
#include <vector>

class Dx12Renderer;
class Player;
class SkinnedMeshActor;
struct ID3D12Device;

class GameScene
{
public:
	void Initialize(ID3D12Device* device, UINT width, UINT height);
	void Update(float deltaTime, const Input& input);
	void Render(Dx12Renderer& renderer) const;

	DirectX::XMMATRIX GetViewProjectionMatrix() const;

private:
	DirectX::XMFLOAT3 GetCameraFollowPosition();

	std::unique_ptr<ICamera> m_camera;
	FollowCamera* m_followCamera{};
	Player* m_player{};
	const SkinnedMeshActor* m_followTarget{};
	float m_cameraFollowTargetY{};
	bool m_hasCameraFollowTargetY{};
	Ground m_ground;
	Cube m_originCube;
	std::vector<std::unique_ptr<Actor>> m_actors;
};
