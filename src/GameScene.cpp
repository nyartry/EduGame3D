#include "GameScene.h"

#include "Dx12Renderer.h"

using namespace DirectX;

namespace
{
	constexpr float CameraFovYDegrees = 55.0f;
	constexpr float CameraNearZ = 0.1f;
	constexpr float CameraFarZ = 100.0f;
}

void GameScene::Initialize(ID3D12Device* device, UINT width, UINT height)
{
	const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	m_camera.SetLens(XMConvertToRadians(CameraFovYDegrees), aspectRatio, CameraNearZ, CameraFarZ);
	m_camera.SetPosition(0.0f, 9.0f, -9.0f);
	m_camera.SetTarget(0.0f, 0.0f, 1.5f);

	m_ground.Initialize(device);
	m_originCube.SetPosition(0.0f, 0.0f, 0.0f);
	m_originCube.Initialize(device);
}

void GameScene::Update(float deltaTime, const Input& input)
{
	m_camera.Update(deltaTime, input);
}

void GameScene::Render(Dx12Renderer& renderer) const
{
	m_ground.Draw(renderer);
	m_originCube.Draw(renderer);
}

XMMATRIX GameScene::GetViewProjectionMatrix() const
{
	return m_camera.GetViewProjectionMatrix();
}
