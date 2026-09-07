#include "Game/Gameplay/CharacterGrounding.h"

using namespace DirectX;

CharacterGroundProbe::CharacterGroundProbe(CharacterGroundingSettings settings)
	: m_settings(settings)
{
}

void CharacterGroundProbe::SetSettings(CharacterGroundingSettings settings)
{
	m_settings = settings;
}

void CharacterGroundProbe::SetCollisionQuery(const ICollisionQuery* query)
{
	m_query = query;
}

void CharacterGroundProbe::SetGround(const ICollisionSurface* ground)
{
	if (m_ground != nullptr) m_localWorld.RemoveSurface(*m_ground);
	m_ground = ground;
	if (ground != nullptr) m_localWorld.RegisterSurface(*ground);
}

void CharacterGroundProbe::AddLandingSurface(const ICollisionSurface* surface)
{
	if (surface != nullptr) m_localWorld.RegisterSurface(*surface);
}

void CharacterGroundProbe::ClearLandingSurfaces()
{
	m_localWorld.Clear();
	if (m_ground != nullptr) m_localWorld.RegisterSurface(*m_ground);
}

bool CharacterGroundProbe::TryFindFloor(
	const XMFLOAT3& position,
	float previousBottomY,
	float currentBottomY,
	float& floorHeight) const
{
	const ICollisionQuery& world = m_query != nullptr ? *m_query : m_localWorld;
	FloorHit hit;
	if (!world.TryFindFloor({ position, m_settings.collisionRadius, previousBottomY,
		currentBottomY, m_settings.landingTolerance }, hit)) return false;
	floorHeight = hit.height;
	return true;
}

bool CharacterGroundProbe::ResolveWallCollision(XMFLOAT3& position) const
{
	const ICollisionQuery& world = m_query != nullptr ? *m_query : m_localWorld;
	return world.ResolveBoundaries(position, m_settings.collisionRadius);
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
