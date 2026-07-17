#pragma once

#include <cstdint>
#include <string>
#include <vector>

class SkinnedVertexBuffer;
class SpriteMaterial;
class SpriteVertexBuffer;
class TexturedMaterial;
class TexturedVertexBuffer;
class VertexBuffer;
struct SkinnedVertex;
struct SpriteVertex;
struct TexturedVertex;
struct Vertex;

// Resource creation is separated from frame rendering so scenes never receive
// an ID3D12Device or another backend-native object.
class IRenderDevice
{
public:
	virtual ~IRenderDevice() = default;

	virtual void CreateVertexBuffer(VertexBuffer& buffer, const std::vector<Vertex>& vertices) = 0;
	virtual void CreateTexturedVertexBuffer(TexturedVertexBuffer& buffer, const std::vector<TexturedVertex>& vertices) = 0;
	virtual void CreateSkinnedVertexBuffer(SkinnedVertexBuffer& buffer, const std::vector<SkinnedVertex>& vertices) = 0;
	virtual void CreateSpriteVertexBuffer(SpriteVertexBuffer& buffer, std::uint32_t vertexCapacity) = 0;

	virtual void CreateSolidColorSpriteMaterial(
		SpriteMaterial& material,
		std::uint8_t red,
		std::uint8_t green,
		std::uint8_t blue,
		std::uint8_t alpha) = 0;
	virtual void CreateTextureSpriteMaterial(
		SpriteMaterial& material,
		const std::string& texturePath,
		bool useSrgb = false) = 0;
	virtual void CreatePixelSpriteMaterial(
		SpriteMaterial& material,
		const std::vector<std::uint8_t>& rgbaPixels,
		std::uint32_t width,
		std::uint32_t height,
		bool useSrgb = false) = 0;
	virtual void CreateTexturedMaterial(
		TexturedMaterial& material,
		const std::string& baseColorTexturePath,
		const std::string& opacityTexturePath,
		const std::string& normalTexturePath) = 0;
};
