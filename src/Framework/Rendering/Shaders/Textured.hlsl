struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

cbuffer SceneConstants : register(b0)
{
	matrix worldViewProjection;
	matrix world;
	matrix normalWorld;
};

Texture2D baseColorTexture : register(t0);
Texture2D opacityTexture : register(t1);
Texture2D normalTexture : register(t2);
SamplerState baseColorSampler : register(s0);

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.0f), worldViewProjection);
	output.normal = normalize(mul(float4(input.normal, 0.0f), normalWorld).xyz);
	output.tangent = normalize(mul(float4(input.tangent, 0.0f), world).xyz);
	output.uv = input.uv;
	output.color = input.color;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 baseColor = baseColorTexture.Sample(baseColorSampler, input.uv);
	baseColor *= input.color;
	float opacity = opacityTexture.Sample(baseColorSampler, input.uv).r * baseColor.a;
	clip(opacity - 0.35f);

	float3 vertexNormal = normalize(input.normal);
	float3 tangent = normalize(input.tangent - vertexNormal * dot(input.tangent, vertexNormal));
	float3 bitangent = normalize(cross(vertexNormal, tangent));
	float3 normalSample = normalTexture.Sample(baseColorSampler, input.uv).xyz * 2.0f - 1.0f;
	float3 normal = normalize(
		tangent * normalSample.x +
		bitangent * normalSample.y +
		vertexNormal * normalSample.z);

	float3 lightDirection = normalize(float3(-0.35f, 0.75f, -0.55f));
	float diffuse = saturate(dot(normal, lightDirection));
	float3 litColor = baseColor.rgb * (diffuse * 0.75f + 0.35f);
	float3 gammaCorrectedColor = pow(saturate(litColor), 1.0f / 2.2f);
	return float4(gammaCorrectedColor, opacity);
}
