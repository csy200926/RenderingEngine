#include "DX12Renderer.h"
#include "Files.h"
#include <d3dcompiler.h>
#include <cstring>
#include <stdexcept>

namespace Lite
{
	void DX12Renderer::CreatePipeline()
	{
		// 1. Root Signature 声明 Shader 的输入：b0 指向每帧常量，b1 直接携带 20 个 DWORD。
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
			throw std::runtime_error(static_cast<const char *>(errors->GetBufferPointer()));
		Check(result, "SerializeRootSignature");
		Check(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
											IID_PPV_ARGS(&m_rootSignature)),
			  "CreateRootSignature");
		ComPtr<ID3DBlob> vertexShader, pixelShader;
		const auto shader = ShaderPath("Basic.hlsl");
		const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
		result = D3DCompileFromFile(shader.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1",
									flags, 0, &vertexShader, &errors);
		if (FAILED(result) && errors)
			throw std::runtime_error(static_cast<const char *>(errors->GetBufferPointer()));
		Check(result, "Compile VSMain");
		errors.Reset();
		result = D3DCompileFromFile(shader.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1",
									flags, 0, &pixelShader, &errors);
		if (FAILED(result) && errors)
			throw std::runtime_error(static_cast<const char *>(errors->GetBufferPointer()));
		Check(result, "Compile PSMain");
		// 2. PSO 把 Shader、顶点布局、混合、光栅化和深度测试一次性组合好。
		const D3D12_INPUT_ELEMENT_DESC input[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
		pso.pRootSignature = m_rootSignature.Get();
		pso.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
		pso.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
		pso.InputLayout = {input, 3};
		pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pso.RasterizerState.DepthClipEnable = TRUE;
		auto &blend = pso.BlendState.RenderTarget[0];
		blend.SrcBlend = D3D12_BLEND_ONE;
		blend.DestBlend = D3D12_BLEND_ZERO;
		blend.BlendOp = D3D12_BLEND_OP_ADD;
		blend.SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.DestBlendAlpha = D3D12_BLEND_ZERO;
		blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blend.LogicOp = D3D12_LOGIC_OP_NOOP;
		blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pso.DepthStencilState.DepthEnable = TRUE;
		pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		pso.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		pso.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		pso.DepthStencilState.FrontFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
										   D3D12_COMPARISON_FUNC_ALWAYS};
		pso.DepthStencilState.BackFace = pso.DepthStencilState.FrontFace;
		pso.SampleMask = UINT_MAX;
		pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso.NumRenderTargets = 1;
		pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pso.SampleDesc.Count = 1;
		Check(m_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_pipeline)), "CreateGraphicsPipelineState");
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
		// 为保持 Demo 易读，静态几何直接放 Upload Heap，省去 staging 和 copy 提交。
		// 大型场景通常应复制到 Default Heap，让 GPU 从本地显存读取。
		buffer.Width = i_mesh.m_vertices.size() * sizeof(Vertex);
		Check(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_GENERIC_READ,
												nullptr, IID_PPV_ARGS(&m_vertices)),
			  "Create vertex buffer");
		m_vertexView = {m_vertices->GetGPUVirtualAddress(), static_cast<UINT>(buffer.Width), sizeof(Vertex)};
		void *mapped = nullptr;
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
		// CBV 地址需要 256 字节对齐；96 字节数据实际分配 256 字节，每帧独占一份。
		buffer.Width = 256;
		for (auto &frame : m_frames)
		{
			Check(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
													D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
													IID_PPV_ARGS(&frame.m_constants)),
				  "Create frame constants");
			Check(frame.m_constants->Map(0, &noRead, reinterpret_cast<void **>(&frame.m_mapped)),
				  "Map frame constants");
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::RTV(UINT i_index) const
	{
		auto handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<SIZE_T>(i_index) * m_rtvStride;
		return handle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::DSV(UINT i_index) const
	{
		auto handle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
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
		for (UINT index = 0; index < FrameCount; ++index)
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
		for (UINT index = 0; index < FrameCount; ++index)
		{
			m_backBuffers[index].Reset();
			m_frames[index].m_depth.Reset();
			m_frames[index].m_fenceValue = 0;
		}
		m_hasRendered = false;
		Check(m_swapChain->ResizeBuffers(FrameCount, i_width, i_height, DXGI_FORMAT_R8G8B8A8_UNORM,
										 m_allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0),
			  "ResizeBuffers");
		m_width = i_width;
		m_height = i_height;
		CreateTargets();
	}
} // namespace Lite
