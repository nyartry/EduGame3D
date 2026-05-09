struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

cbuffer SceneConstants : register(b0)
{
	matrix worldViewProjection;
};

Texture2D baseColorTexture : register(t0);
Texture2D opacityTexture : register(t1);
SamplerState baseColorSampler : register(s0);

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.0f), worldViewProjection);
	output.normal = normalize(input.normal);
	output.uv = input.uv;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 baseColor = baseColorTexture.Sample(baseColorSampler, input.uv);
	float opacity = opacityTexture.Sample(baseColorSampler, input.uv).r * baseColor.a;
	clip(opacity - 0.35f);

	float3 normal = normalize(input.normal);
	float3 lightDirection = normalize(float3(-0.35f, 0.75f, -0.55f));
	float diffuse = saturate(dot(normal, lightDirection)) * 0.65f + 0.35f;
	return float4(baseColor.rgb * diffuse, opacity);
}
