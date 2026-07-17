#pragma once

#include "Framework/Rendering/Core/IRenderer.h"

#include <DirectXMath.h>

#include <string_view>

class IEffectService
{
public:
	virtual ~IEffectService() = default;

	virtual void RegisterEffect(std::string_view id, std::string_view assetPath) = 0;
	virtual void Play(std::string_view id, const DirectX::XMFLOAT3& position, float scale = 1.0f) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render(
		IRenderer& renderer,
		const DirectX::XMMATRIX& view,
		const DirectX::XMMATRIX& projection) = 0;
};
