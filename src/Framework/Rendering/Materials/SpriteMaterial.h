#pragma once

#include <memory>

class RenderResourceAccess;

class SpriteMaterial
{
public:
	SpriteMaterial();
	~SpriteMaterial();
	SpriteMaterial(SpriteMaterial&&) noexcept;
	SpriteMaterial& operator=(SpriteMaterial&&) noexcept;
	SpriteMaterial(const SpriteMaterial&) = delete;
	SpriteMaterial& operator=(const SpriteMaterial&) = delete;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	friend class RenderResourceAccess;
};
