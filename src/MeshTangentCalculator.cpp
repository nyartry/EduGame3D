#include "MeshTangentCalculator.h"

#include <DirectXMath.h>
#include <cmath>

namespace MeshTangentCalculator
{
	void ApplyToTriangle(TexturedVertex (&vertices)[3])
	{
		const TexturedVertex& v0 = vertices[0];
		const TexturedVertex& v1 = vertices[1];
		const TexturedVertex& v2 = vertices[2];

		const float edge1[] =
		{
			v1.position.x - v0.position.x,
			v1.position.y - v0.position.y,
			v1.position.z - v0.position.z,
		};
		const float edge2[] =
		{
			v2.position.x - v0.position.x,
			v2.position.y - v0.position.y,
			v2.position.z - v0.position.z,
		};

		const float deltaUv1[] = { v1.uv.x - v0.uv.x, v1.uv.y - v0.uv.y };
		const float deltaUv2[] = { v2.uv.x - v0.uv.x, v2.uv.y - v0.uv.y };
		const float denominator = deltaUv1[0] * deltaUv2[1] - deltaUv2[0] * deltaUv1[1];
		if (std::abs(denominator) < 0.000001f)
		{
			return;
		}

		const float scale = 1.0f / denominator;
		const float tangent[] =
		{
			(edge1[0] * deltaUv2[1] - edge2[0] * deltaUv1[1]) * scale,
			(edge1[1] * deltaUv2[1] - edge2[1] * deltaUv1[1]) * scale,
			(edge1[2] * deltaUv2[1] - edge2[2] * deltaUv1[1]) * scale,
		};

		for (TexturedVertex& vertex : vertices)
		{
			vertex.tangent = DirectX::XMFLOAT3{ tangent[0], tangent[1], tangent[2] };
		}
	}
}
