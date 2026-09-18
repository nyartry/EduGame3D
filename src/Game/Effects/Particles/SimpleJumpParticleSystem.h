#pragma once

#include "Framework/Rendering/Buffers/VertexBuffer.h"

#include <DirectXMath.h>
#include <array>
#include <cstddef>

class IRenderDevice;
class IRenderer;

class SimpleJumpParticleSystem
{
public:
	void Initialize(IRenderDevice& device);
	void Emit(const DirectX::XMFLOAT3& position);
	void Update(float deltaTime);
	void Render(IRenderer& renderer) const;

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

	void BuildParticleMesh(IRenderDevice& device);

	VertexBuffer m_particleMesh;
	std::array<Particle, MaxParticles> m_particles{};
	bool m_initialized{};
};
