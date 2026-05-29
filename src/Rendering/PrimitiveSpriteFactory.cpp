#include "Rendering/PrimitiveSpriteFactory.h"

#include <cstdint>
#include <vector>

namespace
{
	constexpr UINT BytesPerPixel = 4;

	std::vector<UINT8> CreateDiamondPixels(UINT width, UINT height)
	{
		std::vector<UINT8> pixels(static_cast<size_t>(width) * height * BytesPerPixel, 0);

		for (UINT y = 0; y < height; ++y)
		{
			for (UINT x = 0; x < width; ++x)
			{
				const size_t offset = (static_cast<size_t>(y) * width + x) * BytesPerPixel;
				const bool border = x == 0 || y == 0 || x == width - 1 || y == height - 1;
				const bool diagonal = x == y || x + y == width - 1;
				const bool innerDiamond = x > 6 && x < 17 && y > 6 && y < 17 && (x + y) % 3 != 0;

				if (border)
				{
					pixels[offset + 0] = 232;
					pixels[offset + 1] = 248;
					pixels[offset + 2] = 255;
					pixels[offset + 3] = 230;
				}
				else if (diagonal)
				{
					pixels[offset + 0] = 255;
					pixels[offset + 1] = 204;
					pixels[offset + 2] = 76;
					pixels[offset + 3] = 235;
				}
				else if (innerDiamond)
				{
					pixels[offset + 0] = 68;
					pixels[offset + 1] = 220;
					pixels[offset + 2] = 180;
					pixels[offset + 3] = 245;
				}
				else
				{
					pixels[offset + 0] = 28;
					pixels[offset + 1] = 54;
					pixels[offset + 2] = 82;
					pixels[offset + 3] = 190;
				}
			}
		}

		return pixels;
	}
}

SpriteImage PrimitiveSpriteFactory::CreateDiamondSprite(
	ID3D12Device* device,
	float x,
	float y,
	float width,
	float height)
{
	SpriteImage image;
	image.Initialize(
		device,
		CreateDiamondPixels(DiamondTextureWidth, DiamondTextureHeight),
		DiamondTextureWidth,
		DiamondTextureHeight,
		x,
		y,
		width,
		height);
	return image;
}
