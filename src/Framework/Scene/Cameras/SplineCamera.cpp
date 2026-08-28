#include "Framework/Scene/Cameras/SplineCamera.h"

#include <algorithm>

using namespace DirectX;

namespace
{
	XMFLOAT3 Add(const XMFLOAT3& left, const XMFLOAT3& right)
	{
		return XMFLOAT3{ left.x + right.x, left.y + right.y, left.z + right.z };
	}
}

void SplineCamera::Update(float deltaTime, const Input&)
{
	if (!m_hasPath)
	{
		return;
	}

	if (!m_paused)
	{
		m_amount += m_speed * deltaTime;
		while (m_amount >= 1.0f && !m_paused)
		{
			if (m_index < m_controlPoints.size() - 3)
			{
				++m_index;
				m_amount -= 1.0f;
			}
			else
			{
				m_amount = 1.0f;
				m_paused = true;
			}
		}
	}

	const XMFLOAT3 position = ComputePosition(m_index, m_amount);
	const XMFLOAT3 viewTarget = ComputePosition(m_index, m_amount + 0.01f);
	SetViewPose(position, viewTarget);
}

void SplineCamera::SetTarget(const XMFLOAT3& position, float)
{
	if (!m_hasPath)
	{
		BuildDefaultPath(position);
	}
}

XMFLOAT3 SplineCamera::ComputePosition(std::size_t startIndex, float amount) const
{
	if (m_controlPoints.empty())
	{
		return {};
	}
	if (startIndex == 0 || startIndex + 2 >= m_controlPoints.size())
	{
		return m_controlPoints[std::min(startIndex, m_controlPoints.size() - 1)];
	}

	const XMFLOAT3& p0 = m_controlPoints[startIndex - 1];
	const XMFLOAT3& p1 = m_controlPoints[startIndex];
	const XMFLOAT3& p2 = m_controlPoints[startIndex + 1];
	const XMFLOAT3& p3 = m_controlPoints[startIndex + 2];
	const float amount2 = amount * amount;
	const float amount3 = amount2 * amount;
	return XMFLOAT3
	{
		0.5f * (2.0f * p1.x + (-p0.x + p2.x) * amount +
			(2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * amount2 +
			(-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * amount3),
		0.5f * (2.0f * p1.y + (-p0.y + p2.y) * amount +
			(2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * amount2 +
			(-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * amount3),
		0.5f * (2.0f * p1.z + (-p0.z + p2.z) * amount +
			(2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * amount2 +
			(-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * amount3)
	};
}

void SplineCamera::BuildDefaultPath(const XMFLOAT3& anchor)
{
	// Duplicate end points make the first and last authored positions usable by
	// the Catmull-Rom interpolation, matching the Chapter09 camera technique.
	const XMFLOAT3 first = Add(anchor, XMFLOAT3{ -6.0f, 3.0f, 5.0f });
	const XMFLOAT3 last = Add(anchor, XMFLOAT3{ 0.0f, 3.5f, 6.0f });
	m_controlPoints =
	{
		first,
		first,
		Add(anchor, XMFLOAT3{ -4.0f, 4.0f, 1.0f }),
		Add(anchor, XMFLOAT3{ 0.0f, 5.0f, -5.0f }),
		Add(anchor, XMFLOAT3{ 4.0f, 3.0f, -2.0f }),
		Add(anchor, XMFLOAT3{ 6.0f, 4.0f, 4.0f }),
		last,
		last
	};
	m_index = 1;
	m_amount = 0.0f;
	m_paused = false;
	m_hasPath = true;
	SetViewPose(ComputePosition(m_index, m_amount), ComputePosition(m_index, m_amount + 0.01f));
}
