#pragma once

#include "Rendering/VertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class LoadingOverlay
{
public:
	void Initialize(ID3D12Device* device);
	void Update(float deltaTime);
	void Render(Dx12Renderer& renderer) const;

private:
	struct LoadingFrame
	{
		VertexBuffer text;
		VertexBuffer bar;
	};

	void BuildFrames(ID3D12Device* device);

	std::vector<LoadingFrame> m_frames;
	float m_elapsedTime{};
};
