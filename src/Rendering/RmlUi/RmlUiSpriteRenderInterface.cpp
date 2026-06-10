#include "Rendering/RmlUi/RmlUiSpriteRenderInterface.h"

#include <DirectXMath.h>

using namespace DirectX;

void RmlUiSpriteRenderInterface::Begin(SpriteBatch& batch)
{
	m_batch = &batch;
	m_lastRenderedTriangleCount = 0;
}

void RmlUiSpriteRenderInterface::End()
{
	m_batch = nullptr;
}

UINT RmlUiSpriteRenderInterface::GetLastRenderedTriangleCount() const
{
	return m_lastRenderedTriangleCount;
}

Rml::CompiledGeometryHandle RmlUiSpriteRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	auto* geometry = new Geometry();
	geometry->vertices.assign(vertices.begin(), vertices.end());
	geometry->indices.assign(indices.begin(), indices.end());
	return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
}

void RmlUiSpriteRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometryHandle, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	if (m_batch == nullptr || geometryHandle == 0)
	{
		return;
	}

	// This minimal bridge is intended for untextured RmlUi layout geometry.
	// Textured geometry such as RmlUi font atlases needs a real texture-aware renderer.
	if (texture != 0)
	{
		return;
	}

	const auto* geometry = reinterpret_cast<const Geometry*>(geometryHandle);
	for (size_t index = 0; index + 2 < geometry->indices.size(); index += 3)
	{
		const Rml::Vertex& a = geometry->vertices[static_cast<size_t>(geometry->indices[index])];
		const Rml::Vertex& b = geometry->vertices[static_cast<size_t>(geometry->indices[index + 1])];
		const Rml::Vertex& c = geometry->vertices[static_cast<size_t>(geometry->indices[index + 2])];

		const XMFLOAT2 pointA{ a.position.x + translation.x, a.position.y + translation.y };
		const XMFLOAT2 pointB{ b.position.x + translation.x, b.position.y + translation.y };
		const XMFLOAT2 pointC{ c.position.x + translation.x, c.position.y + translation.y };

		m_batch->DrawTriangle(pointA, pointB, pointC, ConvertColor(a.colour));
		++m_lastRenderedTriangleCount;
	}
}

void RmlUiSpriteRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	delete reinterpret_cast<Geometry*>(geometry);
}

Rml::TextureHandle RmlUiSpriteRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	(void)source;
	texture_dimensions = {};
	return 0;
}

Rml::TextureHandle RmlUiSpriteRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	(void)source;
	(void)source_dimensions;
	return 0;
}

void RmlUiSpriteRenderInterface::ReleaseTexture(Rml::TextureHandle texture)
{
	(void)texture;
}

void RmlUiSpriteRenderInterface::EnableScissorRegion(bool enable)
{
	m_scissorEnabled = enable;
}

void RmlUiSpriteRenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
	m_scissorRegion = region;
}

XMFLOAT4 RmlUiSpriteRenderInterface::ConvertColor(const Rml::ColourbPremultiplied& color) const
{
	(void)m_scissorEnabled;
	(void)m_scissorRegion;
	const Rml::Colourb unpremultiplied = color.ToNonPremultiplied();
	return {
		static_cast<float>(unpremultiplied.red) / 255.0f,
		static_cast<float>(unpremultiplied.green) / 255.0f,
		static_cast<float>(unpremultiplied.blue) / 255.0f,
		static_cast<float>(unpremultiplied.alpha) / 255.0f
	};
}
