#include "Framework/Scene/Cameras/SpringFollowCamera.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

void SpringFollowCamera::Update(float deltaTime, const Input&)
{
	if (!m_hasTarget)
	{
		return;
	}

	// The reference implementation uses a critically damped spring. Capping the
	// integration step keeps the explicit integration stable after a breakpoint
	// or window stall.
	const float step = std::min(deltaTime, 1.0f / 30.0f);
	const float damping = 2.0f * std::sqrt(m_springConstant);
	const XMFLOAT3 idealPosition = ComputeIdealPosition();
	const XMFLOAT3 acceleration
	{
		m_springConstant * (idealPosition.x - m_position.x) - damping * m_velocity.x,
		m_springConstant * (idealPosition.y - m_position.y) - damping * m_velocity.y,
		m_springConstant * (idealPosition.z - m_position.z) - damping * m_velocity.z
	};

	m_velocity.x += acceleration.x * step;
	m_velocity.y += acceleration.y * step;
	m_velocity.z += acceleration.z * step;
	m_position.x += m_velocity.x * step;
	m_position.y += m_velocity.y * step;
	m_position.z += m_velocity.z * step;
	SetViewPose(m_position, ComputeViewTarget());
}

void SpringFollowCamera::SetTarget(const XMFLOAT3& position, float rotationY)
{
	m_followTarget = position;
	m_targetRotationY = rotationY;
	if (!m_hasTarget)
	{
		m_hasTarget = true;
		m_position = ComputeIdealPosition();
		m_velocity = XMFLOAT3{};
		SetViewPose(m_position, ComputeViewTarget());
	}
}

XMFLOAT3 SpringFollowCamera::ComputeIdealPosition() const
{
	const XMFLOAT3 forward
	{
		-std::sin(m_targetRotationY),
		0.0f,
		-std::cos(m_targetRotationY)
	};
	return XMFLOAT3
	{
		m_followTarget.x - forward.x * m_horizontalDistance,
		m_followTarget.y + m_verticalDistance,
		m_followTarget.z - forward.z * m_horizontalDistance
	};
}

XMFLOAT3 SpringFollowCamera::ComputeViewTarget() const
{
	const XMFLOAT3 forward
	{
		-std::sin(m_targetRotationY),
		0.0f,
		-std::cos(m_targetRotationY)
	};
	return XMFLOAT3
	{
		m_followTarget.x + forward.x * m_targetDistance,
		m_followTarget.y + m_lookAtHeight,
		m_followTarget.z + forward.z * m_targetDistance
	};
}
