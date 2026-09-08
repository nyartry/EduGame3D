#include "TestSupport.h"
#include "Framework/Core/Math/MathUtils.h"
#include "Framework/Core/Math/Transform.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace DirectX;

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	void Near(float actual, float expected, const char* message, float tolerance = 0.0001f)
	{
		if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
		{
			throw std::runtime_error(std::string(message) + ": expected " +
				std::to_string(expected) + ", got " + std::to_string(actual));
		}
	}

	void Near(const XMFLOAT3& actual, const XMFLOAT3& expected, const char* message)
	{
		Near(actual.x, expected.x, message);
		Near(actual.y, expected.y, message);
		Near(actual.z, expected.z, message);
	}

	void Near(const XMMATRIX& actual, const XMMATRIX& expected, const char* message)
	{
		XMFLOAT4X4 left, right;
		XMStoreFloat4x4(&left, actual);
		XMStoreFloat4x4(&right, expected);
		for (int row = 0; row < 4; ++row)
			for (int column = 0; column < 4; ++column)
				Near(left.m[row][column], right.m[row][column], message);
	}

	float Dot(const XMFLOAT3& left, const XMFLOAT3& right)
	{
		return left.x * right.x + left.y * right.y + left.z * right.z;
	}

	void IdentityAndSrtOrder()
	{
		Transform transform;
		const XMFLOAT3 point{ 1.0f, 2.0f, 3.0f };
		Near(transform.ToMatrix(), XMMatrixIdentity(), "Default transform is identity");
		Near(transform.TransformPoint(point), point, "Identity preserves points");
		transform.scale = { 2.0f, 3.0f, 4.0f };
		transform.rotationRadians.y = XM_PIDIV2;
		transform.position = { 10.0f, 20.0f, 30.0f };
		Near(transform.TransformPoint(point), { 22.0f, 26.0f, 28.0f }, "Points scale, rotate, then translate");
		Near(transform.TransformDirection(point), { 12.0f, 6.0f, -2.0f }, "Directions include scale and rotation but no translation");
		transform.position = { -100.0f, 200.0f, -300.0f };
		Near(transform.TransformDirection(point), { 12.0f, 6.0f, -2.0f }, "Changing translation preserves direction");
	}

	void LegacyYawCompatibility()
	{
		Transform transform;
		transform.position = { 2.0f, -3.0f, 5.0f };
		for (float yaw : { 0.0f, 0.7f, -1.2f, XM_PI })
		{
			transform.rotationRadians.y = yaw;
			Near(transform.ToMatrix(), XMMatrixRotationY(yaw) * XMMatrixTranslation(2.0f, -3.0f, 5.0f),
				"Transform preserves legacy actor yaw and position");
		}
	}

	void NonuniformScaleNormals()
	{
		Transform transform;
		transform.position = { 4.0f, -8.0f, 12.0f };
		transform.rotationRadians = { 0.3f, -0.7f, 1.1f };
		transform.scale = { 2.0f, 3.0f, 0.5f };
		const XMFLOAT3 sourceNormal{ 1.0f, -1.0f, 1.0f };
		const auto tangentA = transform.TransformDirection({ 1.0f, 1.0f, 0.0f });
		const auto tangentB = transform.TransformDirection({ 0.0f, 1.0f, 1.0f });
		XMFLOAT3 normal;
		Require(transform.TryTransformNormal(sourceNormal, normal), "Nonuniform normal transform succeeds");
		Near(Dot(normal, normal), 1.0f, "Transformed normal is unit length");
		Near(Dot(normal, tangentA), 0.0f, "Normal remains perpendicular to first tangent");
		Near(Dot(normal, tangentB), 0.0f, "Normal remains perpendicular to second tangent");
		transform.position = {};
		XMFLOAT3 untranslated;
		Require(transform.TryTransformNormal(sourceNormal, untranslated), "Normal without translation succeeds");
		Near(untranslated, normal, "Translation has no effect on normals");
		XMFLOAT3 inPlace = sourceNormal;
		Require(transform.TryTransformNormal(inPlace, inPlace), "In-place normal transform succeeds");
		Near(inPlace, normal, "In-place normal transform preserves input before writing output");
	}

	void InvalidNormalsAndMatrices()
	{
		const float infinity = std::numeric_limits<float>::infinity();
		const float nan = std::numeric_limits<float>::quiet_NaN();
		for (const XMFLOAT3 input : { XMFLOAT3{}, XMFLOAT3{ nan, 1.0f, 0.0f }, XMFLOAT3{ 1.0f, infinity, 0.0f } })
		{
			XMFLOAT3 output{ 9.0f, 9.0f, 9.0f };
			Require(!Transform{}.TryTransformNormal(input, output), "Invalid normal is rejected");
			Near(output, {}, "Invalid normal clears output");
		}
		for (const float scaleX : { 0.0f, infinity, nan })
		{
			Transform transform;
			transform.scale.x = scaleX;
			XMFLOAT3 output{ 9.0f, 9.0f, 9.0f };
			Require(!transform.TryTransformNormal({ 0.0f, 1.0f, 0.0f }, output), "Singular or nonfinite transform is rejected");
			Near(output, {}, "Invalid transform clears normal output");
			XMMATRIX matrix = XMMatrixScaling(9.0f, 9.0f, 9.0f);
			Require(!MathUtils::TryCreateNormalMatrix(transform.ToMatrix(), matrix), "Invalid normal matrix is rejected");
			Near(matrix, XMMatrixIdentity(), "Invalid normal matrix resets output to identity");
		}
		XMMATRIX matrix = XMMatrixTranslation(infinity, 0.0f, 0.0f);
		Require(!MathUtils::TryCreateNormalMatrix(matrix, matrix), "Nonfinite translation is rejected with aliased matrix output");
		Near(matrix, XMMatrixIdentity(), "Failed in-place matrix creation defines output");
	}

	void SafeNormalization()
	{
		const float nan = std::numeric_limits<float>::quiet_NaN();
		for (const XMFLOAT3 input : { XMFLOAT3{}, XMFLOAT3{ nan, 1.0f, 0.0f }, XMFLOAT3{ 0.0f, std::numeric_limits<float>::infinity(), 0.0f } })
		{
			XMFLOAT3 output{ 9.0f, 9.0f, 9.0f };
			Require(!MathUtils::TryNormalize(input, output), "Zero or nonfinite normalization fails");
			Near(output, {}, "Failed normalization clears output");
		}
		for (const float magnitude : { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::min)() })
		{
			XMFLOAT3 output;
			Require(MathUtils::TryNormalize({ magnitude, magnitude, 0.0f }, output, 0.0f), "Extreme finite vector normalizes without overflow or underflow");
			Near(output, { 0.70710678f, 0.70710678f, 0.0f }, "Extreme finite vector keeps its direction");
		}
		XMFLOAT3 inPlace{ 3.0f, 0.0f, 4.0f };
		Require(MathUtils::TryNormalize(inPlace, inPlace), "In-place normalization succeeds");
		Near(inPlace, { 0.6f, 0.0f, 0.8f }, "In-place normalization retains original components");
		inPlace = { 3.0f, nan, 4.0f };
		Require(MathUtils::TryNormalizeXZ(inPlace, inPlace), "XZ normalization ignores vertical component and supports aliasing");
		Near(inPlace, { 0.6f, 0.0f, 0.8f }, "XZ result is horizontal and normalized");
		Near(MathUtils::NormalizeOrDefault({}, { 1.0f, 0.0f, 0.0f }), { 1.0f, 0.0f, 0.0f }, "Invalid direction uses caller fallback");
	}

	void ShortestAnglePath()
	{
		const float before = XMConvertToRadians(170.0f);
		const float after = XMConvertToRadians(-170.0f);
		Near(MathUtils::LerpAngle(before, after, 0.25f), XMConvertToRadians(175.0f), "Angle crosses positive pi by shortest path");
		Near(MathUtils::LerpAngle(after, before, 0.25f), XMConvertToRadians(-175.0f), "Angle crosses negative pi by shortest path");
		Near(std::abs(MathUtils::LerpAngle(before, after, 0.5f)), XM_PI, "Midpoint reaches pi boundary instead of zero");
		Near(MathUtils::NormalizeAngle(XM_PI), -XM_PI, "Positive pi maps to half-open interval");
		Near(MathUtils::NormalizeAngle(-XM_PI), -XM_PI, "Negative pi remains in canonical interval");
		Near(MathUtils::LerpAngle(before, after, 0.0f), before, "Zero weight preserves starting angle");
		Near(MathUtils::LerpAngle(before, after, 1.0f), after, "Full weight reaches ending angle");
		Near(MathUtils::NormalizeAngle(std::numeric_limits<float>::infinity()), 0.0f, "Nonfinite angle has defined fallback");
	}

	void StableSmoothing()
	{
		const float tiny = MathUtils::SmoothAmount(2.0f, 1.0e-10f);
		Require(tiny > 0.0f, "Tiny positive time step must retain positive smoothing");
		Near(tiny / 2.0e-10f, 1.0f, "Tiny smoothing agrees with linear limit");
		for (const float invalid : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
		{
			Near(MathUtils::SmoothAmount(2.0f, invalid), 0.0f, "Invalid time step leaves current value unchanged");
			Near(MathUtils::SmoothAmount(invalid, 0.01f), 0.0f, "Invalid sharpness leaves current value unchanged");
		}
		const float whole = MathUtils::SmoothAmount(5.0f, 0.1f);
		const float half = MathUtils::SmoothAmount(5.0f, 0.05f);
		Near(1.0f - whole, (1.0f - half) * (1.0f - half), "Smoothing composes across time steps");
		Near(MathUtils::SmoothAmount((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)()),
			1.0f, "Very large finite parameters converge without overflow");
	}
}

#define MATHCORETESTS_CASES(TEST) \
	TEST(IdentityAndSrtOrder, "identity, SRT order, points, and directions", Cpu) \
	TEST(LegacyYawCompatibility, "legacy actor yaw compatibility", Cpu) \
	TEST(NonuniformScaleNormals, "normals under rotated nonuniform scale", Cpu) \
	TEST(InvalidNormalsAndMatrices, "invalid normals and normal matrices", Cpu) \
	TEST(SafeNormalization, "safe normalization and in-place output", Cpu) \
	TEST(ShortestAnglePath, "shortest angle path across pi", Cpu) \
	TEST(StableSmoothing, "stable smoothing", Cpu)

GAME_TEST_SUITE(MathCoreTests, MATHCORETESTS_CASES)
