#pragma once

#include "Framework/Scene/Input/Input.h"

#include <DirectXMath.h>
#include <string>

class IRenderer;

struct RenderView
{
	DirectX::XMMATRIX view{ DirectX::XMMatrixIdentity() };
	DirectX::XMMATRIX projection{ DirectX::XMMatrixIdentity() };
	bool effectsEnabled{};

	DirectX::XMMATRIX GetViewProjection() const
	{
		return view * projection;
	}
};

class IScene
{
public:
	virtual ~IScene() = default;

	// Prepare runs on a worker thread during asynchronous transitions and must
	// only perform CPU-side work owned by the scene.
	virtual void Prepare() {}
	// Activate always runs on the main thread and may create GPU/UI resources.
	virtual void Activate() = 0;
	virtual void Unload() {}
	virtual void Update(float deltaTime, const Input& input) = 0;
	virtual void RenderWorld(IRenderer& renderer) const { (void)renderer; }
	virtual void RenderOverlay(IRenderer& renderer) const { (void)renderer; }

	virtual RenderView GetRenderView() const = 0;
	virtual std::string GetRequestedSceneName() const { return {}; }
	virtual bool ShouldLoadRequestedSceneAsync() const { return false; }
};
