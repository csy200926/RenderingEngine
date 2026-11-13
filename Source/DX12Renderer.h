#pragma once
#include "IRenderer.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>

namespace Rendering
{
	class DX12Renderer final : public IRenderer
	{
	public:
		~DX12Renderer() override;
		void Initialize(HWND i_window, unsigned i_width, unsigned i_height, const MeshData &i_mesh) override;
		void Resize(unsigned i_width, unsigned i_height) override;
		void Render(const FrameData &i_frame, bool i_vsync) override;
		void Capture(const std::filesystem::path &i_path) override;
		void WaitIdle() override;

	private:
		static constexpr UINT FrameCount = 2;
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		struct Frame
		{
			ComPtr<ID3D12CommandAllocator> m_allocator;
			ComPtr<ID3D12Resource> m_constants;
			ComPtr<ID3D12Resource> m_depth;
			unsigned char* m_mapped = nullptr;
			UINT64 m_fenceValue = 0;
		};

		void CreatePipeline();
		void CreateMesh(const MeshData &i_mesh);
		void CreateTargets();
		void WaitFor(UINT64 i_value);
		D3D12_CPU_DESCRIPTOR_HANDLE RTV(UINT i_index) const;
		D3D12_CPU_DESCRIPTOR_HANDLE DSV(UINT i_index) const;
		static void Check(HRESULT i_result, const char* i_operation);
		static D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* i_resource, D3D12_RESOURCE_STATES i_before,
			D3D12_RESOURCE_STATES i_after);


		ComPtr<IDXGIFactory6> m_factory;
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12CommandQueue> m_queue;
		ComPtr<IDXGISwapChain3> m_swapChain;
		ComPtr<ID3D12Fence> m_fence;

		ComPtr<ID3D12DescriptorHeap> m_rtvHeap, m_dsvHeap;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipeline;
		ComPtr<ID3D12GraphicsCommandList> m_commands;

		ComPtr<ID3D12Resource> m_vertices, m_indices;
		std::array<ComPtr<ID3D12Resource>, FrameCount> m_backBuffers;
		std::array<Frame, FrameCount> m_frames;
		D3D12_VERTEX_BUFFER_VIEW m_vertexView = {};
		D3D12_INDEX_BUFFER_VIEW m_indexView = {};

		HANDLE m_fenceEvent = nullptr;
		UINT64 m_nextFence = 1;
		UINT m_rtvStride = 0, m_dsvStride = 0;

		UINT m_width = 0, m_height = 0;
		UINT m_lastRenderedIndex = 0;
		bool m_allowTearing = false, m_hasRendered = false;
	};
}
