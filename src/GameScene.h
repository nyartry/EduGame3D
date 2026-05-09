#pragma once

#include "Camera.h"
#include "Ground.h"

#include <Windows.h>

#include <DirectXMath.h>

class Dx12Renderer;
struct ID3D12Device;

class GameScene
{
public:
	void Initialize(ID3D12Device* device, UINT width, UINT height);
	void Update();
	void Render(Dx12Renderer& renderer) const;

	DirectX::XMMATRIX GetViewProjectionMatrix() const;

private:
	Camera m_camera;
	Ground m_ground;
};
