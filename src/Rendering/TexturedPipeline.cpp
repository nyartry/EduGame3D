#include "Rendering/TexturedPipeline.h"

#include "Common/Common.h"
#include "Rendering/Dx12BufferHelper.h"
#include "Rendering/Dx12PipelineHelper.h"

#include <cstring>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
	constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	constexpr const wchar_t* ShaderFileName = L"src\\Rendering\\Shaders\\Textured.hlsl";
	constexpr UINT MaxDrawConstants = 4096;
}

void TexturedPipeline::Initialize(ID3D12Device* device)
{
	CreateRootSignature(device);
	CreatePipelineState(device);
	CreateConstantBuffer(device);
}

D3D12_GPU_VIRTUAL_ADDRESS TexturedPipeline::UpdateWorldViewProjection(const XMMATRIX& worldViewProjection)
{
	XMStoreFloat4x4(&m_constantBufferData.worldViewProjection, XMMatrixTranspose(worldViewProjection));

	const UINT constantBufferIndex = m_nextConstantBufferIndex;
	m_nextConstantBufferIndex = (m_nextConstantBufferIndex + 1) % MaxDrawConstants;

	UINT8* destination = m_constantBufferMappedData + static_cast<SIZE_T>(constantBufferIndex) * m_constantBufferSize;
	memcpy(destination, &m_constantBufferData, sizeof(m_constantBufferData));
	return m_constantBuffer->GetGPUVirtualAddress() + static_cast<UINT64>(constantBufferIndex) * m_constantBufferSize;
}

void TexturedPipeline::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
}

ID3D12PipelineState* TexturedPipeline::GetPipelineState() const
{
	return m_pipelineState.Get();
}

void TexturedPipeline::CreateRootSignature(ID3D12Device* device)
{
	D3D12_ROOT_PARAMETER rootParameters[2]{};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_DESCRIPTOR_RANGE srvRange{};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 3;
	srvRange.BaseShaderRegister = 0;
	srvRange.RegisterSpace = 0;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &srvRange;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.MinLOD = 0.0f;
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

void TexturedPipeline::CreatePipelineState(ID3D12Device* device)
{
	ComPtr<ID3DBlob> vertexShader = Dx12PipelineHelper::CompileShader(ShaderFileName, "VSMain", "vs_5_0");
	ComPtr<ID3DBlob> pixelShader = Dx12PipelineHelper::CompileShader(ShaderFileName, "PSMain", "ps_5_0");

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

void TexturedPipeline::CreateConstantBuffer(ID3D12Device* device)
{
	m_constantBufferSize = Dx12BufferHelper::AlignConstantBufferSize(sizeof(SceneConstants));
	m_constantBuffer = Dx12BufferHelper::CreateUploadBuffer(device, static_cast<UINT64>(m_constantBufferSize) * MaxDrawConstants);

	D3D12_RANGE readRange{};
	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferMappedData)));
}
