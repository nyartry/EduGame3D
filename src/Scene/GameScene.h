#pragma once

#include "Scene/Camera.h"
#include "Gameplay/Cube.h"
#include "Gameplay/Ground.h"
#include "Scene/Input.h"
#include "Gameplay/Actor.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <memory>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class GameScene
{
public:
	void Initialize(ID3D12Device* device, UINT width, UINT height);
	void Update(float deltaTime, const Input& input);
	void Render(Dx12Renderer& renderer) const;

	DirectX::XMMATRIX GetViewProjectionMatrix() const;

private:
	Camera m_camera;
	Ground m_ground;
	Cube m_originCube;
	std::vector<std::unique_ptr<Actor>> m_actors;
};
