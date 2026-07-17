#include "Framework/Rendering/Materials/TexturedMaterial.h"

#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"
#include "Framework/Rendering/Materials/Texture2D.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <memory>
#include <string>
#include <utility>

struct TexturedMaterial::Impl
{
	static constexpr std::uint32_t TextureCount = 3;

	void LoadTextures(
		ID3D12Device* device,
		const std::string& baseColorTexturePath,
		const std::string& opacityTexturePath,
		const std::string& normalTexturePath)
	{
		if (baseColorTexturePath.empty())
		{
			baseColorTexture.InitializeSolidColor(device, 255, 255, 255, 255, true);
		}
		else
		{
			baseColorTexture.Initialize(device, baseColorTexturePath, true);
		}

		if (opacityTexturePath.empty())
		{
			opacityTexture.InitializeSolidColor(device, 255, 255, 255, 255, false);
		}
		else
		{
			opacityTexture.Initialize(device, opacityTexturePath, false);
		}

		if (normalTexturePath.empty())
		{
			normalTexture.InitializeSolidColor(device, 128, 128, 255, 255, false);
		}
		else
		{
			normalTexture.Initialize(device, normalTexturePath, false);
		}
	}

	void CreateDescriptorHeap(ID3D12Device* device)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.NumDescriptors = TextureCount;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap)));
	}

	void CreateShaderResourceViews(ID3D12Device* device)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
		const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		baseColorTexture.CreateShaderResourceView(device, handle);
		handle.ptr += descriptorSize;
		opacityTexture.CreateShaderResourceView(device, handle);
		handle.ptr += descriptorSize;
		normalTexture.CreateShaderResourceView(device, handle);
	}

	Texture2D baseColorTexture;
	Texture2D opacityTexture;
	Texture2D normalTexture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
};

TexturedMaterial::TexturedMaterial()
	: m_impl(std::make_unique<Impl>())
{
}

TexturedMaterial::~TexturedMaterial() = default;
TexturedMaterial::TexturedMaterial(TexturedMaterial&&) noexcept = default;
TexturedMaterial& TexturedMaterial::operator=(TexturedMaterial&&) noexcept = default;

void RenderResourceAccess::Initialize(
	TexturedMaterial& material,
	ID3D12Device* device,
	const std::string& baseColorTexturePath,
	const std::string& opacityTexturePath,
	const std::string& normalTexturePath)
{
	if (material.m_impl == nullptr)
	{
		material.m_impl = std::make_unique<TexturedMaterial::Impl>();
	}
	material.m_impl->LoadTextures(device, baseColorTexturePath, opacityTexturePath, normalTexturePath);
	material.m_impl->CreateDescriptorHeap(device);
	material.m_impl->CreateShaderResourceViews(device);
}

void RenderResourceAccess::Bind(
	const TexturedMaterial& material,
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
