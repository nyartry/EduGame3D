#include "Framework/Scene/Cameras/CameraFollowHeightLock.h"

using namespace DirectX;

XMFLOAT3 CameraFollowHeightLock::ResolveFollowPosition(
	const XMFLOAT3& targetPosition,
	bool shouldUpdateHeight)
{
	if (!m_hasHeight)
	{
		m_height = targetPosition.y;
		m_hasHeight = true;
	}

	if (shouldUpdateHeight)
	{
		m_height = targetPosition.y;
	}

	XMFLOAT3 followPosition = targetPosition;
	followPosition.y = m_height;
	return followPosition;
}

void CameraFollowHeightLock::Reset()
{
	m_height = 0.0f;
	m_hasHeight = false;
}
