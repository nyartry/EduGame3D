#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ID3D12Device;
struct ID3D12GraphicsCommandList;

class SkinnedVertexBuffer;
class FrameUploadBuffer;
class SpriteMaterial;
class SpriteVertexBuffer;
class TexturedMaterial;
class TexturedVertexBuffer;
class VertexBuffer;
struct SkinnedVertex;
struct SpriteVertex;
struct TexturedVertex;
struct Vertex;

// Backend-only access to the native implementation hidden by the public
// rendering resource wrappers. Game code never includes this header.
class RenderResourceAccess
{
public:
	static void Initialize(VertexBuffer& buffer, ID3D12Device* device, const std::vector<Vertex>& vertices);
	static void Bind(const VertexBuffer& buffer, ID3D12GraphicsCommandList* commandList);

	static void Initialize(TexturedVertexBuffer& buffer, ID3D12Device* device, const std::vector<TexturedVertex>& vertices);
	static void Bind(const TexturedVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList, FrameUploadBuffer& upload);

	static void Initialize(SkinnedVertexBuffer& buffer, ID3D12Device* device, const std::vector<SkinnedVertex>& vertices);
	static void Bind(const SkinnedVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList);

	static void Initialize(SpriteVertexBuffer& buffer, ID3D12Device* device, std::uint32_t vertexCapacity);
	static void Bind(const SpriteVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList, FrameUploadBuffer& upload);

	static void InitializeSolidColor(
		SpriteMaterial& material,
		ID3D12Device* device,
		std::uint8_t red,
		std::uint8_t green,
		std::uint8_t blue,
		std::uint8_t alpha);
	static void InitializeTexture(SpriteMaterial& material, ID3D12Device* device, const std::string& texturePath, bool useSrgb);
	static void InitializePixels(
		SpriteMaterial& material,
		ID3D12Device* device,
		const std::vector<std::uint8_t>& rgbaPixels,
		std::uint32_t width,
		std::uint32_t height,
		bool useSrgb);
	static void Bind(const SpriteMaterial& material, ID3D12GraphicsCommandList* commandList, std::uint32_t rootParameterIndex);

	static void Initialize(
		TexturedMaterial& material,
		ID3D12Device* device,
		const std::string& baseColorTexturePath,
		const std::string& opacityTexturePath,
		const std::string& normalTexturePath);
	static void Bind(const TexturedMaterial& material, ID3D12GraphicsCommandList* commandList, std::uint32_t rootParameterIndex);
};
