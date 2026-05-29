#pragma once

#include "Rendering/Materials/Texture2D.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <string>
#include <vector>

class SpriteMaterial
{
public:
	void InitializeSolidColor(ID3D12Device* device, UINT8 red, UINT8 green, UINT8 blue, UINT8 alpha);
	void InitializeTexture(ID3D12Device* device, const std::string& texturePath, bool useSrgb = false);
	void InitializePixels(ID3D12Device* device, const std::vector<UINT8>& rgbaPixels, UINT width, UINT height, bool useSrgb = false);
	void Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const;

private:
	void CreateDescriptorHeap(ID3D12Device* device);
	void CreateShaderResourceView(ID3D12Device* device);

	Texture2D m_texture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
};
