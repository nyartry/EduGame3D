#include "Framework/Rendering/Materials/TexturedMaterial.h"

#include "Framework/Common/Common.h"
#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"
#include "Framework/Rendering/Materials/Texture2D.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <memory>
#include <string>
#include <utility>
#include <map>
#include <mutex>
#include <tuple>

namespace
{
	std::string TexturePath(const std::string& path)
	{
		return path.empty() ? std::string{} : AssetPathResolver::ResolveUtf8(path);
	}

	std::shared_ptr<Texture2D> LoadSharedTexture(ID3D12Device* device, const std::string& path,
		bool useSrgb, std::uint32_t fallbackColor)
	{
		// Color-space is part of the key: an opacity/normal map must not share
		// an sRGB resource with a base-color map of the same file.
		using Key = std::tuple<ID3D12Device*, std::string, bool, std::uint32_t>;
		static std::mutex mutex;
		static std::map<Key, std::weak_ptr<Texture2D>> cache;
		std::lock_guard lock(mutex);
		const Key key{device, path, useSrgb, path.empty() ? fallbackColor : 0};
		if (const auto found = cache.find(key); found != cache.end())
			if (auto texture = found->second.lock()) return texture;
		std::erase_if(cache, [](const auto& entry) { return entry.second.expired(); });
		auto texture = std::make_shared<Texture2D>();
		if (path.empty())
		{
			texture->InitializeSolidColor(device, static_cast<UINT8>(fallbackColor >> 24),
				static_cast<UINT8>(fallbackColor >> 16), static_cast<UINT8>(fallbackColor >> 8),
				static_cast<UINT8>(fallbackColor), useSrgb);
		}
		else texture->Initialize(device, path, useSrgb);
		cache[key] = texture;
		return texture;
	}
}

struct TexturedMaterial::Impl
{
	static constexpr std::uint32_t TextureCount = 3;

	void LoadTextures(
		ID3D12Device* device,
		const std::string& baseColorTexturePath,
		const std::string& opacityTexturePath,
		const std::string& normalTexturePath)
	{
		baseColorTexture = LoadSharedTexture(device, baseColorTexturePath, true, 0xffffffff);
		opacityTexture = LoadSharedTexture(device, opacityTexturePath, false, 0xffffffff);
		normalTexture = LoadSharedTexture(device, normalTexturePath, false, 0x8080ffff);
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
		baseColorTexture->CreateShaderResourceView(device, handle);
		handle.ptr += descriptorSize;
		opacityTexture->CreateShaderResourceView(device, handle);
		handle.ptr += descriptorSize;
		normalTexture->CreateShaderResourceView(device, handle);
	}

	std::shared_ptr<Texture2D> baseColorTexture;
	std::shared_ptr<Texture2D> opacityTexture;
	std::shared_ptr<Texture2D> normalTexture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
};

TexturedMaterial::TexturedMaterial()
	: m_impl(std::make_shared<Impl>())
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
	using Key = std::tuple<ID3D12Device*, std::string, std::string, std::string>;
	static std::mutex mutex;
	static std::map<Key, std::weak_ptr<TexturedMaterial::Impl>> cache;
	const auto basePath = TexturePath(baseColorTexturePath);
	const auto opacityPath = TexturePath(opacityTexturePath);
	const auto normalPath = TexturePath(normalTexturePath);
	const Key key{device, basePath, opacityPath, normalPath};
	std::lock_guard lock(mutex);
	if (const auto found = cache.find(key); found != cache.end())
	{
		if (auto existing = found->second.lock())
		{
			material.m_impl = std::move(existing);
			return;
		}
	}
	std::erase_if(cache, [](const auto& entry) { return entry.second.expired(); });
	auto created = std::make_shared<TexturedMaterial::Impl>();
	created->LoadTextures(device, basePath, opacityPath, normalPath);
	created->CreateDescriptorHeap(device);
	created->CreateShaderResourceViews(device);
	cache[key] = created;
	material.m_impl = std::move(created);
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
