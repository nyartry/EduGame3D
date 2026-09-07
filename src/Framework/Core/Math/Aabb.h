#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

class Aabb
{
public:
	// Nonfinite points are rejected without changing the bounds. Empty bounds
	// report zero min/max/center/size; a single finite point is a valid box.
	bool AddPoint(const DirectX::XMFLOAT3& point)
	{
		if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) return false;
		if (m_empty)
		{
			m_min = m_max = point;
			m_empty = false;
			return true;
		}
		m_min = { (std::min)(m_min.x, point.x), (std::min)(m_min.y, point.y), (std::min)(m_min.z, point.z) };
		m_max = { (std::max)(m_max.x, point.x), (std::max)(m_max.y, point.y), (std::max)(m_max.z, point.z) };
		return true;
	}
	bool IsEmpty() const { return m_empty; }
	const DirectX::XMFLOAT3& Min() const { return m_min; }
	const DirectX::XMFLOAT3& Max() const { return m_max; }
	DirectX::XMFLOAT3 Center() const
	{
		return { std::midpoint(m_min.x, m_max.x), std::midpoint(m_min.y, m_max.y), std::midpoint(m_min.z, m_max.z) };
	}
	// A span beyond the float range saturates, so finite points keep finite results.
	DirectX::XMFLOAT3 Size() const
	{
		const auto span = [](float minimum, float maximum)
		{
			return static_cast<float>((std::min)(static_cast<double>(maximum) - minimum,
				static_cast<double>((std::numeric_limits<float>::max)())));
		};
		return { span(m_min.x, m_max.x), span(m_min.y, m_max.y), span(m_min.z, m_max.z) };
	}

private:
	DirectX::XMFLOAT3 m_min{};
	DirectX::XMFLOAT3 m_max{};
	bool m_empty{ true };
};
