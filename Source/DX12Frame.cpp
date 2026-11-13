#include "DX12Renderer.h"
#include "Files.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace Lite
{
	D3D12_RESOURCE_BARRIER DX12Renderer::Transition(ID3D12Resource *i_resource, D3D12_RESOURCE_STATES i_before,
													D3D12_RESOURCE_STATES i_after)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = i_resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = i_before;
		barrier.Transition.StateAfter = i_after;
		return barrier;
	}

	void DX12Renderer::Render(const FrameData &i_frame, bool i_vsync)
	{
		if (!m_width || !m_height)
			return;
		const UINT index = m_swapChain->GetCurrentBackBufferIndex();
		auto &frame = m_frames[index];
		// 1. 只等这一份资源上一次的提交，另一份仍可在 GPU 上执行。
		// allocator 和 Upload CBV 都不能在 GPU 读完之前被 CPU 覆写。
		WaitFor(frame.m_fenceValue);
		std::memcpy(frame.m_mapped, &i_frame.m_constants, sizeof(FrameConstants));
		Check(frame.m_allocator->Reset(), "Reset frame allocator");
		Check(m_commands->Reset(frame.m_allocator.Get(), m_pipeline.Get()), "Reset command list");
		// 2. Command List 先记录命令；此时尚未在 GPU 上执行。
		m_commands->SetGraphicsRootSignature(m_rootSignature.Get());
		m_commands->SetGraphicsRootConstantBufferView(0, frame.m_constants->GetGPUVirtualAddress());
		D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0, 1};
		D3D12_RECT scissor = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
		m_commands->RSSetViewports(1, &viewport);
		m_commands->RSSetScissorRects(1, &scissor);
		// Resource Barrier 明确转换用途，保证 Present 的图像可被作为渲染目标写入。
		auto barrier =
			Transition(m_backBuffers[index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commands->ResourceBarrier(1, &barrier);
		const auto rtv = RTV(index);
		const auto dsv = DSV(index);
		m_commands->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		const float sky[] = {.06f, .08f, .12f, 1.0f};
		m_commands->ClearRenderTargetView(rtv, sky, 0, nullptr);
		m_commands->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
		m_commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commands->IASetVertexBuffers(0, 1, &m_vertexView);
		m_commands->IASetIndexBuffer(&m_indexView);
		for (const auto &item : i_frame.m_items)
		{
			m_commands->SetGraphicsRoot32BitConstants(1, sizeof(DrawConstants) / 4, &item.m_constants, 0);
			m_commands->DrawIndexedInstanced(item.m_indexCount, 1, item.m_firstIndex, 0, 0);
		}
		barrier =
			Transition(m_backBuffers[index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commands->ResourceBarrier(1, &barrier);
		Check(m_commands->Close(), "Close frame commands");
		// 3. Queue 提交，Present 显示；Fence 标记这份帧资源何时可以再次使用。
		ID3D12CommandList *lists[] = {m_commands.Get()};
		m_queue->ExecuteCommandLists(1, lists);
		Check(m_swapChain->Present(i_vsync ? 1 : 0, !i_vsync && m_allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0),
			  "Present");
		const UINT64 value = m_nextFence++;
		Check(m_queue->Signal(m_fence.Get(), value), "Signal frame fence");
		frame.m_fenceValue = value;
		// Present 已推进 current index，截图必须记住刚刚画完的图像。
		m_lastRenderedIndex = index;
		m_hasRendered = true;
	}

	void DX12Renderer::Capture(const std::filesystem::path &i_path)
	{
		if (!m_hasRendered || !m_width || !m_height)
			throw std::runtime_error("DX12: no rendered frame to capture");
		WaitIdle();
		const UINT index = m_lastRenderedIndex;
		auto &frame = m_frames[index];
		const auto texture = m_backBuffers[index]->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		UINT64 bytes = 0;
		// DX12 的 readback 每行按 256 字节对齐，不能假定 RowPitch == width * 4。
		m_device->GetCopyableFootprints(&texture, 0, 1, 0, &footprint, nullptr, nullptr, &bytes);
		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_READBACK;
		D3D12_RESOURCE_DESC buffer = {};
		buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		buffer.Width = bytes;
		buffer.Height = 1;
		buffer.DepthOrArraySize = 1;
		buffer.MipLevels = 1;
		buffer.SampleDesc.Count = 1;
		buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ComPtr<ID3D12Resource> readback;
		Check(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST,
												nullptr, IID_PPV_ARGS(&readback)),
			  "Create screenshot readback");
		Check(frame.m_allocator->Reset(), "Reset capture allocator");
		Check(m_commands->Reset(frame.m_allocator.Get(), nullptr), "Reset capture commands");
		auto barrier =
			Transition(m_backBuffers[index].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_commands->ResourceBarrier(1, &barrier);
		D3D12_TEXTURE_COPY_LOCATION source = {}, destination = {};
		source.pResource = m_backBuffers[index].Get();
		source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destination.pResource = readback.Get();
		destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destination.PlacedFootprint = footprint;
		m_commands->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
		barrier =
			Transition(m_backBuffers[index].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
		m_commands->ResourceBarrier(1, &barrier);
		Check(m_commands->Close(), "Close capture commands");
		ID3D12CommandList *lists[] = {m_commands.Get()};
		m_queue->ExecuteCommandLists(1, lists);
		WaitIdle();
		void *mapped = nullptr;
		D3D12_RANGE range = {0, static_cast<SIZE_T>(bytes)};
		Check(readback->Map(0, &range, &mapped), "Map screenshot readback");
		std::vector<unsigned char> rgba(static_cast<size_t>(m_width) * m_height * 4);
		for (UINT y = 0; y < m_height; ++y)
		{
			std::memcpy(rgba.data() + static_cast<size_t>(y) * m_width * 4,
						static_cast<unsigned char *>(mapped) + footprint.Offset +
							static_cast<size_t>(y) * footprint.Footprint.RowPitch,
						static_cast<size_t>(m_width) * 4);
		}
		D3D12_RANGE noWrite = {0, 0};
		readback->Unmap(0, &noWrite);
		WriteBMP(i_path, m_width, m_height, rgba.data());
	}
} // namespace Lite
