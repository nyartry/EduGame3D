#pragma once

#include "Framework/Scene/Cameras/CameraBase.h"

#include <cstddef>
#include <vector>

class SplineCamera final : public CameraBase
{
public:
	void Update(float deltaTime, const Input& input) override;
	void SetTarget(const DirectX::XMFLOAT3& position, float rotationY) override;

private:
	DirectX::XMFLOAT3 ComputePosition(std::size_t startIndex, float amount) const;
	void BuildDefaultPath(const DirectX::XMFLOAT3& anchor);

	std::vector<DirectX::XMFLOAT3> m_controlPoints;
	std::size_t m_index{ 1 };
	float m_amount{};
	float m_speed{ 0.35f };
	bool m_paused{};
	bool m_hasPath{};
};
