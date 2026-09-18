#include "Game/Effects/Particles/SimpleJumpParticleSystem.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Framework/Rendering/Geometry/Vertex.h"

#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

namespace
{
	constexpr float BaseParticleSize = 0.035f;
	constexpr float ParticleLifetimeSeconds = 0.22f;

	constexpr XMFLOAT4 ParticleColor{ 0.78f, 1.0f, 0.50f, 1.0f };

	constexpr std::array<XMFLOAT3, 3> SpawnOffsets
	{
		XMFLOAT3{ 0.00f, 0.025f, 0.00f },
		XMFLOAT3{ 0.05f, 0.020f, 0.02f },
		XMFLOAT3{ -0.04f, 0.020f, -0.03f },
	};

	constexpr std::array<XMFLOAT3, 3> SpawnVelocities
	{
		XMFLOAT3{ 0.00f, 0.16f, 0.00f },
		XMFLOAT3{ 0.18f, 0.11f, 0.06f },
		XMFLOAT3{ -0.14f, 0.10f, -0.10f },
	};

	Vertex MakeVertex(float x, float y, float z)
	{
		return Vertex
		{
			{ x, y, z },
			{ ParticleColor.x, ParticleColor.y, ParticleColor.z, ParticleColor.w }
		};
	}
}

void SimpleJumpParticleSystem::Initialize(IRenderDevice& device)
{
	BuildParticleMesh(device);
	m_initialized = true;
}

void SimpleJumpParticleSystem::Emit(const XMFLOAT3& position)
{
	if (!m_initialized)
	{
		return;
	}

	for (size_t index = 0; index < MaxParticles; ++index)
	{
		Particle& particle = m_particles[index];
		particle.position =
		{
			position.x + SpawnOffsets[index].x,
			position.y + SpawnOffsets[index].y,
			position.z + SpawnOffsets[index].z
		};
		particle.velocity = SpawnVelocities[index];
		particle.age = 0.0f;
		particle.lifetime = ParticleLifetimeSeconds;
		particle.size = BaseParticleSize;
		particle.active = true;
	}
}

void SimpleJumpParticleSystem::Update(float deltaTime)
{
	if (!m_initialized)
	{
		return;
	}

	for (Particle& particle : m_particles)
	{
		if (!particle.active)
		{
			continue;
		}

		particle.age += deltaTime;
		if (particle.age >= particle.lifetime)
		{
			particle.active = false;
			continue;
		}

		particle.position.x += particle.velocity.x * deltaTime;
		particle.position.y += particle.velocity.y * deltaTime;
		particle.position.z += particle.velocity.z * deltaTime;
	}
}

void SimpleJumpParticleSystem::Render(IRenderer& renderer) const
{
	if (!m_initialized)
	{
		return;
	}

	for (const Particle& particle : m_particles)
	{
		if (!particle.active)
		{
			continue;
		}

		const float lifeRatio = particle.age / particle.lifetime;
		const float scale = particle.size * (1.0f - 0.35f * lifeRatio);
		const XMMATRIX world =
			XMMatrixScaling(scale, scale, scale) *
			XMMatrixTranslation(particle.position.x, particle.position.y, particle.position.z);
		renderer.Draw(m_particleMesh, world);
	}
}

void SimpleJumpParticleSystem::BuildParticleMesh(IRenderDevice& device)
{
	const std::vector<Vertex> vertices =
	{
		MakeVertex(0.0f, 1.0f, 0.0f),
		MakeVertex(1.0f, 0.0f, 0.0f),
		MakeVertex(0.0f, 0.0f, 1.0f),

		MakeVertex(0.0f, 1.0f, 0.0f),
		MakeVertex(0.0f, 0.0f, 1.0f),
		MakeVertex(-1.0f, 0.0f, 0.0f),

		MakeVertex(0.0f, 1.0f, 0.0f),
		MakeVertex(-1.0f, 0.0f, 0.0f),
		MakeVertex(0.0f, 0.0f, -1.0f),

		MakeVertex(0.0f, 1.0f, 0.0f),
		MakeVertex(0.0f, 0.0f, -1.0f),
		MakeVertex(1.0f, 0.0f, 0.0f),
	};
	device.CreateVertexBuffer(m_particleMesh, vertices);
}
