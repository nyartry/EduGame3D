#include "SkinningTestCases.h"
#include "TestSupport.h"

#include <stdexcept>

void RunSkinningGpuTests();

namespace
{
	void Near(const DirectX::XMFLOAT3& value, const DirectX::XMFLOAT3& expected, const char* message)
	{
		if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z) ||
			std::abs(value.x - expected.x) > 1.0e-5f || std::abs(value.y - expected.y) > 1.0e-5f ||
			std::abs(value.z - expected.z) > 1.0e-5f) throw std::runtime_error(message);
	}

	void RunCpuTests()
	{
		using namespace DirectX;
		const auto cases = CreateSkinningTestCases();
		std::vector<XMFLOAT4X4> normalBones;
		std::vector<TexturedVertex> results;
		for (const auto& test : cases)
		{
			BoneSkinning::BuildNormalPalette(test.bones, normalBones);
			results.push_back(BoneSkinning::DeformVertex(test.vertex, test.bones, normalBones));
		}
		Near(results[0].normal, MathUtils::NormalizeOrDefault({ 0.5f, 1.0f, 0.0f }), "Normals need inverse scale.");
		Near(results[0].position, { 2.0f, 2.0f, 1.5f }, "Position retains affine scaling.");
		Near(results[0].tangent, MathUtils::NormalizeOrDefault({ 2.0f, -1.0f, 0.0f }), "Tangents retain forward scale.");
		for (std::size_t index = 0; index < 2; ++index)
		{
			const auto& result = results[index];
			const float dot = result.normal.x * result.tangent.x + result.normal.y * result.tangent.y + result.normal.z * result.tangent.z;
			if (std::abs(dot) > 1.0e-5f) throw std::runtime_error("Single-bone normals must remain perpendicular to transformed tangents.");
		}
		Near(results[2].normal, cases[2].vertex.vertex.normal, "Singular normal matrices use the identity fallback.");
		Near(results[3].normal, MathUtils::NormalizeOrDefault({ 0.8125f, 0.625f, 0.0f }), "Normalize only after weighted normal blending.");
		Near(results[4].normal, cases[4].vertex.vertex.normal, "A cancelling blend keeps a finite source direction.");
		Near(results[5].position, cases[5].vertex.vertex.position, "Invalid influences leave source positions intact.");
		Near(results[6].position, cases[6].vertex.vertex.position, "CPU and GPU share the 512-bone bound.");
		Near(results[7].normal, { 1.0f, 0.0f, 0.0f }, "Very large finite inverse scales remain normalizable.");
		BoneSkinning::BuildNormalPalette(cases[6].bones, normalBones);
		if (normalBones.size() != BoneSkinning::MaxBones) throw std::runtime_error("Normal palette must obey the shader limit.");
	}
}

#define SKINNING_TEST_CASES(TEST) \
	TEST(RunCpuTests, "skinning CPU regression", Cpu) \
	TEST(RunSkinningGpuTests, "skinning WARP GPU regression", Gpu)

GAME_TEST_SUITE(SkinningTests, SKINNING_TEST_CASES)
