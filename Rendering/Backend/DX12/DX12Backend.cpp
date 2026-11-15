#include "Rendering/Backend/IRenderBackend.h"
#include "Rendering/TextureManager.h"
#include "Platform/Paths.h"
#include "Platform/Log.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace Rendering
{
	using Microsoft::WRL::ComPtr;
	namespace
	{
		void CheckDX12(HRESULT i_result, const char *i_operation)
		{
			if (FAILED(i_result))
			{
				std::ostringstream message;
				message << i_operation << " failed, HRESULT=0x" << std::hex << static_cast<unsigned>(i_result);
				throw std::runtime_error(message.str());
			}
		}
		D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE i_type)
		{
			D3D12_HEAP_PROPERTIES result = {};
			result.Type = i_type;
			result.CreationNodeMask = result.VisibleNodeMask = 1;
			return result;
		}
		D3D12_RESOURCE_DESC BufferDescription(UINT64 i_bytes)
		{
			D3D12_RESOURCE_DESC result = {};
			result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			result.Width = i_bytes;
			result.Height = result.DepthOrArraySize = result.MipLevels = 1;
			result.SampleDesc.Count = 1;
			result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			return result;
		}
		void Transition(ID3D12GraphicsCommandList *i_list, ID3D12Resource *i_resource, D3D12_RESOURCE_STATES i_before,
						D3D12_RESOURCE_STATES i_after)
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition = {i_resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, i_before, i_after};
			i_list->ResourceBarrier(1, &barrier);
		}
		ComPtr<ID3DBlob> Compile(const std::string &i_source, const std::string &i_path, const char *i_entry,
								 const char *i_profile)
		{
			ComPtr<ID3DBlob> shader, errors;
			HRESULT result =
				D3DCompile(i_source.data(), i_source.size(), i_path.c_str(), nullptr, nullptr, i_entry, i_profile,
						   D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR, 0, &shader, &errors);
			if (FAILED(result))
				throw std::runtime_error("DX12 HLSL " + i_path + ": " +
										 (errors ? std::string(static_cast<const char *>(errors->GetBufferPointer()),
															   errors->GetBufferSize())
												 : i_entry));
			return shader;
		}
	} // namespace

	class DX12Backend final : public IRenderBackend
	{
	  public:
		~DX12Backend() override
		{
			// 析构不能再抛异常；正常退出仍通过 ShutDown 等待 GPU 完成。
			try
			{
				ShutDown();
			}
			catch (...)
			{
			}
		}
		void Initilize(void *i_window, int i_width, int i_height, int i_samples, bool i_warp) override;
		void Resize(int i_width, int i_height) override;
		void Render(const RenderFrame &i_frame) override;
		void CaptureFrame(const std::filesystem::path &i_path) override;
		void Present(bool i_vsync) override;
		void ShutDown() override;
		void SetProfiling(bool i_enabled) override
		{
			m_profiling = i_enabled;
			if (!i_enabled)
				m_gpuTime = -1;
		}
		double GetGPUTimeMilliseconds() const override { return m_gpuTime; }

	  private:
		static constexpr UINT FrameCount = 2;
		static constexpr UINT ShadowSize = 2048;
		static constexpr UINT TexturesPerDraw = 6;
		static constexpr size_t DrawCapacity = 1000000 / TexturesPerDraw;
		static constexpr UINT ConstantStride = (sizeof(DrawConstants) + 255) & ~255;
		static constexpr UINT TextureCapacity = 32768;
		struct FrameResources
		{
			ComPtr<ID3D12CommandAllocator> allocator;
			ComPtr<ID3D12Resource> constants, timing;
			ComPtr<ID3D12DescriptorHeap> descriptors;
			std::vector<ComPtr<ID3D12Resource>> uploads;
			UINT64 fence = 0;
			size_t capacity = 0;
			bool timed = false;
		};
		struct MeshBuffers
		{
			ComPtr<ID3D12Resource> vertices, indices;
			uint64_t revision = 0;
			bool terrain = false;
			std::weak_ptr<Mesh> owner;
			D3D12_VERTEX_BUFFER_VIEW vertexView = {};
			D3D12_INDEX_BUFFER_VIEW indexView = {};
		};
		struct TextureResource
		{
			ComPtr<ID3D12Resource> resource;
			UINT descriptor = 0;
		};
		struct Pipelines
		{
			std::array<ComPtr<ID3D12PipelineState>, 5> states;
		};
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12CommandQueue> m_queue;
		ComPtr<IDXGISwapChain3> m_swap;
		ComPtr<ID3D12GraphicsCommandList> m_list;
		ComPtr<ID3D12Fence> m_fence;
		ComPtr<ID3D12RootSignature> m_root;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap, m_dsvHeap, m_textureHeap;
		ComPtr<ID3D12Resource> m_depth, m_shadowColor, m_shadowDepth;
		UINT m_shadowDescriptor = 0, m_nullShadowDescriptor = 0;
		bool m_shadowInitialized = false;
		ComPtr<ID3D12QueryHeap> m_queries;
		std::array<ComPtr<ID3D12Resource>, FrameCount> m_backBuffers;
		std::array<FrameResources, FrameCount> m_frames;
		std::map<uint64_t, MeshBuffers> m_meshes;
		std::map<uint64_t, TextureResource> m_textures, m_cubes;
		std::map<std::string, Pipelines> m_pipelines;
		HANDLE m_event = nullptr;
		UINT64 m_nextFence = 1, m_frequency = 0;
		UINT m_rtvSize = 0, m_srvSize = 0, m_textureCount = 0, m_lastFrame = 0;
		int m_width = 0, m_height = 0;
		bool m_profiling = false, m_hasFrame = false;
		double m_gpuTime = -1;
		void CreateShadowTargets();
		void Wait(UINT64 i_value);
		void WaitIdle();
		ComPtr<ID3D12Resource> CreateBuffer(UINT64 i_bytes, D3D12_HEAP_TYPE i_heap, D3D12_RESOURCE_STATES i_state);
		ComPtr<ID3D12Resource> UploadBuffer(const void *i_data, size_t i_bytes, D3D12_RESOURCE_STATES i_state,
											FrameResources &i_frame);
		MeshBuffers &GetMesh(const MeshPtr &i_mesh, FrameResources &i_frame);
		UINT GetTexture(const TexturePtr &i_texture, FrameResources &i_frame);
		UINT GetCube(const TextureCubePtr &i_cube, FrameResources &i_frame);
		TextureResource UploadTexture(const std::vector<TexturePtr> &i_faces, bool i_cube, FrameResources &i_frame);
		D3D12_CPU_DESCRIPTOR_HANDLE TextureHandle(UINT i_index) const;
		Pipelines &GetPipelines(const MaterialPtr &i_material);
		void Draw(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode, FrameResources &i_resources,
				  unsigned char *i_constants, size_t &io_drawIndex);
	};

	ComPtr<ID3D12Resource> DX12Backend::CreateBuffer(UINT64 i_bytes, D3D12_HEAP_TYPE i_heap,
													 D3D12_RESOURCE_STATES i_state)
	{
		ComPtr<ID3D12Resource> result;
		auto heap = HeapProperties(i_heap);
		auto description = BufferDescription(i_bytes);
		CheckDX12(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description, i_state, nullptr,
													IID_PPV_ARGS(&result)),
				  "Create DX12 buffer");
		return result;
	}
	void DX12Backend::Wait(UINT64 i_value)
	{
		if (!i_value || m_fence->GetCompletedValue() >= i_value)
			return;
		CheckDX12(m_fence->SetEventOnCompletion(i_value, m_event), "Set DX12 fence event");
		DWORD result = WaitForSingleObject(m_event, 30000);
		if (result != WAIT_OBJECT_0)
			throw std::runtime_error("DX12 GPU fence wait failed/timed out, device status=" +
									 std::to_string(m_device->GetDeviceRemovedReason()));
	}
	void DX12Backend::WaitIdle()
	{
		if (!m_queue || !m_fence)
			return;
		UINT64 value = m_nextFence++;
		CheckDX12(m_queue->Signal(m_fence.Get(), value), "Signal DX12 idle fence");
		Wait(value);
	}
	D3D12_CPU_DESCRIPTOR_HANDLE DX12Backend::TextureHandle(UINT i_index) const
	{
		auto handle = m_textureHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<SIZE_T>(i_index) * m_srvSize;
		return handle;
	}

	void DX12Backend::Initilize(void *i_window, int i_width, int i_height, int i_samples, bool i_warp)
	{
		// 第一步：显式选择硬件或 WARP，不把设备创建失败静默变成另一种后端。
		ComPtr<IDXGIFactory4> factory;
		CheckDX12(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "Create DX12 DXGI factory");
		ComPtr<IDXGIAdapter1> adapter;
		if (i_warp)
			CheckDX12(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)), "Select DX12 WARP adapter");
		else
		{
			for (UINT index = 0; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index)
			{
				DXGI_ADAPTER_DESC1 description = {};
				CheckDX12(adapter->GetDesc1(&description), "Read DX12 adapter");
				if (!(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
					SUCCEEDED(
						D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
					break;
				adapter.Reset();
			}
			if (!adapter)
				throw std::runtime_error("No DX12 hardware adapter; use --warp explicitly for software rendering");
		}
		CheckDX12(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)),
				  "Create DX12 device");
		D3D12_COMMAND_QUEUE_DESC queue = {};
		queue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		CheckDX12(m_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&m_queue)), "Create DX12 queue");
		CheckDX12(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "Create DX12 fence");
		m_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (!m_event)
			throw std::runtime_error("Create DX12 fence event failed");
		CheckDX12(m_queue->GetTimestampFrequency(&m_frequency), "Read DX12 timestamp frequency");
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = {};
		HRESULT rayQuery = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options));
		Platform::Log("DX12 device raytracing tier=" +
					  std::to_string(SUCCEEDED(rayQuery) ? static_cast<int>(options.RaytracingTier) : 0) +
					  "; hardware ray tracing passes are not implemented");
		DXGI_SWAP_CHAIN_DESC1 swap = {};
		swap.Width = std::max(1, i_width);
		swap.Height = std::max(1, i_height);
		swap.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap.SampleDesc.Count = 1;
		swap.BufferCount = FrameCount;
		swap.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		ComPtr<IDXGISwapChain1> swap1;
		CheckDX12(factory->CreateSwapChainForHwnd(m_queue.Get(), static_cast<HWND>(i_window), &swap, nullptr, nullptr,
												  &swap1),
				  "Create DX12 swapchain");
		CheckDX12(swap1.As(&m_swap), "Query DX12 swapchain3");
		CheckDX12(factory->MakeWindowAssociation(static_cast<HWND>(i_window), DXGI_MWA_NO_ALT_ENTER),
				  "Disable DX12 implicit fullscreen");
		// 第二步：当前直接绘制到单采样交换链。查询请求能力，但如实报告未启用 MSAA。
		if (i_samples > 1)
		{
			D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS samples = {};
			samples.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			samples.SampleCount = static_cast<UINT>(i_samples);
			HRESULT supported =
				m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &samples, sizeof(samples));
			Platform::Log("DX12: requested MSAA=" + std::to_string(i_samples) +
						  ", qualityLevels=" + std::to_string(SUCCEEDED(supported) ? samples.NumQualityLevels : 0) +
						  "; this backend currently uses samples=1 (MSAA resolve not implemented)");
		}
		auto createHeap = [&](D3D12_DESCRIPTOR_HEAP_TYPE i_type, UINT i_count, ComPtr<ID3D12DescriptorHeap> &o_heap)
		{
			D3D12_DESCRIPTOR_HEAP_DESC description = {};
			description.Type = i_type;
			description.NumDescriptors = i_count;
			CheckDX12(m_device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&o_heap)),
					  "Create DX12 descriptor heap");
		};
		createHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FrameCount + 1, m_rtvHeap);
		createHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, m_dsvHeap);
		createHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, TextureCapacity, m_textureHeap);
		m_rtvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// 空 Cube SRV 读取为黑色：没有天空/辐照度时输入有定义，不绑定无效描述符。
		D3D12_SHADER_RESOURCE_VIEW_DESC nullCube = {};
		nullCube.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		nullCube.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		nullCube.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		nullCube.TextureCube.MipLevels = 1;
		m_device->CreateShaderResourceView(nullptr, &nullCube, TextureHandle(m_textureCount++));
		CreateShadowTargets();
		// b0 是逐 Draw 根 CBV；t0..t5 是连续 SRV 表，t5 用于阴影。
		D3D12_DESCRIPTOR_RANGE range = {};
		range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range.NumDescriptors = TexturesPerDraw;
		D3D12_ROOT_PARAMETER parameters[2] = {};
		parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameters[0].Descriptor.ShaderRegister = 0;
		parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		parameters[1].DescriptorTable = {1, &range};
		parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
		for (UINT i = 0; i < 2; ++i)
		{
			auto &sampler = samplers[i];
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			sampler.AddressU = sampler.AddressV = sampler.AddressW =
				i ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;
			sampler.MaxAnisotropy = 1;
			sampler.ShaderRegister = i;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}
		D3D12_ROOT_SIGNATURE_DESC root = {};
		root.NumParameters = 2;
		root.pParameters = parameters;
		root.NumStaticSamplers = 2;
		root.pStaticSamplers = samplers;
		root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		ComPtr<ID3DBlob> serialized, errors;
		CheckDX12(D3D12SerializeRootSignature(&root, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors),
				  "Serialize DX12 root signature");
		CheckDX12(m_device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
												IID_PPV_ARGS(&m_root)),
				  "Create DX12 root signature");
		D3D12_QUERY_HEAP_DESC queries = {};
		queries.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		queries.Count = FrameCount * 2;
		CheckDX12(m_device->CreateQueryHeap(&queries, IID_PPV_ARGS(&m_queries)), "Create DX12 timestamp heap");
		for (auto &frame : m_frames)
		{
			CheckDX12(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.allocator)),
					  "Create DX12 frame allocator");
			frame.timing = CreateBuffer(16, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
		}
		CheckDX12(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frames[0].allocator.Get(), nullptr,
											  IID_PPV_ARGS(&m_list)),
				  "Create DX12 command list");
		CheckDX12(m_list->Close(), "Close initial DX12 command list");
		Resize(std::max(1, i_width), std::max(1, i_height));
		GetPipelines(nullptr);
		DXGI_ADAPTER_DESC1 description = {};
		CheckDX12(adapter->GetDesc1(&description), "Read DX12 adapter name");
		Platform::Log("Renderer: DX12, GPU=" + Platform::ToUTF8(description.Description) + ", device=" +
					  (i_warp ? "WARP" : "hardware") + ", samples=1; raster PBR + sun shadow map, no GI baking/ray tracing");
	}
	void DX12Backend::CreateShadowTargets()
	{
		// 固定分辨率阴影不随窗口缩放。单个队列按顺序完成阴影写入和场景读取。
		D3D12_RESOURCE_DESC color = {};
		color.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		color.Width = color.Height = ShadowSize;
		color.DepthOrArraySize = color.MipLevels = 1;
		color.SampleDesc.Count = 1;
		color.Format = DXGI_FORMAT_R32_FLOAT;
		color.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		D3D12_CLEAR_VALUE clear = {};
		clear.Format = color.Format;
		clear.Color[0] = clear.Color[1] = clear.Color[2] = clear.Color[3] = 1;
		auto heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CheckDX12(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &color,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear, IID_PPV_ARGS(&m_shadowColor)), "Create DX12 shadow color");
		auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += FrameCount * m_rtvSize;
		m_device->CreateRenderTargetView(m_shadowColor.Get(), nullptr, rtv);
		D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
		view.Format = color.Format;
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		view.Texture2D.MipLevels = 1;
		m_shadowDescriptor = m_textureCount++;
		m_device->CreateShaderResourceView(m_shadowColor.Get(), &view, TextureHandle(m_shadowDescriptor));
		// 阴影绘制时绑定空 SRV，避免把正在写入的阴影纹理同时当作输入。
		m_nullShadowDescriptor = m_textureCount++;
		m_device->CreateShaderResourceView(nullptr, &view, TextureHandle(m_nullShadowDescriptor));
		color.Format = DXGI_FORMAT_D32_FLOAT;
		color.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		clear = {};
		clear.Format = color.Format;
		clear.DepthStencil.Depth = 1;
		CheckDX12(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &color,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&m_shadowDepth)), "Create DX12 shadow depth");
		auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		dsv.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_device->CreateDepthStencilView(m_shadowDepth.Get(), nullptr, dsv);
		m_shadowInitialized = false;
	}
	void DX12Backend::Resize(int i_width, int i_height)
	{
		if (!m_swap || i_width < 1 || i_height < 1)
			return;
		if (m_width == i_width && m_height == i_height)
			return;
		// resize 会释放交换链纹理，必须先等所有旧命令使用完它们。
		WaitIdle();
		for (auto &buffer : m_backBuffers)
			buffer.Reset();
		m_depth.Reset();
		CheckDX12(m_swap->ResizeBuffers(FrameCount, i_width, i_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0),
				  "Resize DX12 swapchain");
		m_width = i_width;
		m_height = i_height;
		m_hasFrame = false;
		auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 0; i < FrameCount; ++i)
		{
			CheckDX12(m_swap->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])), "Get DX12 back buffer");
			m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtv);
			rtv.ptr += m_rtvSize;
		}
		D3D12_RESOURCE_DESC depth = {};
		depth.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depth.Width = i_width;
		depth.Height = i_height;
		depth.DepthOrArraySize = depth.MipLevels = 1;
		depth.SampleDesc.Count = 1;
		depth.Format = DXGI_FORMAT_D32_FLOAT;
		depth.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		D3D12_CLEAR_VALUE clear = {};
		clear.Format = depth.Format;
		clear.DepthStencil.Depth = 1;
		auto heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CheckDX12(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &depth,
													D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&m_depth)),
				  "Create DX12 depth target");
		m_device->CreateDepthStencilView(m_depth.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	ComPtr<ID3D12Resource> DX12Backend::UploadBuffer(const void *i_data, size_t i_bytes, D3D12_RESOURCE_STATES i_state,
													 FrameResources &i_frame)
	{
		// CPU 写 Upload Heap，再在当前命令列表复制到 Default Heap；临时上传缓冲保留到本帧 Fence 完成。
		auto destination = CreateBuffer(i_bytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
		auto upload = CreateBuffer(i_bytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		void *mapped = nullptr;
		D3D12_RANGE noRead = {0, 0};
		CheckDX12(upload->Map(0, &noRead, &mapped), "Map DX12 buffer upload");
		memcpy(mapped, i_data, i_bytes);
		upload->Unmap(0, nullptr);
		m_list->CopyBufferRegion(destination.Get(), 0, upload.Get(), 0, i_bytes);
		Transition(m_list.Get(), destination.Get(), D3D12_RESOURCE_STATE_COPY_DEST, i_state);
		i_frame.uploads.push_back(std::move(upload));
		return destination;
	}
	DX12Backend::MeshBuffers &DX12Backend::GetMesh(const MeshPtr &i_mesh, FrameResources &i_frame)
	{
		auto found = m_meshes.find(i_mesh->GetHandle());
		if (found != m_meshes.end() && found->second.revision == i_mesh->m_revision)
			return found->second;
		if (found != m_meshes.end())
		{
			// 旧Default资源挂到本帧，等本帧fence完成才释放，也覆盖前面排队的帧。
			i_frame.uploads.push_back(found->second.vertices);
			i_frame.uploads.push_back(found->second.indices);
			m_meshes.erase(found);
		}
		size_t vertexBytes = i_mesh->m_vertices.size() * sizeof(Vertex);
		size_t indexBytes = i_mesh->m_indices.size() * sizeof(unsigned);
		if (!vertexBytes || !indexBytes || vertexBytes > UINT_MAX || indexBytes > UINT_MAX)
			throw std::runtime_error("DX12 mesh is empty or exceeds buffer view size");
		MeshBuffers mesh;
		mesh.revision = i_mesh->m_revision;
		mesh.terrain = bool(i_mesh->m_terrain);
		mesh.owner = i_mesh;
		mesh.vertices = UploadBuffer(i_mesh->m_vertices.data(), vertexBytes,
									 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, i_frame);
		mesh.indices = UploadBuffer(i_mesh->m_indices.data(), indexBytes, D3D12_RESOURCE_STATE_INDEX_BUFFER, i_frame);
		mesh.vertexView = {mesh.vertices->GetGPUVirtualAddress(), static_cast<UINT>(vertexBytes), sizeof(Vertex)};
		mesh.indexView = {mesh.indices->GetGPUVirtualAddress(), static_cast<UINT>(indexBytes), DXGI_FORMAT_R32_UINT};
		return m_meshes.emplace(i_mesh->GetHandle(), std::move(mesh)).first->second;
	}
	DX12Backend::TextureResource DX12Backend::UploadTexture(const std::vector<TexturePtr> &i_faces, bool i_cube,
															FrameResources &i_frame)
	{
		if (m_textureCount >= TextureCapacity)
			throw std::runtime_error("DX12 persistent texture descriptor heap is full");
		const auto &first = i_faces.at(0);
		if (!first || first->m_width < 1 || first->m_height < 1)
			throw std::runtime_error("DX12 texture dimensions are invalid");
		UINT width = first->m_width, height = first->m_height;
		if (i_cube && width != height)
			throw std::runtime_error("DX12 cubemap faces must be square");
		UINT mipCount = 1;
		for (UINT size = std::max(width, height); size > 1; size >>= 1)
			++mipCount;
		// CPU 生成完整 mip 链，避免远处贴图只有最高分辨率而闪烁；原始 Texture 仍归共享资源系统所有。
		std::vector<std::vector<unsigned char>> pixels;
		for (const auto &face : i_faces)
		{
			if (!face || face->m_width != static_cast<int>(width) || face->m_height != static_cast<int>(height) ||
				face->m_pixels.size() != static_cast<size_t>(width) * height * 4)
				throw std::runtime_error("DX12 texture face size/pixels do not match");
			pixels.push_back(face->m_pixels);
			UINT oldWidth = width, oldHeight = height;
			for (UINT mip = 1; mip < mipCount; ++mip)
			{
				UINT nextWidth = std::max(1u, oldWidth / 2), nextHeight = std::max(1u, oldHeight / 2);
				std::vector<unsigned char> next(static_cast<size_t>(nextWidth) * nextHeight * 4);
				const auto &previous = pixels.back();
				for (UINT y = 0; y < nextHeight; ++y)
					for (UINT x = 0; x < nextWidth; ++x)
						for (UINT channel = 0; channel < 4; ++channel)
						{
							unsigned sum = 0;
							for (UINT dy = 0; dy < 2; ++dy)
								for (UINT dx = 0; dx < 2; ++dx)
									sum +=
										previous[(static_cast<size_t>(std::min(oldHeight - 1, y * 2 + dy)) * oldWidth +
												  std::min(oldWidth - 1, x * 2 + dx)) *
													 4 +
												 channel];
							next[(static_cast<size_t>(y) * nextWidth + x) * 4 + channel] =
								static_cast<unsigned char>((sum + 2) / 4);
						}
				pixels.push_back(std::move(next));
				oldWidth = nextWidth;
				oldHeight = nextHeight;
			}
		}
		D3D12_RESOURCE_DESC description = {};
		description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		description.Width = width;
		description.Height = height;
		description.DepthOrArraySize = static_cast<UINT16>(i_faces.size());
		description.MipLevels = static_cast<UINT16>(mipCount);
		description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		description.SampleDesc.Count = 1;
		TextureResource texture;
		auto heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CheckDX12(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description,
													D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
													IID_PPV_ARGS(&texture.resource)),
				  "Create DX12 texture");
		UINT count = static_cast<UINT>(pixels.size());
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(count);
		std::vector<UINT> rows(count);
		std::vector<UINT64> rowBytes(count);
		UINT64 bytes = 0;
		m_device->GetCopyableFootprints(&description, 0, count, 0, layouts.data(), rows.data(), rowBytes.data(),
										&bytes);
		auto upload = CreateBuffer(bytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
		unsigned char *mapped = nullptr;
		D3D12_RANGE noRead = {0, 0};
		CheckDX12(upload->Map(0, &noRead, reinterpret_cast<void **>(&mapped)), "Map DX12 texture upload");
		// D3D12 每行需要 256 字节对齐；不能把紧密的 RGBA 数组直接当作 GPU 行距。
		for (UINT subresource = 0; subresource < count; ++subresource)
		{
			const auto &layout = layouts[subresource];
			for (UINT row = 0; row < rows[subresource]; ++row)
				memcpy(mapped + layout.Offset + static_cast<size_t>(row) * layout.Footprint.RowPitch,
					   pixels[subresource].data() + static_cast<size_t>(row) * rowBytes[subresource],
					   static_cast<size_t>(rowBytes[subresource]));
		}
		upload->Unmap(0, nullptr);
		for (UINT subresource = 0; subresource < count; ++subresource)
		{
			D3D12_TEXTURE_COPY_LOCATION source = {}, destination = {};
			source.pResource = upload.Get();
			source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			source.PlacedFootprint = layouts[subresource];
			destination.pResource = texture.resource.Get();
			destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			destination.SubresourceIndex = subresource;
			m_list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
		}
		Transition(m_list.Get(), texture.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
				   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		i_frame.uploads.push_back(std::move(upload));
		D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
		view.Format = description.Format;
		view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		view.ViewDimension = i_cube ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
		if (i_cube)
			view.TextureCube.MipLevels = mipCount;
		else
			view.Texture2D.MipLevels = mipCount;
		texture.descriptor = m_textureCount++;
		m_device->CreateShaderResourceView(texture.resource.Get(), &view, TextureHandle(texture.descriptor));
		return texture;
	}
	UINT DX12Backend::GetTexture(const TexturePtr &i_texture, FrameResources &i_frame)
	{
		auto texture = i_texture ? i_texture : TextureManager::GetInstance()->GetWhiteTexture();
		auto found = m_textures.find(texture->GetHandle());
		if (found == m_textures.end())
			found = m_textures.emplace(texture->GetHandle(), UploadTexture({texture}, false, i_frame)).first;
		return found->second.descriptor;
	}
	UINT DX12Backend::GetCube(const TextureCubePtr &i_cube, FrameResources &i_frame)
	{
		if (!i_cube)
			return 0;
		auto found = m_cubes.find(i_cube->GetHandle());
		if (found == m_cubes.end())
			found = m_cubes
						.emplace(i_cube->GetHandle(),
								 UploadTexture(std::vector<TexturePtr>(i_cube->m_faces.begin(), i_cube->m_faces.end()),
											   true, i_frame))
						.first;
		return found->second.descriptor;
	}

	DX12Backend::Pipelines &DX12Backend::GetPipelines(const MaterialPtr &i_material)
	{
		// 沿用材质已有 HLSL 路径：接口/寄存器完全相同，DX12 只将 profile 提升到 5.1。
		std::string path = i_material ? i_material->m_dxShader : "Shaders/DX11/Default.hlsl";
		auto found = m_pipelines.find(path);
		if (found != m_pipelines.end())
			return found->second;
		auto source = Platform::ReadText(Platform::GetAssetPath(path));
		auto vs = Compile(source, path, "VSMain", "vs_5_1"), ps = Compile(source, path, "PSMain", "ps_5_1");
		D3D12_INPUT_ELEMENT_DESC layout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
											  offsetof(Vertex, _position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
											  0},
											 {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, _normal),
											  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
											 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, _texCoords),
											  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
		Pipelines pipelines;
		// PSO 顺序：不透明、精灵、天空、透明、阴影。阴影双面绘制并写入深度。
		for (UINT mode = 0; mode < pipelines.states.size(); ++mode)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
			state.pRootSignature = m_root.Get();
			state.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
			state.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
			state.InputLayout = {layout, 3};
			state.SampleMask = UINT_MAX;
			state.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			state.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			state.RasterizerState.FrontCounterClockwise = TRUE;
			state.RasterizerState.DepthClipEnable = TRUE;
			state.DepthStencilState.DepthEnable = mode != 1;
			state.DepthStencilState.DepthWriteMask =
				(mode == 0 || mode == 4) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			state.DepthStencilState.DepthFunc = mode == 4 ? D3D12_COMPARISON_FUNC_LESS : D3D12_COMPARISON_FUNC_LESS_EQUAL;
			state.DepthStencilState.StencilReadMask = state.DepthStencilState.StencilWriteMask = 0xff;
			state.DepthStencilState.FrontFace.StencilFailOp = state.DepthStencilState.FrontFace.StencilDepthFailOp =
				state.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			state.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			state.DepthStencilState.BackFace = state.DepthStencilState.FrontFace;
			auto &blend = state.BlendState.RenderTarget[0];
			blend.BlendEnable = mode == 1 || mode == 3;
			blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			blend.BlendOp = D3D12_BLEND_OP_ADD;
			blend.SrcBlendAlpha = D3D12_BLEND_ONE;
			blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			blend.LogicOp = D3D12_LOGIC_OP_NOOP;
			blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			state.NumRenderTargets = 1;
			state.RTVFormats[0] = mode == 4 ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
			state.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			state.SampleDesc.Count = 1;
			CheckDX12(m_device->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&pipelines.states[mode])),
					  "Create DX12 graphics PSO");
		}
		return m_pipelines.emplace(path, std::move(pipelines)).first->second;
	}
	void DX12Backend::Draw(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode, FrameResources &i_resources,
						   unsigned char *i_constants, size_t &io_drawIndex)
	{
		if (!i_item.m_mesh)
			return;
		auto &mesh = GetMesh(i_item.m_mesh, i_resources);
		auto &part = i_item.m_mesh->m_subMeshes.at(i_item.m_submesh);
		auto constants = MakeConstants(i_frame, i_item, i_mode);
		memcpy(i_constants + io_drawIndex * ConstantStride, &constants, sizeof(constants));
		m_list->SetGraphicsRootConstantBufferView(0, i_resources.constants->GetGPUVirtualAddress() +
														 io_drawIndex * ConstantStride);
		UINT textures[] = {GetTexture(i_item.m_material ? i_item.m_material->m_texture : nullptr, i_resources),
						   GetCube(i_frame.m_irradiance, i_resources), GetCube(i_frame.m_sky, i_resources),
						   GetTexture(i_item.m_material ? i_item.m_material->m_opacityTexture : nullptr, i_resources),
						   GetTexture(i_item.m_material ? i_item.m_material->m_normalTexture : nullptr, i_resources),
						   i_mode == 3 ? m_nullShadowDescriptor : m_shadowDescriptor};
		auto cpu = i_resources.descriptors->GetCPUDescriptorHandleForHeapStart();
		auto gpu = i_resources.descriptors->GetGPUDescriptorHandleForHeapStart();
		cpu.ptr += io_drawIndex * TexturesPerDraw * m_srvSize;
		gpu.ptr += io_drawIndex * TexturesPerDraw * m_srvSize;
		// 每次 Draw 的六个 SRV 独占一段描述符，后一个 Draw 不会覆盖尚未执行的 Draw。
		for (UINT i = 0; i < TexturesPerDraw; ++i)
		{
			m_device->CopyDescriptorsSimple(1, cpu, TextureHandle(textures[i]), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cpu.ptr += m_srvSize;
		}
		m_list->SetGraphicsRootDescriptorTable(1, gpu);
		bool transparent = i_item.m_material && i_item.m_material->m_transparent;
		UINT pipeline = i_mode == 3 ? 4 : (i_mode == 0 && transparent ? 3 : static_cast<UINT>(i_mode));
		m_list->SetPipelineState(GetPipelines(i_mode == 3 ? nullptr : i_item.m_material).states[pipeline].Get());
		m_list->IASetVertexBuffers(0, 1, &mesh.vertexView);
		m_list->IASetIndexBuffer(&mesh.indexView);
		m_list->DrawIndexedInstanced(part.m_indexCount, 1, part.m_firstIndex, 0, 0);
		++io_drawIndex;
	}
	void DX12Backend::Render(const RenderFrame &i_frame)
	{
		if (!m_swap || m_width < 1 || m_height < 1)
			return;
		UINT index = m_swap->GetCurrentBackBufferIndex();
		auto &frame = m_frames[index];
		// 第一步：只等待将要复用的帧。Fence 完成后才能重置 allocator、释放上传资源、重写常量与 SRV。
		Wait(frame.fence);
		if (frame.timed && m_profiling)
		{
			UINT64 *timestamps = nullptr;
			D3D12_RANGE range = {0, 16};
			CheckDX12(frame.timing->Map(0, &range, reinterpret_cast<void **>(&timestamps)), "Map DX12 GPU timestamps");
			m_gpuTime = m_frequency ? static_cast<double>(timestamps[1] - timestamps[0]) * 1000.0 / m_frequency : -1;
			D3D12_RANGE noWrite = {0, 0};
			frame.timing->Unmap(0, &noWrite);
		}
		frame.uploads.clear();
		// 过期地形也先进入本帧延迟释放队列；本帧fence覆盖此前排队使用它的帧。
		for (auto it = m_meshes.begin(); it != m_meshes.end();)
		{
			if (it->second.terrain && it->second.owner.expired())
			{
				frame.uploads.push_back(it->second.vertices);
				frame.uploads.push_back(it->second.indices);
				it = m_meshes.erase(it);
			}
			else
				++it;
		}

		size_t shadowDraws = 0;
		if (i_frame.m_shadowParams.w > 0)
			for (const auto &item : i_frame.m_items)
				if (CastsShadow(i_frame, item))
					++shadowDraws;
		size_t needed = i_frame.m_items.size() + shadowDraws + 1;
		if (needed > DrawCapacity)
			throw std::runtime_error("DX12 draw count exceeds per-frame descriptor limit");
		if (needed > frame.capacity)
		{
			// 当前帧已完成，可单独扩大它自己的内存，不影响另一帧的资源。
			frame.capacity =
				std::min<size_t>(DrawCapacity, std::max<size_t>(needed, std::max<size_t>(64, frame.capacity * 2)));
			frame.constants = CreateBuffer(frame.capacity * ConstantStride, D3D12_HEAP_TYPE_UPLOAD,
										   D3D12_RESOURCE_STATE_GENERIC_READ);
			D3D12_DESCRIPTOR_HEAP_DESC descriptors = {};
			descriptors.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			descriptors.NumDescriptors = static_cast<UINT>(frame.capacity * TexturesPerDraw);
			descriptors.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			frame.descriptors.Reset();
			CheckDX12(m_device->CreateDescriptorHeap(&descriptors, IID_PPV_ARGS(&frame.descriptors)),
					  "Create DX12 per-frame shader heap");
		}
		CheckDX12(frame.allocator->Reset(), "Reset DX12 frame allocator");
		CheckDX12(m_list->Reset(frame.allocator.Get(), nullptr), "Reset DX12 command list");
		unsigned char *constants = nullptr;
		D3D12_RANGE noRead = {0, 0};
		CheckDX12(frame.constants->Map(0, &noRead, reinterpret_cast<void **>(&constants)), "Map DX12 frame constants");
		// 第二步：记录 GPU 时间起点与 PRESENT -> RENDER_TARGET 屏障，再清颜色和深度。
		frame.timed = m_profiling;
		if (frame.timed)
			m_list->EndQuery(m_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2);
		m_list->SetGraphicsRootSignature(m_root.Get());
		ID3D12DescriptorHeap *heaps[] = {frame.descriptors.Get()};
		m_list->SetDescriptorHeaps(1, heaps);
		m_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		size_t drawIndex = 0;
		// 即使第一帧关闭阴影，也要先清成远平面，确保场景绑定的纹理已经初始化。
		if (i_frame.m_shadowParams.w > 0 || !m_shadowInitialized)
		{
			Transition(m_list.Get(), m_shadowColor.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			auto shadowRTV = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
			shadowRTV.ptr += FrameCount * m_rtvSize;
			auto shadowDSV = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
			shadowDSV.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			float clear[] = {1, 1, 1, 1};
			m_list->ClearRenderTargetView(shadowRTV, clear, 0, nullptr);
			m_list->ClearDepthStencilView(shadowDSV, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
			m_list->OMSetRenderTargets(1, &shadowRTV, FALSE, &shadowDSV);
			D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(ShadowSize), static_cast<float>(ShadowSize), 0, 1};
			D3D12_RECT scissor = {0, 0, ShadowSize, ShadowSize};
			m_list->RSSetViewports(1, &viewport);
			m_list->RSSetScissorRects(1, &scissor);
			try
			{
				if (i_frame.m_shadowParams.w > 0)
					for (const auto &item : i_frame.m_items)
						if (CastsShadow(i_frame, item))
							Draw(i_frame, item, 3, frame, constants, drawIndex);
			}
			catch (...)
			{
				frame.constants->Unmap(0, nullptr);
				throw;
			}
			Transition(m_list.Get(), m_shadowColor.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
		Transition(m_list.Get(), m_backBuffers[index].Get(), D3D12_RESOURCE_STATE_PRESENT,
				   D3D12_RESOURCE_STATE_RENDER_TARGET);
		auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += index * m_rtvSize;
		auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		float clear[] = {.025f, .04f, .065f, 1};
		m_list->ClearRenderTargetView(rtv, clear, 0, nullptr);
		m_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
		m_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0, 1};
		D3D12_RECT scissor = {0, 0, m_width, m_height};
		m_list->RSSetViewports(1, &viewport);
		m_list->RSSetScissorRects(1, &scissor);
		// 第三步：先天空再物体；透明排序和精灵顺序由 RenderingEngine 统一收集，后端保留该顺序。
		try
		{
			if (i_frame.m_drawSky && i_frame.m_sky && i_frame.m_skyMesh)
			{
				DrawItem sky;
				sky.m_mesh = i_frame.m_skyMesh;
				sky.m_transform =
					glm::translate(glm::mat4(1), i_frame.m_camera.cameraPos) * glm::scale(glm::mat4(1), glm::vec3(100));
				Draw(i_frame, sky, 2, frame, constants, drawIndex);
			}
			for (const auto &item : i_frame.m_items)
				Draw(i_frame, item, item.m_sprite ? 1 : 0, frame, constants, drawIndex);
		}
		catch (...)
		{
			frame.constants->Unmap(0, nullptr);
			throw;
		}
		frame.constants->Unmap(0, nullptr);
		Transition(m_list.Get(), m_backBuffers[index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
				   D3D12_RESOURCE_STATE_PRESENT);
		if (frame.timed)
		{
			m_list->EndQuery(m_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2 + 1);
			m_list->ResolveQueryData(m_queries.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2, 2, frame.timing.Get(), 0);
		}
		// 第四步：提交并发出帧 Fence。Present 单独调用，截图可在 Present 前读取这张已渲染的交换链纹理。
		CheckDX12(m_list->Close(), "Close DX12 frame command list");
		ID3D12CommandList *lists[] = {m_list.Get()};
		m_queue->ExecuteCommandLists(1, lists);
		m_shadowInitialized = true;
		frame.fence = m_nextFence++;
		CheckDX12(m_queue->Signal(m_fence.Get(), frame.fence), "Signal DX12 frame fence");
		m_lastFrame = index;
		m_hasFrame = true;
	}

	void DX12Backend::CaptureFrame(const std::filesystem::path &i_path)
	{
		if (!m_hasFrame)
			throw std::runtime_error("DX12 capture requires a rendered frame before Present");
		// 第一步：截图是显式同步操作；等待主绘制，使用独立 allocator，避免破坏飞行中的帧资源。
		WaitIdle();
		auto description = m_backBuffers[m_lastFrame]->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
		UINT rows = 0;
		UINT64 rowBytes = 0, bytes = 0;
		m_device->GetCopyableFootprints(&description, 0, 1, 0, &layout, &rows, &rowBytes, &bytes);
		auto readback = CreateBuffer(bytes, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
		ComPtr<ID3D12CommandAllocator> allocator;
		ComPtr<ID3D12GraphicsCommandList> list;
		CheckDX12(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
				  "Create DX12 capture allocator");
		CheckDX12(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
											  IID_PPV_ARGS(&list)),
				  "Create DX12 capture list");
		Transition(list.Get(), m_backBuffers[m_lastFrame].Get(), D3D12_RESOURCE_STATE_PRESENT,
				   D3D12_RESOURCE_STATE_COPY_SOURCE);
		D3D12_TEXTURE_COPY_LOCATION source = {}, destination = {};
		source.pResource = m_backBuffers[m_lastFrame].Get();
		source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destination.pResource = readback.Get();
		destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destination.PlacedFootprint = layout;
		list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
		Transition(list.Get(), m_backBuffers[m_lastFrame].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
				   D3D12_RESOURCE_STATE_PRESENT);
		CheckDX12(list->Close(), "Close DX12 capture list");
		ID3D12CommandList *lists[] = {list.Get()};
		m_queue->ExecuteCommandLists(1, lists);
		WaitIdle();
		// 第二步：移除 GPU 行距填充，保留图片顶行为首行的 RGBA 约定，再由公共函数输出 BMP。
		unsigned char *mapped = nullptr;
		D3D12_RANGE range = {static_cast<SIZE_T>(layout.Offset), static_cast<SIZE_T>(bytes)};
		CheckDX12(readback->Map(0, &range, reinterpret_cast<void **>(&mapped)), "Map DX12 screenshot");
		std::vector<unsigned char> pixels(static_cast<size_t>(m_width) * m_height * 4);
		for (int y = 0; y < m_height; ++y)
			memcpy(pixels.data() + static_cast<size_t>(y) * m_width * 4,
				   mapped + layout.Offset + static_cast<size_t>(y) * layout.Footprint.RowPitch,
				   static_cast<size_t>(m_width) * 4);
		D3D12_RANGE noWrite = {0, 0};
		readback->Unmap(0, &noWrite);
		Platform::WriteBMP(i_path, m_width, m_height, pixels.data(), false);
		Platform::Log("Captured DX12 frame: " + i_path.u8string());
	}
	void DX12Backend::Present(bool i_vsync)
	{
		if (m_swap && m_hasFrame)
			CheckDX12(m_swap->Present(i_vsync ? 1 : 0, 0), "Present DX12 frame");
	}
	void DX12Backend::ShutDown()
	{
		// GPU 停止引用资源以后，按依赖反序释放；本函数也支持初始化中途失败后的清理。
		std::exception_ptr waitError;
		try
		{
			WaitIdle();
		}
		catch (...)
		{
			waitError = std::current_exception();
		}
		m_meshes.clear();
		m_textures.clear();
		m_cubes.clear();
		m_pipelines.clear();
		for (auto &frame : m_frames)
		{
			frame.uploads.clear();
			frame.constants.Reset();
			frame.timing.Reset();
			frame.descriptors.Reset();
			frame.allocator.Reset();
			frame.fence = 0;
			frame.capacity = 0;
			frame.timed = false;
		}
		for (auto &buffer : m_backBuffers)
			buffer.Reset();
		m_depth.Reset();
		m_shadowColor.Reset();
		m_shadowDepth.Reset();
		m_shadowInitialized = false;
		m_shadowDescriptor = m_nullShadowDescriptor = 0;
		m_queries.Reset();
		m_list.Reset();
		m_root.Reset();
		m_rtvHeap.Reset();
		m_dsvHeap.Reset();
		m_textureHeap.Reset();
		m_swap.Reset();
		m_queue.Reset();
		m_fence.Reset();
		m_device.Reset();
		if (m_event)
		{
			CloseHandle(m_event);
			m_event = nullptr;
		}
		m_width = m_height = 0;
		m_textureCount = 0;
		m_nextFence = 1;
		m_gpuTime = -1;
		m_hasFrame = false;
		if (waitError)
			std::rethrow_exception(waitError);
	}
	std::unique_ptr<IRenderBackend> CreateDX12Backend()
	{
		return std::make_unique<DX12Backend>();
	}
} // namespace Rendering
