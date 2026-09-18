#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <limits>

// Shared, stateless math. Distances use world units; angles use radians.
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
		return std::hypot(value.x, value.z);
	}

	// Failure always clears the output. Double intermediates avoid overflow for
	// finite float vectors near FLT_MAX and underflow for very small vectors.
	inline bool TryNormalize(
		const DirectX::XMFLOAT3& value,
		DirectX::XMFLOAT3& normalized,
		float epsilon = 1.0e-6f)
	{
		const double x = value.x;
		const double y = value.y;
		const double z = value.z;
		normalized = {};
		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
			!std::isfinite(epsilon) || epsilon < 0.0f)
		{
			return false;
		}

		const double length = std::hypot(x, y, z);
		if (length <= epsilon)
		{
			return false;
		}
		normalized = { static_cast<float>(x / length), static_cast<float>(y / length), static_cast<float>(z / length) };
		return true;
	}

	inline DirectX::XMFLOAT3 NormalizeOrDefault(
		const DirectX::XMFLOAT3& value,
		const DirectX::XMFLOAT3& defaultValue = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f })
	{
		DirectX::XMFLOAT3 normalized{};
		return TryNormalize(value, normalized) ? normalized : defaultValue;
	}

	// XZ intentionally ignores Y and retains the existing exact-zero cutoff.
	inline bool TryNormalizeXZ(
		const DirectX::XMFLOAT3& value,
		DirectX::XMFLOAT3& normalized,
		float epsilon = 0.0f)
	{
		const DirectX::XMFLOAT3 horizontal{ value.x, 0.0f, value.z };
		return TryNormalize(horizontal, normalized, epsilon);
	}

	inline DirectX::XMFLOAT3 NormalizeXZOrDefault(
		const DirectX::XMFLOAT3& value,
		const DirectX::XMFLOAT3& defaultValue = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f })
	{
		DirectX::XMFLOAT3 normalized{};
		return TryNormalizeXZ(value, normalized) ? normalized : defaultValue;
	}

	namespace Detail
	{
		inline float WrapAngle(double radians)
		{
			if (!std::isfinite(radians))
			{
				return 0.0f;
			}
			double result = std::remainder(radians, static_cast<double>(DirectX::XM_2PI));
			if (result >= DirectX::XM_PI)
			{
				result -= DirectX::XM_2PI;
			}
			const float wrapped = static_cast<float>(result);
			// Rounding back to float must preserve the half-open interval.
			return wrapped >= DirectX::XM_PI ? -DirectX::XM_PI : wrapped;
		}
	}

	// Canonical interval is [-pi, pi). Nonfinite inputs return zero.
	inline float NormalizeAngle(float radians)
	{
		return Detail::WrapAngle(radians);
	}

	// Shortest-path interpolation; amount may extrapolate, as with Lerp.
	// A nonfinite amount leaves the normalized starting angle unchanged.
	inline float LerpAngle(float from, float to, float amount)
	{
		const float start = NormalizeAngle(from);
		if (!std::isfinite(amount))
		{
			return start;
		}
		const float difference = NormalizeAngle(NormalizeAngle(to) - start);
		return Detail::WrapAngle(static_cast<double>(start) + static_cast<double>(difference) * amount);
	}

	// Exponential smoothing, stable even when sharpness * deltaTime is tiny.
	// Nonpositive or nonfinite parameters leave the current value unchanged.
	inline float SmoothAmount(float sharpness, float deltaTime)
	{
		if (!std::isfinite(sharpness) || !std::isfinite(deltaTime) || sharpness <= 0.0f || deltaTime <= 0.0f)
		{
			return 0.0f;
		}
		return static_cast<float>(-std::expm1(-static_cast<double>(sharpness) * deltaTime));
	}

	// Row-vector affine transforms require inverse-transpose of the linear 3x3
	// for normals. Translation is deliberately excluded. The output is identity
	// on failure, allowing renderers to choose an explicit fallback.
	inline bool TryCreateNormalMatrix(const DirectX::XMMATRIX& world, DirectX::XMMATRIX& normalMatrix)
	{
		DirectX::XMFLOAT4X4 source;
		DirectX::XMStoreFloat4x4(&source, world);
		normalMatrix = DirectX::XMMatrixIdentity();
		for (const auto& row : source.m)
		{
			for (const float value : row)
			{
				if (!std::isfinite(value))
				{
					return false;
				}
			}
		}

		const double a = source._11, b = source._12, c = source._13;
		const double d = source._21, e = source._22, f = source._23;
		const double g = source._31, h = source._32, i = source._33;
		const double cofactors[3][3]
		{
			{ e * i - f * h, f * g - d * i, d * h - e * g },
			{ c * h - b * i, a * i - c * g, b * g - a * h },
			{ b * f - c * e, c * d - a * f, a * e - b * d }
		};
		const double determinant = a * cofactors[0][0] + b * cofactors[0][1] + c * cofactors[0][2];
		if (determinant == 0.0 || !std::isfinite(determinant))
		{
			return false;
		}

		DirectX::XMFLOAT4X4 result{};
		result._44 = 1.0f;
		for (int row = 0; row < 3; ++row)
		{
			for (int column = 0; column < 3; ++column)
			{
				const double value = cofactors[row][column] / determinant;
				if (!std::isfinite(value) || std::abs(value) > (std::numeric_limits<float>::max)())
				{
					return false;
				}
				result.m[row][column] = static_cast<float>(value);
			}
		}
		normalMatrix = DirectX::XMLoadFloat4x4(&result);
		return true;
	}
}
