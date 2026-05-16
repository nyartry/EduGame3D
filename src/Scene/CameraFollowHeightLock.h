#pragma once

#include <DirectXMath.h>

class CameraFollowHeightLock
{
public:
	DirectX::XMFLOAT3 ResolveFollowPosition(
		const DirectX::XMFLOAT3& targetPosition,
		bool shouldUpdateHeight);
	void Reset();

private:
	float m_height{};
	bool m_hasHeight{};
};
