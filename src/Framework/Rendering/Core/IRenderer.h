#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <vector>

class SkinnedVertexBuffer;
class SpriteMaterial;
class SpriteVertexBuffer;
class TexturedMaterial;
class TexturedVertexBuffer;
class VertexBuffer;

// Game-side rendering depends on this engine-facing contract. Backend details
// such as command lists and descriptor heaps remain in Dx12Renderer.
class IRenderer
{
public:
	virtual ~IRenderer() = default;

	virtual void Draw(const VertexBuffer& vertexBuffer, const DirectX::XMMATRIX& world) = 0;
	virtual void DrawScreen(const VertexBuffer& vertexBuffer, const DirectX::XMMATRIX& world) = 0;
	virtual void DrawTextured(
		const TexturedVertexBuffer& vertexBuffer,
		const TexturedMaterial& material,
		const DirectX::XMMATRIX& world) = 0;
	virtual void DrawSprites(const SpriteVertexBuffer& vertexBuffer, const SpriteMaterial& material) = 0;
	virtual void DrawSkinnedTextured(
		const SkinnedVertexBuffer& vertexBuffer,
		const TexturedMaterial& material,
		const DirectX::XMMATRIX& world,
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) = 0;

	virtual std::uint32_t GetWidth() const = 0;
	virtual std::uint32_t GetHeight() const = 0;
};
