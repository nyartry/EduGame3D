#include "Framework/Rendering/Materials/SpriteMaterial.h"

#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"
#include "Framework/Rendering/Materials/Texture2D.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct SpriteMaterial::Impl
{
	void CreateDescriptorHeap(ID3D12Device* device)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap)));
	}

	void CreateShaderResourceView(ID3D12Device* device)
	{
		texture.CreateShaderResourceView(device, srvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	Texture2D texture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
};

SpriteMaterial::SpriteMaterial()
	: m_impl(std::make_unique<Impl>())
{
}

SpriteMaterial::~SpriteMaterial() = default;
SpriteMaterial::SpriteMaterial(SpriteMaterial&&) noexcept = default;
SpriteMaterial& SpriteMaterial::operator=(SpriteMaterial&&) noexcept = default;

void RenderResourceAccess::InitializeSolidColor(
	SpriteMaterial& material,
	ID3D12Device* device,
	std::uint8_t red,
	std::uint8_t green,
	std::uint8_t blue,
	std::uint8_t alpha)
{
	if (material.m_impl == nullptr)
	{
		material.m_impl = std::make_unique<SpriteMaterial::Impl>();
	}
	material.m_impl->texture.InitializeSolidColor(device, red, green, blue, alpha, false);
	material.m_impl->CreateDescriptorHeap(device);
	material.m_impl->CreateShaderResourceView(device);
}

void RenderResourceAccess::InitializeTexture(
	SpriteMaterial& material,
	ID3D12Device* device,
	const std::string& texturePath,
	bool useSrgb)
{
	if (material.m_impl == nullptr)
	{
		material.m_impl = std::make_unique<SpriteMaterial::Impl>();
	}
	material.m_impl->texture.Initialize(device, texturePath, useSrgb);
	material.m_impl->CreateDescriptorHeap(device);
	material.m_impl->CreateShaderResourceView(device);
}

void RenderResourceAccess::InitializePixels(
	SpriteMaterial& material,
	ID3D12Device* device,
	const std::vector<std::uint8_t>& rgbaPixels,
	std::uint32_t width,
	std::uint32_t height,
	bool useSrgb)
{
	if (material.m_impl == nullptr)
	{
		material.m_impl = std::make_unique<SpriteMaterial::Impl>();
	}
	material.m_impl->texture.InitializeFromPixels(device, rgbaPixels.data(), width, height, useSrgb);
	material.m_impl->CreateDescriptorHeap(device);
	material.m_impl->CreateShaderResourceView(device);
}

void RenderResourceAccess::Bind(
	const SpriteMaterial& material,
	ID3D12GraphicsCommandList* commandList,
	std::uint32_t rootParameterIndex)
{
	if (material.m_impl == nullptr || material.m_impl->srvHeap == nullptr)
	{
		return;
	}
	ID3D12DescriptorHeap* heaps[] = { material.m_impl->srvHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, material.m_impl->srvHeap->GetGPUDescriptorHandleForHeapStart());
}
