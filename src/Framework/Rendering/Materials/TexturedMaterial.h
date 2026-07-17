#pragma once

#include <memory>

class RenderResourceAccess;

class TexturedMaterial
{
public:
	TexturedMaterial();
	~TexturedMaterial();
	TexturedMaterial(TexturedMaterial&&) noexcept;
	TexturedMaterial& operator=(TexturedMaterial&&) noexcept;
	TexturedMaterial(const TexturedMaterial&) = delete;
	TexturedMaterial& operator=(const TexturedMaterial&) = delete;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	friend class RenderResourceAccess;
};
