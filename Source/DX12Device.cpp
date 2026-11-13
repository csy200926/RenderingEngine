#include "DX12Renderer.h"
#include "Files.h"
#include <cstdio>
#include <stdexcept>

namespace Lite
{
	void DX12Renderer::Check(HRESULT i_result, const char *i_operation)
	{
		if (FAILED(i_result))
		{
			char code[32];
			std::snprintf(code, sizeof(code), " (HRESULT 0x%08lX)", static_cast<unsigned long>(i_result));
			throw std::runtime_error(std::string("DX12: ") + i_operation + code);
		}
	}

	std::unique_ptr<IRenderer> CreateDX12Renderer()
	{
		return std::make_unique<DX12Renderer>();
	}

	DX12Renderer::~DX12Renderer()
	{
		// 析构不能抛异常；设备移除时也要继续释放 CPU 句柄。
		try
		{
			WaitIdle();
		}
		catch (...)
		{
		}
		for (auto &frame : m_frames)
		{
			if (frame.m_mapped)
				frame.m_constants->Unmap(0, nullptr);
		}
		if (m_fenceEvent)
			CloseHandle(m_fenceEvent);
	}

	void DX12Renderer::Initialize(HWND i_window, unsigned i_width, unsigned i_height, const MeshData &i_mesh)
	{
		if (!i_window || !i_width || !i_height)
			throw std::runtime_error("DX12: invalid initial window size");
		m_width = i_width;
		m_height = i_height;
		// 1. Factory 枚举显卡；Device 是创建 GPU 资源的入口。
		Check(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory)), "CreateDXGIFactory2");
		for (UINT index = 0;; ++index)
		{
			ComPtr<IDXGIAdapter1> adapter;
			HRESULT result = m_factory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
																   IID_PPV_ARGS(&adapter));
			if (result == DXGI_ERROR_NOT_FOUND)
				break;
			Check(result, "EnumAdapterByGpuPreference");
			DXGI_ADAPTER_DESC1 desc = {};
			Check(adapter->GetDesc1(&desc), "GetDesc1");
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
				break;
		}
		if (!m_device)
			throw std::runtime_error("DX12: no compatible hardware adapter");
		// 2. Queue 顺序执行提交的 Command List。Fence 让 CPU 知道 GPU 执行到了哪里。
		D3D12_COMMAND_QUEUE_DESC queue = {};
		queue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		Check(m_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&m_queue)), "CreateCommandQueue");
		Check(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "CreateFence");
		m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (!m_fenceEvent)
			Check(HRESULT_FROM_WIN32(GetLastError()), "CreateEvent");
		BOOL tearing = FALSE;
		if (SUCCEEDED(m_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing))))
		{
			m_allowTearing = tearing != FALSE;
		}
		// 3. 两个交换链图像轮流显示；其索引也选择本帧的独立资源。
		DXGI_SWAP_CHAIN_DESC1 swap = {};
		swap.Width = m_width;
		swap.Height = m_height;
		swap.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap.SampleDesc.Count = 1;
		swap.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap.BufferCount = FrameCount;
		// SEQUENTIAL 保留 Present 后的图像内容，供 Capture 读取上一帧。
		swap.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swap.Flags = m_allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		ComPtr<IDXGISwapChain1> swapChain;
		Check(m_factory->CreateSwapChainForHwnd(m_queue.Get(), i_window, &swap, nullptr, nullptr, &swapChain),
			  "CreateSwapChainForHwnd");
		Check(swapChain.As(&m_swapChain), "Query swap chain 3");
		Check(m_factory->MakeWindowAssociation(i_window, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");
		D3D12_DESCRIPTOR_HEAP_DESC heap = {};
		heap.NumDescriptors = FrameCount;
		heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		Check(m_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&m_rtvHeap)), "Create RTV heap");
		heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		Check(m_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&m_dsvHeap)), "Create DSV heap");
		m_rtvStride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_dsvStride = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		for (auto &frame : m_frames)
		{
			Check(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.m_allocator)),
				  "CreateCommandAllocator");
		}
		CreatePipeline();
		CreateMesh(i_mesh);
		CreateTargets();
		Check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frames[0].m_allocator.Get(),
										  m_pipeline.Get(), IID_PPV_ARGS(&m_commands)),
			  "CreateCommandList");
		Check(m_commands->Close(), "Close initial command list");
		Log("DX12 initialized: two frames in flight");
	}

	void DX12Renderer::WaitFor(UINT64 i_value)
	{
		const UINT64 completed = m_fence->GetCompletedValue();
		// 设备移除会把完成值置为 UINT64_MAX，它不表示所有工作成功完成。
		if (completed == UINT64_MAX)
			Check(m_device->GetDeviceRemovedReason(), "Device removed while waiting");
		if (!i_value || completed >= i_value)
			return;
		Check(m_fence->SetEventOnCompletion(i_value, m_fenceEvent), "SetEventOnCompletion");
		if (WaitForSingleObject(m_fenceEvent, INFINITE) != WAIT_OBJECT_0)
		{
			throw std::runtime_error("DX12: fence wait failed");
		}
	}

	void DX12Renderer::WaitIdle()
	{
		if (!m_queue || !m_fence || !m_fenceEvent)
			return;
		const UINT64 value = m_nextFence++;
		Check(m_queue->Signal(m_fence.Get(), value), "Signal idle fence");
		WaitFor(value);
	}
} // namespace Lite
