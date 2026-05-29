#pragma once

#include "Scene/Input/Input.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <string>

class Dx12Renderer;
struct ID3D12Device;

struct SceneLoadContext
{
	ID3D12Device* device{};
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
};
