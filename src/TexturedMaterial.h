#pragma once

#include "Texture2D.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <string>

class TexturedMaterial
{
public:
	void Initialize(ID3D12Device* device, const std::string& baseColorTexturePath, const std::string& opacityTexturePath);
	void Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const;

private:
	Texture2D m_baseColorTexture;
	Texture2D m_opacityTexture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
};
