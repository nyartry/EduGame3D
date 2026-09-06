#define MAX_BONES 512

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
	int4 boneIndices : BLENDINDICES;
	float4 boneWeights : BLENDWEIGHT;
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
	float4 modelFit; // centerX, minY, centerZ, scale
	uint boneCount;
	float3 sceneConstantsPadding;
};

cbuffer BoneConstants : register(b1)
{
	matrix boneMatrices[MAX_BONES];
};

Texture2D baseColorTexture : register(t0);
Texture2D opacityTexture : register(t1);
Texture2D normalTexture : register(t2);
SamplerState baseColorSampler : register(s0);

void AccumulateBone(
	inout float4 position,
	inout float3 normal,
	inout float3 tangent,
	inout float totalWeight,
	float3 sourcePosition,
	float3 sourceNormal,
	float3 sourceTangent,
	int boneIndex,
	float weight)
{
	if (boneIndex < 0 || (uint)boneIndex >= boneCount || boneIndex >= MAX_BONES || weight == 0.0f)
	{
		return;
	}

	matrix boneMatrix = boneMatrices[boneIndex];
	position += mul(float4(sourcePosition, 1.0f), boneMatrix) * weight;
	normal += mul(float4(sourceNormal, 0.0f), boneMatrix).xyz * weight;
	tangent += mul(float4(sourceTangent, 0.0f), boneMatrix).xyz * weight;
	totalWeight += weight;
}

PSInput VSMain(VSInput input)
{
	float totalWeight = 0.0f;
	float4 skinnedPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);
	float3 skinnedTangent = float3(0.0f, 0.0f, 0.0f);

	AccumulateBone(skinnedPosition, skinnedNormal, skinnedTangent, totalWeight, input.position, input.normal, input.tangent, input.boneIndices.x, input.boneWeights.x);
	AccumulateBone(skinnedPosition, skinnedNormal, skinnedTangent, totalWeight, input.position, input.normal, input.tangent, input.boneIndices.y, input.boneWeights.y);
	AccumulateBone(skinnedPosition, skinnedNormal, skinnedTangent, totalWeight, input.position, input.normal, input.tangent, input.boneIndices.z, input.boneWeights.z);
	AccumulateBone(skinnedPosition, skinnedNormal, skinnedTangent, totalWeight, input.position, input.normal, input.tangent, input.boneIndices.w, input.boneWeights.w);

	if (totalWeight == 0.0f)
	{
		skinnedPosition = float4(input.position, 1.0f);
		skinnedNormal = input.normal;
		skinnedTangent = input.tangent;
	}
	else if (totalWeight != 1.0f)
	{
		skinnedPosition /= totalWeight;
		skinnedNormal /= totalWeight;
		skinnedTangent /= totalWeight;
	}

	float3 fittedPosition = float3(
		(skinnedPosition.x - modelFit.x) * modelFit.w,
		(skinnedPosition.y - modelFit.y) * modelFit.w,
		(skinnedPosition.z - modelFit.z) * modelFit.w);

	PSInput output;
	output.position = mul(float4(fittedPosition, 1.0f), worldViewProjection);
	output.normal = normalize(mul(float4(skinnedNormal, 0.0f), normalWorld).xyz);
	output.tangent = normalize(mul(float4(skinnedTangent, 0.0f), world).xyz);
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
