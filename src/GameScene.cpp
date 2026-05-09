#include "GameScene.h"

#include "Dx12Renderer.h"

using namespace DirectX;

void GameScene::Initialize(ID3D12Device* device, UINT width, UINT height)
{
	const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	m_camera.SetLens(XMConvertToRadians(55.0f), aspectRatio, 0.1f, 100.0f);
	m_camera.LookAt(
		XMFLOAT3{ 0.0f, 9.0f, -9.0f },
		XMFLOAT3{ 0.0f, 0.0f, 1.5f },
		XMFLOAT3{ 0.0f, 1.0f, 0.0f });

	m_ground.Initialize(device);
}

void GameScene::Update(float deltaTime, const Input& input)
{
	m_camera.Update(deltaTime, input);
}

void GameScene::Render(Dx12Renderer& renderer) const
{
	m_ground.Draw(renderer);
}

XMMATRIX GameScene::GetViewProjectionMatrix() const
{
	return m_camera.GetViewProjectionMatrix();
}
