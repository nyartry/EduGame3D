#include "TexturedMaterial.h"

#include "Common.h"

void TexturedMaterial::Initialize(
	ID3D12Device* device,
	const std::string& baseColorTexturePath,
	const std::string& opacityTexturePath,
	const std::string& normalTexturePath)
{
	m_baseColorTexture.Initialize(device, baseColorTexturePath, true);
	if (opacityTexturePath.empty())
	{
		m_opacityTexture.InitializeSolidColor(device, 255, 255, 255, 255, false);
	}
	else
	{
		m_opacityTexture.Initialize(device, opacityTexturePath, false);
	}

	if (normalTexturePath.empty())
	{
		m_normalTexture.InitializeSolidColor(device, 128, 128, 255, 255, false);
	}
	else
	{
		m_normalTexture.Initialize(device, normalTexturePath, false);
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = 3;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap)));

	const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	m_baseColorTexture.CreateShaderResourceView(device, handle);
	handle.ptr += descriptorSize;
	m_opacityTexture.CreateShaderResourceView(device, handle);
	handle.ptr += descriptorSize;
	m_normalTexture.CreateShaderResourceView(device, handle);
}

void TexturedMaterial::Bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
}
