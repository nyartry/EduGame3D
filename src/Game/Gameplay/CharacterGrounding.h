#pragma once

#include <DirectXMath.h>

#include <vector>

class Ground;
class PrimitiveObject;

struct CharacterGroundingSettings
{
	float collisionRadius{ 0.35f };
	float landingTolerance{ 0.05f };
};

class CharacterGroundProbe
{
public:
	explicit CharacterGroundProbe(CharacterGroundingSettings settings = {});

	void SetSettings(CharacterGroundingSettings settings);
	void SetGround(const Ground* ground);
	void AddLandingSurface(const PrimitiveObject* surface);
	void ClearLandingSurfaces();

	bool TryFindFloor(
		const DirectX::XMFLOAT3& position,
		float previousBottomY,
		float currentBottomY,
		float& floorHeight) const;
	bool ResolveWallCollision(DirectX::XMFLOAT3& position) const;

private:
	CharacterGroundingSettings m_settings;
	const Ground* m_ground{};
	std::vector<const PrimitiveObject*> m_landingSurfaces;
};

struct CharacterVerticalMotionSettings
{
	float gravity{ -18.0f };
	float jumpSpeed{ 7.0f };
	bool gravityEnabled{ true };
};

class CharacterVerticalMotion
{
public:
	explicit CharacterVerticalMotion(CharacterVerticalMotionSettings settings = {});

	void SetSettings(CharacterVerticalMotionSettings settings);
	void SetGravityEnabled(bool enabled);
	bool IsGravityEnabled() const;
	// Returns true only when a programmatic jump is accepted this update.
	// Animation Y is additive, but is suppressed from jump takeoff through landing.
	bool Update(
		float deltaTime,
		bool wantsJump,
		DirectX::XMFLOAT3& position,
		const CharacterGroundProbe& groundProbe,
		float rootMotionDisplacementY = 0.0f);

	bool IsGrounded() const;
	float GetVerticalVelocity() const;
	void ResetVerticalVelocity(float velocity = 0.0f);

private:
	CharacterVerticalMotionSettings m_settings;
	float m_verticalVelocity{};
	bool m_isGrounded{};
	bool m_programmaticJumpActive{};
};
