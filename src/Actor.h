#pragma once

#include <Windows.h>

class Dx12Renderer;
struct ID3D12Device;

class Actor
{
public:
	virtual ~Actor() = default;

	virtual void Initialize(ID3D12Device* device) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Draw(Dx12Renderer& renderer) const = 0;
};
