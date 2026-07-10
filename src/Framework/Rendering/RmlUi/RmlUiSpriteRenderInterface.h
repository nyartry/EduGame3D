#pragma once

#include "Framework/Rendering/Sprites/SpriteBatch.h"

#include <RmlUi/Core/RenderInterface.h>

#include <vector>

class RmlUiSpriteRenderInterface final : public Rml::RenderInterface
{
public:
	void Begin(SpriteBatch& batch);
	void End();
	UINT GetLastRenderedTriangleCount() const;

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

private:
	struct Geometry
	{
		std::vector<Rml::Vertex> vertices;
		std::vector<int> indices;
	};

	DirectX::XMFLOAT4 ConvertColor(const Rml::ColourbPremultiplied& color) const;

	SpriteBatch* m_batch{};
	UINT m_lastRenderedTriangleCount{};
	bool m_scissorEnabled{};
	Rml::Rectanglei m_scissorRegion{};
};
