#pragma once

#include <DirectXMath.h>

#include <string_view>

class IEffectCatalog
{
public:
	virtual ~IEffectCatalog() = default;
	virtual void RegisterEffect(std::string_view id, std::string_view assetPath) = 0;
};

// Game-facing effect contract. Backend update and rendering deliberately stay
// outside this interface and are driven by the composition root.
class IEffectPlayer
{
public:
	virtual ~IEffectPlayer() = default;
	virtual void Play(std::string_view id, const DirectX::XMFLOAT3& position, float scale = 1.0f) = 0;
};
