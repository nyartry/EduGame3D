#include "Framework/Rendering/Pipelines/BasicColorPipeline.h"

#include "Framework/Rendering/Core/Dx12PipelineHelper.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
	constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	constexpr const wchar_t* ShaderFileName = L"src\\Framework\\Rendering\\Shaders\\BasicColor.hlsl";
}

void BasicColorPipeline::Initialize(ID3D12Device* device)
{
	CreateRootSignature(device);
	CreatePipelineState(device);
	CreateConstantBuffers(device);
}

D3D12_GPU_VIRTUAL_ADDRESS BasicColorPipeline::UpdateWorldViewProjection(const XMMATRIX& worldViewProjection)
{
	XMStoreFloat4x4(&m_constantBufferData.worldViewProjection, XMMatrixTranspose(worldViewProjection));
	return m_sceneConstantBuffer.Write(m_constantBufferData);
}

void BasicColorPipeline::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
}

ID3D12PipelineState* BasicColorPipeline::GetPipelineState() const
{
	return m_pipelineState.Get();
}

void BasicColorPipeline::CreateRootSignature(ID3D12Device* device)
{
	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor.ShaderRegister = 0;
	rootParameter.Descriptor.RegisterSpace = 0;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParameter;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void BasicColorPipeline::CreatePipelineState(ID3D12Device* device)
{
	ComPtr<ID3DBlob> vertexShader = Dx12PipelineHelper::CompileShader(ShaderFileName, "VSMain", "vs_5_0");
	ComPtr<ID3DBlob> pixelShader = Dx12PipelineHelper::CompileShader(ShaderFileName, "PSMain", "ps_5_0");
	
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

void BasicColorPipeline::CreateConstantBuffers(ID3D12Device* device)
{
	m_sceneConstantBuffer.Initialize(device);
}
