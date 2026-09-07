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
	// A Prepare/Activate failure destroys the candidate through RAII. Resources
	// acquired before success must therefore be owned by destructible members.
	virtual void Activate() = 0;
	virtual void Unload() {}
	virtual void Update(float deltaTime, const Input& input) = 0;
	// UI and presentation update once per rendered frame, independently of
	// the fixed simulation step and its buffered input snapshot.
	virtual void UpdateFrame(float deltaTime, const Input& input) { (void)deltaTime; (void)input; }
	virtual void RenderWorld(IRenderer& renderer) const { (void)renderer; }
	virtual void RenderOverlay(IRenderer& renderer) const { (void)renderer; }

	virtual RenderView GetRenderView() const = 0;
	virtual std::string GetRequestedSceneName() const { return {}; }
	virtual bool ShouldLoadRequestedSceneAsync() const { return false; }
	virtual void OnSceneLoadFailed() {}
};
