#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>

namespace MathUtils
{
	inline DirectX::XMFLOAT3 Lerp(
		const DirectX::XMFLOAT3& from,
		const DirectX::XMFLOAT3& to,
		float amount)
	{
		return DirectX::XMFLOAT3
		{
			from.x * (1.0f - amount) + to.x * amount,
			from.y * (1.0f - amount) + to.y * amount,
			from.z * (1.0f - amount) + to.z * amount
		};
	}

	inline float LengthXZ(const DirectX::XMFLOAT3& value)
	{
		return std::sqrt(value.x * value.x + value.z * value.z);
	}

	inline bool TryNormalizeXZ(const DirectX::XMFLOAT3& value, DirectX::XMFLOAT3& normalized)
	{
		normalized = DirectX::XMFLOAT3{ value.x, 0.0f, value.z };
		const float length = LengthXZ(normalized);
		if (length == 0.0f)
		{
			return false;
		}

		normalized.x /= length;
		normalized.z /= length;
		return true;
	}

	inline DirectX::XMFLOAT3 NormalizeXZOrDefault(
		const DirectX::XMFLOAT3& value,
		const DirectX::XMFLOAT3& defaultValue = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f })
	{
		DirectX::XMFLOAT3 normalized{};
		return TryNormalizeXZ(value, normalized) ? normalized : defaultValue;
	}

	inline float SmoothAmount(float sharpness, float deltaTime)
	{
		return std::clamp(1.0f - std::exp(-sharpness * deltaTime), 0.0f, 1.0f);
	}
}
