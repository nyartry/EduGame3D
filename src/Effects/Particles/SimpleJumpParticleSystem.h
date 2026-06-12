#pragma once

#include "Rendering/Buffers/VertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <array>
#include <cstddef>

class Dx12Renderer;
struct ID3D12Device;

class SimpleJumpParticleSystem
{
public:
	void Initialize(ID3D12Device* device);
	void Emit(const DirectX::XMFLOAT3& position);
	void Update(float deltaTime);
	void Render(Dx12Renderer& renderer) const;

private:
	struct Particle
	{
		DirectX::XMFLOAT3 position{};
		DirectX::XMFLOAT3 velocity{};
		float age{};
		float lifetime{};
		float size{};
		bool active{};
	};

	static constexpr size_t MaxParticles = 3;

	void BuildParticleMesh(ID3D12Device* device);

	VertexBuffer m_particleMesh;
	std::array<Particle, MaxParticles> m_particles{};
	bool m_initialized{};
};
