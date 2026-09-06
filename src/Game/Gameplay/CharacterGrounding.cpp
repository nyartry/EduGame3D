#include "Game/Gameplay/CharacterGrounding.h"

#include "Game/Gameplay/Ground.h"
#include "Game/Gameplay/PrimitiveObject.h"

#include <algorithm>

using namespace DirectX;

namespace
{
	bool IsLandingOnSurface(float previousBottomY, float currentBottomY, float surfaceHeight, float tolerance)
	{
		return previousBottomY >= surfaceHeight - tolerance && currentBottomY <= surfaceHeight;
	}
}

CharacterGroundProbe::CharacterGroundProbe(CharacterGroundingSettings settings)
	: m_settings(settings)
{
}

void CharacterGroundProbe::SetSettings(CharacterGroundingSettings settings)
{
	m_settings = settings;
}

void CharacterGroundProbe::SetGround(const Ground* ground)
{
	m_ground = ground;
}

void CharacterGroundProbe::AddLandingSurface(const PrimitiveObject* surface)
{
	if (surface == nullptr)
	{
		return;
	}

	if (std::find(m_landingSurfaces.begin(), m_landingSurfaces.end(), surface) == m_landingSurfaces.end())
	{
		m_landingSurfaces.push_back(surface);
	}
}

void CharacterGroundProbe::ClearLandingSurfaces()
{
	m_landingSurfaces.clear();
}

bool CharacterGroundProbe::TryFindFloor(
	const XMFLOAT3& position,
	float previousBottomY,
	float currentBottomY,
	float& floorHeight) const
{
	bool hasFloor = false;
	float bestFloorHeight = 0.0f;

	if (m_ground != nullptr && m_ground->TryGetHeightAt(position, m_settings.collisionRadius, bestFloorHeight))
	{
		hasFloor = true;
	}

	for (const PrimitiveObject* surface : m_landingSurfaces)
	{
		float surfaceHeight = 0.0f;
		if (surface == nullptr ||
			!surface->TryGetTopSurfaceAt(position, m_settings.collisionRadius, surfaceHeight))
		{
			continue;
		}

		if (!IsLandingOnSurface(previousBottomY, currentBottomY, surfaceHeight, m_settings.landingTolerance))
		{
			continue;
		}

		if (!hasFloor || surfaceHeight > bestFloorHeight)
		{
			bestFloorHeight = surfaceHeight;
			hasFloor = true;
		}
	}

	if (!hasFloor)
	{
		return false;
	}

	floorHeight = bestFloorHeight;
	return true;
}

bool CharacterGroundProbe::ResolveWallCollision(XMFLOAT3& position) const
{
	return m_ground != nullptr && m_ground->ResolveWallCollision(position, m_settings.collisionRadius);
}

CharacterVerticalMotion::CharacterVerticalMotion(CharacterVerticalMotionSettings settings)
	: m_settings(settings)
{
}

void CharacterVerticalMotion::SetSettings(CharacterVerticalMotionSettings settings)
{
	m_settings = settings;
	if (!m_settings.gravityEnabled)
	{
		ResetVerticalVelocity();
		m_programmaticJumpActive = false;
	}
}

void CharacterVerticalMotion::SetGravityEnabled(bool enabled)
{
	m_settings.gravityEnabled = enabled;
	if (!m_settings.gravityEnabled)
	{
		ResetVerticalVelocity();
		m_programmaticJumpActive = false;
	}
}

bool CharacterVerticalMotion::IsGravityEnabled() const
{
	return m_settings.gravityEnabled;
}

bool CharacterVerticalMotion::Update(
	float deltaTime,
	bool wantsJump,
	XMFLOAT3& position,
	const CharacterGroundProbe& groundProbe,
	float rootMotionDisplacementY)
{
	// Keep the start of the whole movement for swept floor/landing queries,
	// including animation-driven descent when gravity is disabled.
	const float previousBottomY = position.y;
	const bool startedJump = m_settings.gravityEnabled && wantsJump && m_isGrounded;
	if (startedJump)
	{
		m_verticalVelocity = m_settings.jumpSpeed;
		m_isGrounded = false;
		m_programmaticJumpActive = true;
	}

	if (!m_programmaticJumpActive)
	{
		position.y += rootMotionDisplacementY;
	}
	if (m_settings.gravityEnabled)
	{
		m_verticalVelocity += m_settings.gravity * deltaTime;
		position.y += m_verticalVelocity * deltaTime;
	}

	float floorHeight = 0.0f;
	if (!groundProbe.TryFindFloor(position, previousBottomY, position.y, floorHeight))
	{
		m_isGrounded = false;
		return startedJump;
	}

	if (position.y <= floorHeight)
	{
		position.y = floorHeight;
		m_verticalVelocity = 0.0f;
		m_isGrounded = true;
		m_programmaticJumpActive = false;
		return startedJump;
	}

	m_isGrounded = false;
	return startedJump;
}

bool CharacterVerticalMotion::IsGrounded() const
{
	return m_isGrounded;
}

float CharacterVerticalMotion::GetVerticalVelocity() const
{
	return m_verticalVelocity;
}

void CharacterVerticalMotion::ResetVerticalVelocity(float velocity)
{
	m_verticalVelocity = velocity;
}
