#include "Effects/Effekseer/EffekseerEffectSystem.h"

#include "Rendering/Core/Dx12Renderer.h"

#include <algorithm>
#include <stdexcept>

using namespace DirectX;

namespace
{
	constexpr int32_t MaxSpriteCount = 8000;
	constexpr float FixedUpdateFrameRate = 60.0f;
}

void EffekseerEffectSystem::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
	if (device == nullptr || commandQueue == nullptr)
	{
		throw std::runtime_error("Effekseer requires a valid D3D12 device and command queue.");
	}

	m_manager = Effekseer::Manager::Create(MaxSpriteCount);
	m_manager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	m_graphicsDevice = EffekseerRendererDX12::CreateGraphicsDevice(device, commandQueue, Dx12Renderer::FrameCount);

	DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_renderer = EffekseerRendererDX12::Create(
		m_graphicsDevice,
		&renderTargetFormat,
		1,
		DXGI_FORMAT_D32_FLOAT,
		false,
		MaxSpriteCount);
	if (m_renderer == nullptr)
	{
		throw std::runtime_error("Failed to create Effekseer DX12 renderer.");
	}

	m_memoryPool = EffekseerRenderer::CreateSingleFrameMemoryPool(m_renderer->GetGraphicsDevice());
	m_commandList = EffekseerRenderer::CreateCommandList(m_renderer->GetGraphicsDevice(), m_memoryPool);

	m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
	m_manager->SetRibbonRenderer(m_renderer->CreateRibbonRenderer());
	m_manager->SetRingRenderer(m_renderer->CreateRingRenderer());
	m_manager->SetTrackRenderer(m_renderer->CreateTrackRenderer());
	m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());
	m_manager->SetTextureLoader(m_renderer->CreateTextureLoader());
	m_manager->SetModelLoader(m_renderer->CreateModelLoader());
	m_manager->SetMaterialLoader(m_renderer->CreateMaterialLoader());
	m_manager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

	m_initialized = true;
}

void EffekseerEffectSystem::LoadSampleEffect(const std::string& effectPath)
{
	if (!m_initialized)
	{
		return;
	}

	const std::u16string utf16Path = ToUtf16Path(effectPath);
	m_sampleEffect = Effekseer::Effect::Create(m_manager, utf16Path.c_str());
}

void EffekseerEffectSystem::PlaySampleEffect(const XMFLOAT3& position, float scale)
{
	if (!m_initialized || m_sampleEffect == nullptr)
	{
		return;
	}

	m_sampleHandle = m_manager->Play(m_sampleEffect, position.x, position.y, position.z);
	m_manager->SetScale(m_sampleHandle, scale, scale, scale);
}

void EffekseerEffectSystem::Update(float deltaTime)
{
	if (!m_initialized)
	{
		return;
	}

	m_elapsedTime += deltaTime;

	Effekseer::Manager::UpdateParameter updateParameter;
	updateParameter.DeltaFrame = std::max(deltaTime * FixedUpdateFrameRate, 0.0f);
	m_manager->Update(updateParameter);
}

void EffekseerEffectSystem::Render(Dx12Renderer& renderer, const XMMATRIX& view, const XMMATRIX& projection)
{
	if (!m_initialized || m_renderer == nullptr || m_commandList == nullptr)
	{
		return;
	}

	m_memoryPool->NewFrame();
	EffekseerRendererDX12::BeginCommandList(m_commandList, renderer.GetCommandList());
	m_renderer->SetCommandList(m_commandList);
	m_renderer->SetTime(m_elapsedTime);
	m_renderer->SetCameraMatrix(ToEffekseerMatrix(view));
	m_renderer->SetProjectionMatrix(ToEffekseerMatrix(projection));

	Effekseer::Manager::LayerParameter layerParameter;
	layerParameter.ViewerPosition = ExtractViewerPosition(view);
	m_manager->SetLayerParameter(0, layerParameter);

	m_renderer->BeginRendering();

	Effekseer::Manager::DrawParameter drawParameter;
	drawParameter.ZNear = 0.0f;
	drawParameter.ZFar = 1.0f;
	drawParameter.ViewProjectionMatrix = m_renderer->GetCameraProjectionMatrix();
	m_manager->Draw(drawParameter);

	m_renderer->EndRendering();
	m_renderer->SetCommandList(nullptr);
	EffekseerRendererDX12::EndCommandList(m_commandList);
}

bool EffekseerEffectSystem::IsInitialized() const
{
	return m_initialized;
}

Effekseer::Matrix44 EffekseerEffectSystem::ToEffekseerMatrix(const XMMATRIX& matrix)
{
	XMFLOAT4X4 stored{};
	XMStoreFloat4x4(&stored, matrix);

	Effekseer::Matrix44 result;
	for (int row = 0; row < 4; ++row)
	{
		for (int column = 0; column < 4; ++column)
		{
			result.Values[row][column] = stored.m[row][column];
		}
	}
	return result;
}

Effekseer::Vector3D EffekseerEffectSystem::ExtractViewerPosition(const XMMATRIX& view)
{
	XMVECTOR determinant{};
	const XMMATRIX inverseView = XMMatrixInverse(&determinant, view);
	XMFLOAT4X4 stored{};
	XMStoreFloat4x4(&stored, inverseView);
	return { stored._41, stored._42, stored._43 };
}

std::u16string EffekseerEffectSystem::ToUtf16Path(const std::string& path)
{
	if (path.empty())
	{
		return {};
	}

	const int wideLength = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
	if (wideLength == 0)
	{
		throw std::runtime_error("Failed to convert Effekseer path to UTF-16.");
	}

	std::wstring widePath(static_cast<size_t>(wideLength), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, widePath.data(), wideLength);

	std::u16string utf16Path;
	utf16Path.reserve(widePath.size());
	for (wchar_t character : widePath)
	{
		utf16Path.push_back(static_cast<char16_t>(character));
	}
	return utf16Path;
}
