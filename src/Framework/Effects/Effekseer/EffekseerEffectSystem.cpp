#include "Framework/Effects/Effekseer/EffekseerEffectSystem.h"

#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Rendering/Core/Dx12Renderer.h"

#include <Windows.h>

#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

using namespace DirectX;

namespace
{
	constexpr int32_t MaxSpriteCount = 8000;
	constexpr float FixedUpdateFrameRate = 60.0f;

	Effekseer::Matrix44 ToEffekseerMatrix(const XMMATRIX& matrix)
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

	Effekseer::Vector3D ExtractViewerPosition(const XMMATRIX& view)
	{
		XMVECTOR determinant{};
		const XMMATRIX inverseView = XMMatrixInverse(&determinant, view);
		XMFLOAT4X4 stored{};
		XMStoreFloat4x4(&stored, inverseView);
		return { stored._41, stored._42, stored._43 };
	}

	std::u16string ToUtf16Path(const std::filesystem::path& path)
	{
		const std::wstring widePath = path.wstring();
		std::u16string utf16Path;
		utf16Path.reserve(widePath.size());
		for (wchar_t character : widePath)
		{
			utf16Path.push_back(static_cast<char16_t>(character));
		}
		return utf16Path;
	}
}

struct EffekseerEffectSystem::Impl
{
	Effekseer::ManagerRef manager;
	EffekseerRenderer::RendererRef renderer;
	Effekseer::Backend::GraphicsDeviceRef graphicsDevice;
	Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> memoryPool;
	Effekseer::RefPtr<EffekseerRenderer::CommandList> commandList;
	std::unordered_map<std::string, Effekseer::EffectRef> effects;
	float elapsedTime{};
	bool initialized{};
};

EffekseerEffectSystem::EffekseerEffectSystem()
	: m_impl(std::make_unique<Impl>())
{
}

EffekseerEffectSystem::~EffekseerEffectSystem() = default;
EffekseerEffectSystem::EffekseerEffectSystem(EffekseerEffectSystem&&) noexcept = default;
EffekseerEffectSystem& EffekseerEffectSystem::operator=(EffekseerEffectSystem&&) noexcept = default;

void EffekseerEffectSystem::Initialize(Dx12Renderer& renderer)
{
	ID3D12Device* device = renderer.GetDevice();
	ID3D12CommandQueue* commandQueue = renderer.GetCommandQueue();
	if (device == nullptr || commandQueue == nullptr)
	{
		throw std::runtime_error("Effekseer requires a valid D3D12 device and command queue.");
	}

	m_impl->manager = Effekseer::Manager::Create(MaxSpriteCount);
	m_impl->manager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
	m_impl->graphicsDevice = EffekseerRendererDX12::CreateGraphicsDevice(device, commandQueue, Dx12Renderer::FrameCount);

	DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_impl->renderer = EffekseerRendererDX12::Create(
		m_impl->graphicsDevice,
		&renderTargetFormat,
		1,
		DXGI_FORMAT_D32_FLOAT,
		false,
		MaxSpriteCount);
	if (m_impl->renderer == nullptr)
	{
		throw std::runtime_error("Failed to create Effekseer DX12 renderer.");
	}

	m_impl->memoryPool = EffekseerRenderer::CreateSingleFrameMemoryPool(m_impl->renderer->GetGraphicsDevice());
	m_impl->commandList = EffekseerRenderer::CreateCommandList(m_impl->renderer->GetGraphicsDevice(), m_impl->memoryPool);
	m_impl->manager->SetSpriteRenderer(m_impl->renderer->CreateSpriteRenderer());
	m_impl->manager->SetRibbonRenderer(m_impl->renderer->CreateRibbonRenderer());
	m_impl->manager->SetRingRenderer(m_impl->renderer->CreateRingRenderer());
	m_impl->manager->SetTrackRenderer(m_impl->renderer->CreateTrackRenderer());
	m_impl->manager->SetModelRenderer(m_impl->renderer->CreateModelRenderer());
	m_impl->manager->SetTextureLoader(m_impl->renderer->CreateTextureLoader());
	m_impl->manager->SetModelLoader(m_impl->renderer->CreateModelLoader());
	m_impl->manager->SetMaterialLoader(m_impl->renderer->CreateMaterialLoader());
	m_impl->manager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());
	m_impl->initialized = true;
}

void EffekseerEffectSystem::RegisterEffect(std::string_view id, std::string_view assetPath)
{
	if (!m_impl->initialized)
	{
		return;
	}
	const std::u16string utf16Path = ToUtf16Path(AssetPathResolver::Resolve(assetPath));
	m_impl->effects[std::string(id)] = Effekseer::Effect::Create(m_impl->manager, utf16Path.c_str());
}

void EffekseerEffectSystem::Play(std::string_view id, const XMFLOAT3& position, float scale)
{
	const auto effect = m_impl->effects.find(std::string(id));
	if (!m_impl->initialized || effect == m_impl->effects.end() || effect->second == nullptr)
	{
		return;
	}
	const Effekseer::Handle handle = m_impl->manager->Play(effect->second, position.x, position.y, position.z);
	m_impl->manager->SetScale(handle, scale, scale, scale);
}

void EffekseerEffectSystem::Update(float deltaTime)
{
	if (!m_impl->initialized)
	{
		return;
	}
	m_impl->elapsedTime += deltaTime;
	Effekseer::Manager::UpdateParameter updateParameter;
	updateParameter.DeltaFrame = std::max(deltaTime * FixedUpdateFrameRate, 0.0f);
	m_impl->manager->Update(updateParameter);
}

void EffekseerEffectSystem::Render(Dx12Renderer& renderer, const XMMATRIX& view, const XMMATRIX& projection)
{
	if (!m_impl->initialized || m_impl->renderer == nullptr || m_impl->commandList == nullptr)
	{
		return;
	}

	m_impl->memoryPool->NewFrame();
	EffekseerRendererDX12::BeginCommandList(m_impl->commandList, renderer.GetCommandList());
	m_impl->renderer->SetCommandList(m_impl->commandList);
	m_impl->renderer->SetTime(m_impl->elapsedTime);
	m_impl->renderer->SetCameraMatrix(ToEffekseerMatrix(view));
	m_impl->renderer->SetProjectionMatrix(ToEffekseerMatrix(projection));

	Effekseer::Manager::LayerParameter layerParameter;
	layerParameter.ViewerPosition = ExtractViewerPosition(view);
	m_impl->manager->SetLayerParameter(0, layerParameter);
	m_impl->renderer->BeginRendering();

	Effekseer::Manager::DrawParameter drawParameter;
	drawParameter.ZNear = 0.0f;
	drawParameter.ZFar = 1.0f;
	drawParameter.ViewProjectionMatrix = m_impl->renderer->GetCameraProjectionMatrix();
	m_impl->manager->Draw(drawParameter);

	m_impl->renderer->EndRendering();
	m_impl->renderer->SetCommandList(nullptr);
	EffekseerRendererDX12::EndCommandList(m_impl->commandList);
}

bool EffekseerEffectSystem::IsInitialized() const
{
	return m_impl->initialized;
}
