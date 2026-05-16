#include "Rendering/SkinnedTexturedPipeline.h"

#include "Common/Common.h"
#include "Rendering/Dx12PipelineHelper.h"

#include <algorithm>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
	constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	constexpr const wchar_t* ShaderFileName = L"src\\Rendering\\Shaders\\SkinnedTextured.hlsl";
}

void SkinnedTexturedPipeline::Initialize(ID3D12Device* device)
{
	CreateRootSignature(device);
	CreatePipelineState(device);
	CreateConstantBuffers(device);
}

SkinnedTexturedPipeline::ConstantBufferViews SkinnedTexturedPipeline::UpdateConstants(
	const XMMATRIX& worldViewProjection,
	const std::vector<XMFLOAT4X4>& boneMatrices,
	float modelCenterX,
	float modelMinY,
	float modelCenterZ,
	float modelScale)
{
	XMStoreFloat4x4(&m_sceneConstants.worldViewProjection, XMMatrixTranspose(worldViewProjection));
	m_sceneConstants.modelFit = XMFLOAT4{ modelCenterX, modelMinY, modelCenterZ, modelScale };

	const size_t boneCount = std::min(boneMatrices.size(), MaxBones);
	m_sceneConstants.boneCount = static_cast<UINT>(boneCount);
	for (size_t boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		XMStoreFloat4x4(
			&m_boneConstants.boneMatrices[boneIndex],
			XMMatrixTranspose(XMLoadFloat4x4(&boneMatrices[boneIndex])));
	}

	return ConstantBufferViews
	{
		m_sceneConstantBuffer.Write(m_sceneConstants),
		m_boneConstantBuffer.Write(m_boneConstants)
	};
}

void SkinnedTexturedPipeline::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
}

ID3D12PipelineState* SkinnedTexturedPipeline::GetPipelineState() const
{
	return m_pipelineState.Get();
}

void SkinnedTexturedPipeline::CreateRootSignature(ID3D12Device* device)
{
	D3D12_ROOT_PARAMETER rootParameters[3]{};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 1;
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_DESCRIPTOR_RANGE srvRange{};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 3;
	srvRange.BaseShaderRegister = 0;
	srvRange.RegisterSpace = 0;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &srvRange;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void SkinnedTexturedPipeline::CreatePipelineState(ID3D12Device* device)
{
	ComPtr<ID3DBlob> vertexShader = Dx12PipelineHelper::CompileShader(ShaderFileName, "VSMain", "vs_5_0");
	ComPtr<ID3DBlob> pixelShader = Dx12PipelineHelper::CompileShader(ShaderFileName, "PSMain", "ps_5_0");

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	psoDesc.RasterizerState = Dx12PipelineHelper::CreateDefaultRasterizerDesc();
	psoDesc.BlendState = Dx12PipelineHelper::CreateDefaultBlendDesc();
	psoDesc.DepthStencilState = Dx12PipelineHelper::CreateDefaultDepthStencilDesc();
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DepthStencilFormat;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void SkinnedTexturedPipeline::CreateConstantBuffers(ID3D12Device* device)
{
	m_sceneConstantBuffer.Initialize(device);
	m_boneConstantBuffer.Initialize(device);
}
