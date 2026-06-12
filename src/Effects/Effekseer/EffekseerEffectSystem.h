#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <DirectXMath.h>
#include <d3d12.h>

#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

#include <string>

class Dx12Renderer;

class EffekseerEffectSystem
{
public:
	void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
	void LoadSampleEffect(const std::string& effectPath);
	void Update(float deltaTime);
	void Render(Dx12Renderer& renderer, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);

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
	Effekseer::EffectRef m_sampleEffect;
	Effekseer::Handle m_sampleHandle{};
	float m_elapsedTime{};
	float m_sampleReplayTimer{};
	bool m_initialized{};
};
