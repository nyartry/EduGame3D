#include "Rendering/Dx12PipelineHelper.h"

#include "Common/Common.h"

#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace
{
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
}

namespace Dx12PipelineHelper
{
	ComPtr<ID3DBlob> CompileShader(const wchar_t* fileName, const char* entryPoint, const char* target)
	{
		UINT compileFlags = 0;
	#if defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#endif

		ComPtr<ID3DBlob> shader;
		ComPtr<ID3DBlob> error;
		const HRESULT hr = D3DCompileFromFile(fileName, nullptr, nullptr, entryPoint, target, compileFlags, 0, &shader, &error);
		ThrowIfShaderFailed(hr, error.Get());
		return shader;
	}

	D3D12_RASTERIZER_DESC CreateDefaultRasterizerDesc()
	{
		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterizerDesc.DepthClipEnable = TRUE;
		return rasterizerDesc;
	}

	D3D12_BLEND_DESC CreateDefaultBlendDesc()
	{
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;

		const D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc =
		{
			FALSE, FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};

		for (D3D12_RENDER_TARGET_BLEND_DESC& target : blendDesc.RenderTarget)
		{
			target = renderTargetBlendDesc;
		}

		return blendDesc;
	}

	D3D12_BLEND_DESC CreateAlphaBlendDesc()
	{
		D3D12_BLEND_DESC blendDesc = CreateDefaultBlendDesc();
		D3D12_RENDER_TARGET_BLEND_DESC& target = blendDesc.RenderTarget[0];
		target.BlendEnable = TRUE;
		target.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		target.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		target.BlendOp = D3D12_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D12_BLEND_ONE;
		target.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		return blendDesc;
	}

	D3D12_DEPTH_STENCIL_DESC CreateDefaultDepthStencilDesc()
	{
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDesc.StencilEnable = FALSE;
		return depthStencilDesc;
	}

	D3D12_DEPTH_STENCIL_DESC CreateDepthDisabledDesc()
	{
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = FALSE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		depthStencilDesc.StencilEnable = FALSE;
		return depthStencilDesc;
	}
}
