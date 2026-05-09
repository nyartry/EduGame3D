#include "MeshTangentCalculator.h"

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
			v1.position[0] - v0.position[0],
			v1.position[1] - v0.position[1],
			v1.position[2] - v0.position[2],
		};
		const float edge2[] =
		{
			v2.position[0] - v0.position[0],
			v2.position[1] - v0.position[1],
			v2.position[2] - v0.position[2],
		};

		const float deltaUv1[] = { v1.uv[0] - v0.uv[0], v1.uv[1] - v0.uv[1] };
		const float deltaUv2[] = { v2.uv[0] - v0.uv[0], v2.uv[1] - v0.uv[1] };
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
			vertex.tangent[0] = tangent[0];
			vertex.tangent[1] = tangent[1];
			vertex.tangent[2] = tangent[2];
		}
	}
}
