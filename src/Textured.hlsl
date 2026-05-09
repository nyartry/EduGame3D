struct VSInput
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer SceneConstants : register(b0)
{
	matrix worldViewProjection;
};

Texture2D baseColorTexture : register(t0);
SamplerState baseColorSampler : register(s0);

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.0f), worldViewProjection);
	output.uv = input.uv;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return baseColorTexture.Sample(baseColorSampler, input.uv);
}
