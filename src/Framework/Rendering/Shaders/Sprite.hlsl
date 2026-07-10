struct VSInput
{
	float2 position : POSITION;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

cbuffer SpriteConstants : register(b0)
{
	float4 screenSize; // width, height, 1 / width, 1 / height
};

Texture2D spriteTexture : register(t0);
SamplerState spriteSampler : register(s0);

PSInput VSMain(VSInput input)
{
	const float2 clipPosition = float2(
		input.position.x * screenSize.z * 2.0f - 1.0f,
		1.0f - input.position.y * screenSize.w * 2.0f);

	PSInput output;
	output.position = float4(clipPosition, 0.0f, 1.0f);
	output.uv = input.uv;
	output.color = input.color;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return spriteTexture.Sample(spriteSampler, input.uv) * input.color;
}
