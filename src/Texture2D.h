#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <string>

class Texture2D
{
public:
	void Initialize(ID3D12Device* device, const std::string& filePath);
	void Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const;

private:
	void CreateFallbackTexture(ID3D12Device* device);
	void CreateTextureResource(ID3D12Device* device, const void* pixels, UINT width, UINT height);
	void CreateShaderResourceView(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
};
