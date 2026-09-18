#include "Game/Gameplay/Cube.h"

namespace
{
	constexpr float HalfSize = 0.5f;

	constexpr float Red[] = { 0.90f, 0.18f, 0.16f, 1.0f };
	constexpr float Green[] = { 0.18f, 0.75f, 0.25f, 1.0f };
	constexpr float Blue[] = { 0.20f, 0.38f, 0.95f, 1.0f };
	constexpr float Yellow[] = { 0.95f, 0.82f, 0.20f, 1.0f };
	constexpr float Cyan[] = { 0.20f, 0.85f, 0.90f, 1.0f };
	constexpr float Magenta[] = { 0.85f, 0.25f, 0.90f, 1.0f };

	Vertex MakeVertex(float x, float y, float z, const float color[4])
	{
		return Vertex{ { x, y, z }, { color[0], color[1], color[2], color[3] } };
	}

	void AddFace(
		std::vector<Vertex>& vertices,
		const Vertex& v0,
		const Vertex& v1,
		const Vertex& v2,
		const Vertex& v3)
	{
		vertices.push_back(v0);
		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v0);
		vertices.push_back(v2);
		vertices.push_back(v3);
	}
}

std::vector<Vertex> Cube::BuildVertices() const
{
	std::vector<Vertex> vertices;
	vertices.reserve(36);

	const Vertex leftBottomBack = MakeVertex(-HalfSize, -HalfSize, -HalfSize, Red);
	const Vertex rightBottomBack = MakeVertex(HalfSize, -HalfSize, -HalfSize, Red);
	const Vertex rightTopBack = MakeVertex(HalfSize, HalfSize, -HalfSize, Red);
	const Vertex leftTopBack = MakeVertex(-HalfSize, HalfSize, -HalfSize, Red);

	const Vertex leftBottomFront = MakeVertex(-HalfSize, -HalfSize, HalfSize, Green);
	const Vertex rightBottomFront = MakeVertex(HalfSize, -HalfSize, HalfSize, Green);
	const Vertex rightTopFront = MakeVertex(HalfSize, HalfSize, HalfSize, Green);
	const Vertex leftTopFront = MakeVertex(-HalfSize, HalfSize, HalfSize, Green);

	AddFace(vertices, leftBottomFront, rightBottomFront, rightTopFront, leftTopFront);
	AddFace(vertices, rightBottomBack, leftBottomBack, leftTopBack, rightTopBack);
	AddFace(
		vertices,
		MakeVertex(-HalfSize, -HalfSize, -HalfSize, Blue),
		MakeVertex(-HalfSize, -HalfSize, HalfSize, Blue),
		MakeVertex(-HalfSize, HalfSize, HalfSize, Blue),
		MakeVertex(-HalfSize, HalfSize, -HalfSize, Blue));
	AddFace(
		vertices,
		MakeVertex(HalfSize, -HalfSize, HalfSize, Yellow),
		MakeVertex(HalfSize, -HalfSize, -HalfSize, Yellow),
		MakeVertex(HalfSize, HalfSize, -HalfSize, Yellow),
		MakeVertex(HalfSize, HalfSize, HalfSize, Yellow));
	AddFace(
		vertices,
		MakeVertex(-HalfSize, HalfSize, HalfSize, Cyan),
		MakeVertex(HalfSize, HalfSize, HalfSize, Cyan),
		MakeVertex(HalfSize, HalfSize, -HalfSize, Cyan),
		MakeVertex(-HalfSize, HalfSize, -HalfSize, Cyan));
	AddFace(
		vertices,
		MakeVertex(-HalfSize, -HalfSize, -HalfSize, Magenta),
		MakeVertex(HalfSize, -HalfSize, -HalfSize, Magenta),
		MakeVertex(HalfSize, -HalfSize, HalfSize, Magenta),
		MakeVertex(-HalfSize, -HalfSize, HalfSize, Magenta));

	return vertices;
}
