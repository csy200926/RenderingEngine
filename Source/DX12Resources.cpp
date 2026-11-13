#include "DX12Renderer.h"
#include "Files.h"
#include <d3dcompiler.h>
#include <cstring>
#include <stdexcept>

namespace Rendering
{
	void DX12Renderer::CreatePipeline()
	{
		// b0 每帧更新，b1 每次 Draw 更新。
		D3D12_ROOT_PARAMETER roots[2] = {};
		roots[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		roots[0].Descriptor.ShaderRegister = 0;
		roots[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		roots[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		roots[1].Constants.ShaderRegister = 1;
		roots[1].Constants.Num32BitValues = sizeof(DrawConstants) / 4;
		roots[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
		rootDesc.NumParameters = 2;
		rootDesc.pParameters = roots;
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		ComPtr<ID3DBlob> signature, errors;
		HRESULT result = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors);
		if (FAILED(result) && errors)
			throw std::runtime_error(static_cast<const char*>(errors->GetBufferPointer()));
		Check(result, "SerializeRootSignature");
		Check(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)),
			  "CreateRootSignature");


		ComPtr<ID3DBlob> vertexShader, pixelShader;
		const std::filesystem::path shaderPath = ShaderPath("Basic.hlsl");
		const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
		result = D3DCompileFromFile(shaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1",
			flags, 0, &vertexShader, &errors);
		if (FAILED(result) && errors)
			throw std::runtime_error(static_cast<const char*>(errors->GetBufferPointer()));
		Check(result, "Compile VSMain");

		errors.Reset();
		result = D3DCompileFromFile(shaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1",
			flags, 0, &pixelShader, &errors);
		if (FAILED(result) && errors)
			throw std::runtime_error(static_cast<const char*>(errors->GetBufferPointer()));
		Check(result, "Compile PSMain");


		const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
		pipelineDesc.pRootSignature = m_rootSignature.Get();
		pipelineDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
		pipelineDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
		pipelineDesc.InputLayout = {inputLayout, 3};

		pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pipelineDesc.RasterizerState.DepthClipEnable = TRUE;

		D3D12_RENDER_TARGET_BLEND_DESC &blend = pipelineDesc.BlendState.RenderTarget[0];
		blend.SrcBlend = D3D12_BLEND_ONE;
		blend.DestBlend = D3D12_BLEND_ZERO;
		blend.BlendOp = D3D12_BLEND_OP_ADD;
		blend.SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.DestBlendAlpha = D3D12_BLEND_ZERO;
		blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blend.LogicOp = D3D12_LOGIC_OP_NOOP;
		blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		pipelineDesc.DepthStencilState.DepthEnable = TRUE;
		pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		pipelineDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		pipelineDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		pipelineDesc.DepthStencilState.FrontFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
			D3D12_COMPARISON_FUNC_ALWAYS};
		pipelineDesc.DepthStencilState.BackFace = pipelineDesc.DepthStencilState.FrontFace;


		pipelineDesc.SampleMask = UINT_MAX;
		pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineDesc.NumRenderTargets = 1;
		pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineDesc.SampleDesc.Count = 1;
		Check(m_device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&m_pipeline)), "CreateGraphicsPipelineState");
	}

	void DX12Renderer::CreateMesh(const MeshData &i_mesh)
	{
		if (i_mesh.m_vertices.empty() || i_mesh.m_indices.empty())
			throw std::runtime_error("DX12: empty mesh");

		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC buffer = {};
		buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		buffer.Height = 1;
		buffer.DepthOrArraySize = 1;
		buffer.MipLevels = 1;
		buffer.SampleDesc.Count = 1;
		buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		// 这里直接用 Upload Heap 存几何。
		buffer.Width = i_mesh.m_vertices.size() * sizeof(Vertex);
		Check(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&m_vertices)),
			  "Create vertex buffer");
		m_vertexView = {m_vertices->GetGPUVirtualAddress(), static_cast<UINT>(buffer.Width), sizeof(Vertex)};

		void* mapped = nullptr;
		D3D12_RANGE noRead = {0, 0};
		Check(m_vertices->Map(0, &noRead, &mapped), "Map vertices");
		std::memcpy(mapped, i_mesh.m_vertices.data(), static_cast<size_t>(buffer.Width));
		m_vertices->Unmap(0, nullptr);


		buffer.Width = i_mesh.m_indices.size() * sizeof(uint32_t);
		Check(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&m_indices)),
			  "Create index buffer");
		m_indexView = {m_indices->GetGPUVirtualAddress(), static_cast<UINT>(buffer.Width), DXGI_FORMAT_R32_UINT};
		Check(m_indices->Map(0, &noRead, &mapped), "Map indices");
		std::memcpy(mapped, i_mesh.m_indices.data(), static_cast<size_t>(buffer.Width));
		m_indices->Unmap(0, nullptr);

		// CBV 按 256 字节分配，每帧各一份。
		buffer.Width = 256;
		for (Frame &frame : m_frames)
		{
			Check(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&frame.m_constants)),
				  "Create frame constants");
			Check(frame.m_constants->Map(0, &noRead, reinterpret_cast<void**>(&frame.m_mapped)),
				  "Map frame constants");
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::RTV(UINT i_index) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<SIZE_T>(i_index) * m_rtvStride;
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::DSV(UINT i_index) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<SIZE_T>(i_index) * m_dsvStride;
		return handle;
	}

	void DX12Renderer::CreateTargets()
	{
		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC depth = {};
		depth.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depth.Width = m_width;
		depth.Height = m_height;
		depth.DepthOrArraySize = 1;
		depth.MipLevels = 1;
		depth.Format = DXGI_FORMAT_D32_FLOAT;
		depth.SampleDesc.Count = 1;
		depth.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clear = {};
		clear.Format = depth.Format;
		clear.DepthStencil.Depth = 1.0f;

		for (UINT index = 0; index < FrameCount; index++)
		{
			Check(m_swapChain->GetBuffer(index, IID_PPV_ARGS(&m_backBuffers[index])), "Get swap chain buffer");
			m_device->CreateRenderTargetView(m_backBuffers[index].Get(), nullptr, RTV(index));

			Check(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &depth,
				D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear,
				IID_PPV_ARGS(&m_frames[index].m_depth)),
				  "Create depth target");
			m_device->CreateDepthStencilView(m_frames[index].m_depth.Get(), nullptr, DSV(index));
		}
	}

	void DX12Renderer::Resize(unsigned i_width, unsigned i_height)
	{
		if (!i_width || !i_height)
		{
			// 最小化时不创建零尺寸纹理，也不继续提交帧。
			m_width = i_width;
			m_height = i_height;
			return;
		}
		if (i_width == m_width && i_height == m_height)
			return;

		WaitIdle();

		for (UINT index = 0; index < FrameCount; index++)
		{
			m_backBuffers[index].Reset();
			m_frames[index].m_depth.Reset();
			m_frames[index].m_fenceValue = 0;
		}
		m_hasRendered = false;

		UINT swapChainFlags = 0;
		if (m_allowTearing)
			swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		Check(m_swapChain->ResizeBuffers(FrameCount, i_width, i_height, DXGI_FORMAT_R8G8B8A8_UNORM, swapChainFlags),
			  "ResizeBuffers");
		m_width = i_width;
		m_height = i_height;
		CreateTargets();
	}
}
