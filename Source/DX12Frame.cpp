#include "DX12Renderer.h"
#include "Files.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace Rendering
{
	D3D12_RESOURCE_BARRIER DX12Renderer::Transition(ID3D12Resource* i_resource, D3D12_RESOURCE_STATES i_before,
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

		const UINT frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		Frame &frame = m_frames[frameIndex];
		// 等这份 allocator 和常量用完再覆写。
		WaitFor(frame.m_fenceValue);
		std::memcpy(frame.m_mapped, &i_frame.m_constants, sizeof(FrameConstants));
		Check(frame.m_allocator->Reset(), "Reset frame allocator");
		Check(m_commands->Reset(frame.m_allocator.Get(), m_pipeline.Get()), "Reset command list");


		m_commands->SetGraphicsRootSignature(m_rootSignature.Get());
		m_commands->SetGraphicsRootConstantBufferView(0, frame.m_constants->GetGPUVirtualAddress());
		D3D12_VIEWPORT viewport = {0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0, 1};
		D3D12_RECT scissor = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
		m_commands->RSSetViewports(1, &viewport);
		m_commands->RSSetScissorRects(1, &scissor);


		D3D12_RESOURCE_BARRIER barrier =
			Transition(m_backBuffers[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commands->ResourceBarrier(1, &barrier);
		const D3D12_CPU_DESCRIPTOR_HANDLE rtv = RTV(frameIndex);
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv = DSV(frameIndex);
		m_commands->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		const float sky[] = {.06f, .08f, .12f, 1.0f};
		m_commands->ClearRenderTargetView(rtv, sky, 0, nullptr);
		m_commands->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

		m_commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commands->IASetVertexBuffers(0, 1, &m_vertexView);
		m_commands->IASetIndexBuffer(&m_indexView);
		for (size_t itemIndex = 0; itemIndex < i_frame.m_items.size(); itemIndex++)
		{
			const DrawItem &item = i_frame.m_items[itemIndex];
			m_commands->SetGraphicsRoot32BitConstants(1, sizeof(DrawConstants) / 4, &item.m_constants, 0);
			m_commands->DrawIndexedInstanced(item.m_indexCount, 1, item.m_firstIndex, 0, 0);
		}

		barrier =
			Transition(m_backBuffers[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commands->ResourceBarrier(1, &barrier);
		Check(m_commands->Close(), "Close frame commands");


		ID3D12CommandList* lists[] = {m_commands.Get()};
		m_queue->ExecuteCommandLists(1, lists);
		// Check(m_swapChain->Present(i_vsync ? 1 : 0, !i_vsync && m_allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0), "Present");
		UINT syncInterval = 0;
		if (i_vsync)
			syncInterval = 1;
		UINT presentFlags = 0;
		if (!i_vsync && m_allowTearing)
			presentFlags = DXGI_PRESENT_ALLOW_TEARING;
		Check(m_swapChain->Present(syncInterval, presentFlags), "Present");

		const UINT64 fenceValue = m_nextFence++;
		Check(m_queue->Signal(m_fence.Get(), fenceValue), "Signal frame fence");
		frame.m_fenceValue = fenceValue;
		// 截图取刚画完的这张，不取 Present 后的新 index。
		m_lastRenderedIndex = frameIndex;
		m_hasRendered = true;
	}

	void DX12Renderer::Capture(const std::filesystem::path &i_path)
	{
		if (!m_hasRendered || !m_width || !m_height)
			throw std::runtime_error("DX12: no rendered frame to capture");

		WaitIdle();
		const UINT frameIndex = m_lastRenderedIndex;
		Frame &frame = m_frames[frameIndex];
		const D3D12_RESOURCE_DESC textureDesc = m_backBuffers[frameIndex]->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		UINT64 readbackSize = 0;
		// 回读的行距有对齐，拷贝用 RowPitch。
		m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, nullptr, nullptr, &readbackSize);

		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_READBACK;
		D3D12_RESOURCE_DESC buffer = {};
		buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		buffer.Width = readbackSize;
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
		D3D12_RESOURCE_BARRIER barrier =
			Transition(m_backBuffers[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_commands->ResourceBarrier(1, &barrier);
		D3D12_TEXTURE_COPY_LOCATION source = {};
		D3D12_TEXTURE_COPY_LOCATION destination = {};
		source.pResource = m_backBuffers[frameIndex].Get();
		source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destination.pResource = readback.Get();
		destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destination.PlacedFootprint = footprint;
		m_commands->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
		barrier =
			Transition(m_backBuffers[frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
		m_commands->ResourceBarrier(1, &barrier);
		Check(m_commands->Close(), "Close capture commands");
		ID3D12CommandList* lists[] = {m_commands.Get()};
		m_queue->ExecuteCommandLists(1, lists);
		WaitIdle();

		void* mapped = nullptr;
		D3D12_RANGE range = {0, static_cast<SIZE_T>(readbackSize)};
		Check(readback->Map(0, &range, &mapped), "Map screenshot readback");
		const size_t rowSize = static_cast<size_t>(m_width) * 4;
		std::vector<unsigned char> rgba(rowSize * m_height);
		const unsigned char* sourcePixels = static_cast<unsigned char*>(mapped) + footprint.Offset;
		for (UINT y = 0; y < m_height; y++)
		{
			size_t sourceOffset = static_cast<size_t>(y) * footprint.Footprint.RowPitch;
			size_t destinationOffset = static_cast<size_t>(y) * rowSize;
			std::memcpy(rgba.data() + destinationOffset, sourcePixels + sourceOffset, rowSize);
		}
		D3D12_RANGE noWrite = {0, 0};
		readback->Unmap(0, &noWrite);
		WriteBMP(i_path, m_width, m_height, rgba.data());
	}
}
