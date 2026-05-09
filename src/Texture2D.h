#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <string>

class Texture2D
{
public:
	void Initialize(ID3D12Device* device, const std::string& filePath);
	void InitializeSolidColor(ID3D12Device* device, UINT8 red, UINT8 green, UINT8 blue, UINT8 alpha);
	void CreateShaderResourceView(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

private:
	void CreateFallbackTexture(ID3D12Device* device);
	void CreateTextureResource(ID3D12Device* device, const void* pixels, UINT width, UINT height);

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};
