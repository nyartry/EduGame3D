#pragma once

#include "Framework/Core/Math/Aabb.h"

// Asset-space fit only; an actor's world transform remains independent.
struct ModelFit
{
	DirectX::XMFLOAT3 origin{};
	float scale{ 1.0f };

	DirectX::XMFLOAT3 Apply(const DirectX::XMFLOAT3& position) const
	{
		return
		{
			static_cast<float>((static_cast<double>(position.x) - origin.x) * scale),
			static_cast<float>((static_cast<double>(position.y) - origin.y) * scale),
			static_cast<float>((static_cast<double>(position.z) - origin.z) * scale)
		};
	}
};

inline bool TryCreateModelFit(const Aabb& bounds, float targetHeight, ModelFit& fit)
{
	fit = {};
	if (bounds.IsEmpty() || !std::isfinite(targetHeight) || targetHeight <= 0.0f) return false;
	const double height = static_cast<double>(bounds.Max().y) - bounds.Min().y;
	if (height <= 0.0) return false;
	const double scale = targetHeight / height;
	if (!std::isfinite(scale) || scale > (std::numeric_limits<float>::max)() || static_cast<float>(scale) <= 0.0f) return false;
	const auto center = bounds.Center();
	fit = { { center.x, bounds.Min().y, center.z }, static_cast<float>(scale) };
	return true;
}
