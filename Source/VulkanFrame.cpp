#include "VulkanRenderer.h"
#include "Files.h"
#include <cstring>
#include <limits>

namespace Rendering
{
	void VulkanRenderer::WaitFrame(const Frame &i_frame)
	{
		if (!i_frame.finished)
			return;
		VkSemaphoreWaitInfo wait{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
		wait.semaphoreCount = 1;
		wait.pSemaphores = &m_timeline;
		wait.pValues = &i_frame.finished;
		Check(vkWaitSemaphores(m_device, &wait, std::numeric_limits<uint64_t>::max()), "WaitFrameTimeline");
	}
	void VulkanRenderer::ChangeImageLayout(VkCommandBuffer i_command, VkImage i_image,
		VkImageAspectFlags i_aspect, VkImageLayout i_oldLayout, VkImageLayout i_newLayout,
		VkPipelineStageFlags2 i_sourceStage, VkAccessFlags2 i_sourceAccess,
		VkPipelineStageFlags2 i_destinationStage, VkAccessFlags2 i_destinationAccess)
	{
		VkImageMemoryBarrier2 imageBarrier = {};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageBarrier.srcStageMask = i_sourceStage;
		imageBarrier.srcAccessMask = i_sourceAccess;
		imageBarrier.dstStageMask = i_destinationStage;
		imageBarrier.dstAccessMask = i_destinationAccess;
		imageBarrier.oldLayout = i_oldLayout;
		imageBarrier.newLayout = i_newLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = i_image;
		imageBarrier.subresourceRange = {i_aspect, 0, 1, 0, 1};

		VkDependencyInfo dependency = {};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.imageMemoryBarrierCount = 1;
		dependency.pImageMemoryBarriers = &imageBarrier;
		vkCmdPipelineBarrier2(i_command, &dependency);
	}

	void VulkanRenderer::Render(const FrameData &i_data, bool i_vsync)
	{
		if (!m_width || !m_height)
			return;
		if (i_vsync != m_vsync)
		{
			m_vsync = i_vsync;
			Resize(m_width, m_height);
		}
		if (!m_swapchain)
			CreateSwapchain();
		if (!m_swapchain)
			return;

		Frame& frame = m_frames[m_current];
		// 先等这一份帧资源用完，再重写 UBO 和命令池。
		WaitFrame(frame);
		uint32_t index = 0;
		VkResult acquired = vkAcquireNextImageKHR(m_device, m_swapchain,
			std::numeric_limits<uint64_t>::max(), frame.acquired, VK_NULL_HANDLE, &index);
		if (acquired == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Resize(m_width, m_height);
			return;
		}
		if (acquired != VK_SUBOPTIMAL_KHR)
			Check(acquired, "AcquireNextImage");
		SwapImage& image = m_images[index];
		if (image.presentPending)
		{
			Check(vkWaitForFences(m_device, 1, &image.presentFence, true, UINT64_MAX), "WaitImagePresentation");
			image.presentPending = false;
		}
		Check(vkResetFences(m_device, 1, &image.presentFence), "ResetPresentFence");

		std::memcpy(frame.uniform.mapped, &i_data.m_constants, sizeof(FrameConstants));
		Check(vkResetCommandPool(m_device, frame.pool, 0), "ResetCommandPool");
		VkCommandBufferBeginInfo begin = {};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		Check(vkBeginCommandBuffer(frame.command, &begin), "BeginCommandBuffer");

		// 清屏丢弃旧内容；交换链图像仍由 acquire 信号量保护。
		ChangeImageLayout(frame.command, image.handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
		ChangeImageLayout(frame.command, frame.depth.handle, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

		VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
		color.imageView = image.view;
		color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color.clearValue.color = {{.06f, .08f, .12f, 1}};
		VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
		depth.imageView = frame.depth.view;
		depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth.clearValue.depthStencil.depth = 1;
		VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
		rendering.renderArea.extent = m_extent;
		rendering.layerCount = 1;
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachments = &color;
		rendering.pDepthAttachment = &depth;
		vkCmdBeginRendering(frame.command, &rendering);
		vkCmdBindPipeline(frame.command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
		vkCmdBindDescriptorSets(frame.command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout,
			0, 1, &frame.descriptor, 0, nullptr);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(frame.command, 0, 1, &m_vertices.handle, &offset);
		vkCmdBindIndexBuffer(frame.command, m_indices.handle, 0, VK_INDEX_TYPE_UINT32);
		VkViewport viewport{0, 0, float(m_extent.width), float(m_extent.height), 0, 1};
		VkRect2D scissor{{0, 0}, m_extent};
		vkCmdSetViewport(frame.command, 0, 1, &viewport);
		vkCmdSetScissor(frame.command, 0, 1, &scissor);

		for (size_t itemIndex = 0; itemIndex < i_data.m_items.size(); itemIndex++)
		{
			const DrawItem& item = i_data.m_items[itemIndex];
			vkCmdPushConstants(frame.command, m_layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(DrawConstants), &item.m_constants);
			vkCmdDrawIndexed(frame.command, item.m_indexCount, 1, item.m_firstIndex, 0, 0);
		}
		vkCmdEndRendering(frame.command);

		if (m_canCapture)
		{
			// 颜色写完再拷贝，各帧使用自己的截图 buffer。
			ChangeImageLayout(frame.command, image.handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT,
					VK_ACCESS_2_TRANSFER_READ_BIT);
			VkBufferImageCopy copy{};
			copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			copy.imageExtent = {m_extent.width, m_extent.height, 1};
			vkCmdCopyImageToBuffer(frame.command, image.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								   frame.readback.handle, 1, &copy);
			VkBufferMemoryBarrier2 host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
			host.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			host.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			host.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
			host.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
			host.srcQueueFamilyIndex = host.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			host.buffer = frame.readback.handle;
			host.size = VK_WHOLE_SIZE;
			VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			dependency.bufferMemoryBarrierCount = 1;
			dependency.pBufferMemoryBarriers = &host;
			vkCmdPipelineBarrier2(frame.command, &dependency);
		}
		ChangeImageLayout(frame.command, image.handle, VK_IMAGE_ASPECT_COLOR_BIT,
				m_canCapture ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				m_canCapture ? VK_PIPELINE_STAGE_2_COPY_BIT : VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				m_canCapture ? VK_ACCESS_2_TRANSFER_READ_BIT : VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_NONE, 0);
		Check(vkEndCommandBuffer(frame.command), "EndCommandBuffer");

		VkSemaphoreSubmitInfo wait{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
		wait.semaphore = frame.acquired;
		wait.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		VkSemaphoreSubmitInfo signals[2] = {};
		signals[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signals[0].semaphore = image.presentDone;
		signals[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		m_serial++;
		signals[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signals[1].semaphore = m_timeline;
		signals[1].value = m_serial;
		signals[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		VkCommandBufferSubmitInfo command{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
		command.commandBuffer = frame.command;
		VkSubmitInfo2 submit{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
		submit.waitSemaphoreInfoCount = 1;
		submit.pWaitSemaphoreInfos = &wait;
		submit.commandBufferInfoCount = 1;
		submit.pCommandBufferInfos = &command;
		submit.signalSemaphoreInfoCount = 2;
		submit.pSignalSemaphoreInfos = signals;
		Check(vkQueueSubmit2(m_queue, 1, &submit, VK_NULL_HANDLE), "QueueSubmit2");
		frame.finished = m_serial;
		m_latest = m_current;
		m_hasFrame = true;
		m_current = (m_current + 1) % 2;

		// 每张交换链图像单独一个 binary semaphore，present 等它。
		VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
		VkSwapchainPresentFenceInfoEXT presentFence{VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT};
		presentFence.swapchainCount = 1;
		presentFence.pFences = &image.presentFence;
		present.pNext = &presentFence;
		present.waitSemaphoreCount = 1;
		present.pWaitSemaphores = &image.presentDone;
		present.swapchainCount = 1;
		present.pSwapchains = &m_swapchain;
		present.pImageIndices = &index;
		VkResult result = vkQueuePresentKHR(m_queue, &present);
		// OUT_OF_DATE/SURFACE_LOST 仍入队，必须等 fence；OOM 则保证完全没有入队。
		image.presentPending = result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR ||
							   result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_SURFACE_LOST_KHR ||
							   result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT;
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || acquired == VK_SUBOPTIMAL_KHR)
		{
			if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR)
				Check(result, "QueuePresent");
			Resize(m_width, m_height);
		}
		else
			Check(result, "QueuePresent");
	}

	void VulkanRenderer::Capture(const std::filesystem::path &i_path)
	{
		if (!m_canCapture)
			throw std::runtime_error("Screenshot unavailable: swapchain lacks TRANSFER_SRC support");
		if (!m_hasFrame)
			throw std::runtime_error("Screenshot unavailable: render a frame first");

		Frame& frame = m_frames[m_latest];
		WaitFrame(frame);
		size_t bytes = size_t(m_extent.width) * m_extent.height * 4;
		std::vector<unsigned char> rgba(bytes);
		std::memcpy(rgba.data(), frame.readback.mapped, bytes);
		if (m_format == VK_FORMAT_B8G8R8A8_UNORM)
		{
			for (size_t pixelIndex = 0; pixelIndex < bytes; pixelIndex += 4)
			{
				std::swap(rgba[pixelIndex], rgba[pixelIndex + 2]);
			}
		}
		WriteBMP(i_path, m_extent.width, m_extent.height, rgba.data());
	}
}
