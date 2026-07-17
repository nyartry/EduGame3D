#pragma once

#include "Framework/Effects/IEffectService.h"

#include <Windows.h>
#include <wrl/client.h>

#include <DirectXMath.h>
#include <d3d12.h>

#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

#include <string>
#include <string_view>
#include <unordered_map>

class Dx12Renderer;

class EffekseerEffectSystem final : public IEffectService
{
public:
	void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
	void RegisterEffect(std::string_view id, std::string_view assetPath) override;
	void Play(std::string_view id, const DirectX::XMFLOAT3& position, float scale = 1.0f) override;
	void Update(float deltaTime) override;
	void Render(IRenderer& renderer, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection) override;

	bool IsInitialized() const;

private:
	static Effekseer::Matrix44 ToEffekseerMatrix(const DirectX::XMMATRIX& matrix);
	static Effekseer::Vector3D ExtractViewerPosition(const DirectX::XMMATRIX& view);
	static std::u16string ToUtf16Path(const std::string& path);

	Effekseer::ManagerRef m_manager;
	EffekseerRenderer::RendererRef m_renderer;
	Effekseer::Backend::GraphicsDeviceRef m_graphicsDevice;
	Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> m_memoryPool;
	Effekseer::RefPtr<EffekseerRenderer::CommandList> m_commandList;
	std::unordered_map<std::string, Effekseer::EffectRef> m_effects;
	float m_elapsedTime{};
	bool m_initialized{};
};
