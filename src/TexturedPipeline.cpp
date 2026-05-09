#include "TexturedPipeline.h"

#include "Common.h"
#include "Dx12BufferHelper.h"

#include <d3dcompiler.h>
#include <cstring>
#include <stdexcept>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
	constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	constexpr const wchar_t* ShaderFileNames[] =
	{
		L"src\\Textured.hlsl",
		L"..\\..\\src\\Textured.hlsl",
	};

	void ThrowIfShaderFailed(HRESULT hr, ID3DBlob* error)
	{
		if (FAILED(hr))
		{
			if (error != nullptr)
			{
				const char* message = static_cast<const char*>(error->GetBufferPointer());
				throw std::runtime_error(message);
			}

			ThrowIfFailed(hr);
		}
	}

	void CompileShader(const char* entryPoint, const char* target, UINT compileFlags, ComPtr<ID3DBlob>& shader)
	{
		ComPtr<ID3DBlob> lastError;

		for (const wchar_t* fileName : ShaderFileNames)
		{
			lastError.Reset();
			const HRESULT hr = D3DCompileFromFile(fileName, nullptr, nullptr, entryPoint, target, compileFlags, 0, &shader, &lastError);
			if (SUCCEEDED(hr))
			{
				return;
			}
		}

		ThrowIfShaderFailed(E_FAIL, lastError.Get());
	}
}

void TexturedPipeline::Initialize(ID3D12Device* device)
{
	CreateRootSignature(device);
	CreatePipelineState(device);
	CreateConstantBuffer(device);
}

void TexturedPipeline::UpdateWorldViewProjection(const XMMATRIX& worldViewProjection)
{
	XMStoreFloat4x4(&m_constantBufferData.worldViewProjection, XMMatrixTranspose(worldViewProjection));
	memcpy(m_constantBufferMappedData, &m_constantBufferData, sizeof(m_constantBufferData));
}

void TexturedPipeline::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
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
	srvRange.NumDescriptors = 1;
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
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	CompileShader("VSMain", "vs_5_0", compileFlags, vertexShader);
	CompileShader("PSMain", "ps_5_0", compileFlags, pixelShader);

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.DepthClipEnable = TRUE;

	D3D12_BLEND_DESC blendDesc{};
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (auto& target : blendDesc.RenderTarget)
	{
		target = defaultRenderTargetBlendDesc;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
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
	const UINT constantBufferSize = Dx12BufferHelper::AlignConstantBufferSize(sizeof(SceneConstants));
	m_constantBuffer = Dx12BufferHelper::CreateUploadBuffer(device, constantBufferSize);

	D3D12_RANGE readRange{};
	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferMappedData)));
}
