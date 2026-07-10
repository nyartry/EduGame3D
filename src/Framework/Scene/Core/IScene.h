#pragma once

#include "Framework/Scene/Input/Input.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <string>

class Dx12Renderer;
class IAudioService;
struct ID3D12CommandQueue;
struct ID3D12Device;

struct SceneLoadContext
{
	ID3D12Device* device{};
	ID3D12CommandQueue* commandQueue{};
	IAudioService* audio{};
	UINT width{};
	UINT height{};
};

class IScene
{
public:
	virtual ~IScene() = default;

	virtual void Load(const SceneLoadContext& context) = 0;
	virtual void Unload() {}
	virtual void Update(float deltaTime, const Input& input) = 0;
	virtual void Render(Dx12Renderer& renderer) const = 0;

	virtual DirectX::XMMATRIX GetViewProjectionMatrix() const = 0;
	virtual std::string GetRequestedSceneName() const { return {}; }
	virtual bool ShouldLoadRequestedSceneAsync() const { return false; }
};
