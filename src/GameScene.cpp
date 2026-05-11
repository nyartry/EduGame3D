#include "GameScene.h"

#include "DavenPlayer.h"
#include "Dx12Renderer.h"
#include "ForestGoddessPlayer.h"
#include "OrcPlayer.h"

#include <memory>

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
	m_camera.SetPosition(0.0f, 9.0f, -1.0f);
	m_camera.SetTarget(0.0f, 0.0f, 1.5f);

	m_ground.Initialize(device);
	m_originCube.SetPosition(0.0f, 0.0f, 0.0f);
	m_originCube.Initialize(device);

	m_actors.clear();
	m_actors.push_back(std::make_unique<OrcPlayer>());
	m_actors.push_back(std::make_unique<DavenPlayer>());
	m_actors.push_back(std::make_unique<ForestGoddessPlayer>());
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Initialize(device);
	}
}

void GameScene::Update(float deltaTime, const Input& input)
{
	m_camera.Update(deltaTime, input);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Update(deltaTime, input);
	}
}

void GameScene::Render(Dx12Renderer& renderer) const
{
	m_ground.Draw(renderer);
	m_originCube.Draw(renderer);
	for (const std::unique_ptr<Actor>& actor : m_actors)
	{
		actor->Draw(renderer);
	}
}

XMMATRIX GameScene::GetViewProjectionMatrix() const
{
	return m_camera.GetViewProjectionMatrix();
}
