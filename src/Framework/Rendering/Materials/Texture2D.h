#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <string>
#include <cstdint>

class Texture2D
{
public:
	void Initialize(ID3D12Device* device, const std::string& filePath, bool useSrgb);
	void InitializeSolidColor(ID3D12Device* device, UINT8 red, UINT8 green, UINT8 blue, UINT8 alpha, bool useSrgb);
	void InitializeFromPixels(ID3D12Device* device, const void* rgbaPixels, UINT width, UINT height, bool useSrgb);
	void CreateShaderResourceView(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
	static std::uint64_t GetUploadCount();

private:
	void CreateTextureResource(ID3D12Device* device, const void* pixels, UINT width, UINT height, bool useSrgb);
	void UploadPixels(ID3D12Device* device, const void* pixels, const D3D12_RESOURCE_DESC& textureDesc);

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	DXGI_FORMAT m_format{ DXGI_FORMAT_R8G8B8A8_UNORM };
};
