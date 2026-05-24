#include "Rendering/SpriteMaterial.h"

#include "Common/Common.h"

void SpriteMaterial::InitializeSolidColor(ID3D12Device* device, UINT8 red, UINT8 green, UINT8 blue, UINT8 alpha)
{
	m_texture.InitializeSolidColor(device, red, green, blue, alpha, false);
	CreateDescriptorHeap(device);
	CreateShaderResourceView(device);
}

void SpriteMaterial::InitializeTexture(ID3D12Device* device, const std::string& texturePath, bool useSrgb)
{
	m_texture.Initialize(device, texturePath, useSrgb);
	CreateDescriptorHeap(device);
	CreateShaderResourceView(device);
}

void SpriteMaterial::Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
}

void SpriteMaterial::CreateDescriptorHeap(ID3D12Device* device)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap)));
}

void SpriteMaterial::CreateShaderResourceView(ID3D12Device* device)
{
	m_texture.CreateShaderResourceView(device, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
}
