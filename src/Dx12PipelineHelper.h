#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <d3dcompiler.h>

namespace Dx12PipelineHelper
{
	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const wchar_t* fileName,
		const char* entryPoint,
		const char* target);

	D3D12_RASTERIZER_DESC CreateDefaultRasterizerDesc();
	D3D12_BLEND_DESC CreateDefaultBlendDesc();
	D3D12_DEPTH_STENCIL_DESC CreateDefaultDepthStencilDesc();
}
