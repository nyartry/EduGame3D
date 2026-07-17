#pragma once

#include "Framework/Scene/Input/Input.h"

#include <DirectXMath.h>
#include <cstdint>
#include <string>

class IAudioService;
class IEffectService;
class IRenderDevice;
class IRenderer;
class IUiService;

struct SceneLoadContext
{
	IRenderDevice* renderDevice{};
	IAudioService* audio{};
	IEffectService* effects{};
	IUiService* ui{};
	std::uint32_t width{};
	std::uint32_t height{};
};

class IScene
{
public:
	virtual ~IScene() = default;

	virtual void Load(const SceneLoadContext& context) = 0;
	virtual void Unload() {}
	virtual void Update(float deltaTime, const Input& input) = 0;
	virtual void Render(IRenderer& renderer) const = 0;

	virtual DirectX::XMMATRIX GetViewProjectionMatrix() const = 0;
	virtual std::string GetRequestedSceneName() const { return {}; }
	virtual bool ShouldLoadRequestedSceneAsync() const { return false; }
};
