#include "VulkanRenderer.h"
#include "Files.h"
#include <algorithm>
#include <limits>

namespace Lite
{
	uint32_t VulkanRenderer::MemoryType(uint32_t bits, VkMemoryPropertyFlags flags)
	{
		VkPhysicalDeviceMemoryProperties properties{};
		vkGetPhysicalDeviceMemoryProperties(m_physical, &properties);
		for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
			if ((bits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags)
				return i;
		throw std::runtime_error("Required Vulkan memory type is unavailable");
	}
	void VulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, Buffer &buffer)
	{
		VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		info.size = size;
		info.usage = usage;
		Check(vkCreateBuffer(m_device, &info, nullptr, &buffer.handle), "CreateBuffer");
		VkMemoryRequirements requirements{};
		vkGetBufferMemoryRequirements(m_device, buffer.handle, &requirements);
		VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		allocate.allocationSize = requirements.size;
		allocate.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
																			   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		Check(vkAllocateMemory(m_device, &allocate, nullptr, &buffer.memory), "AllocateBufferMemory");
		Check(vkBindBufferMemory(m_device, buffer.handle, buffer.memory, 0), "BindBufferMemory");
		Check(vkMapMemory(m_device, buffer.memory, 0, size, 0, &buffer.mapped), "MapMemory");
	}
	void VulkanRenderer::DestroyBuffer(Buffer &buffer)
	{
		if (buffer.mapped)
			vkUnmapMemory(m_device, buffer.memory);
		if (buffer.handle)
			vkDestroyBuffer(m_device, buffer.handle, nullptr);
		if (buffer.memory)
			vkFreeMemory(m_device, buffer.memory, nullptr);
		buffer = {};
	}
	void VulkanRenderer::CreateDepth(Image &image)
	{
		VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_D32_SFLOAT;
		info.extent = {m_extent.width, m_extent.height, 1};
		info.mipLevels = info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		Check(vkCreateImage(m_device, &info, nullptr, &image.handle), "CreateDepthImage");
		VkMemoryRequirements requirements{};
		vkGetImageMemoryRequirements(m_device, image.handle, &requirements);
		VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		allocate.allocationSize = requirements.size;
		allocate.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		Check(vkAllocateMemory(m_device, &allocate, nullptr, &image.memory), "AllocateDepthMemory");
		Check(vkBindImageMemory(m_device, image.handle, image.memory, 0), "BindDepthMemory");
		VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		view.image = image.handle;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = info.format;
		view.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
		Check(vkCreateImageView(m_device, &view, nullptr, &image.view), "DepthView");
	}
	void VulkanRenderer::DestroyImage(Image &image)
	{
		if (image.view)
			vkDestroyImageView(m_device, image.view, nullptr);
		if (image.handle)
			vkDestroyImage(m_device, image.handle, nullptr);
		if (image.memory)
			vkFreeMemory(m_device, image.memory, nullptr);
		image = {};
	}
	void VulkanRenderer::DestroySwapchain()
	{
		if (!m_device)
			return;
		if (m_pipeline)
			vkDestroyPipeline(m_device, m_pipeline, nullptr);
		m_pipeline = VK_NULL_HANDLE;
		for (auto &image : m_images)
		{
			if (image.view)
				vkDestroyImageView(m_device, image.view, nullptr);
			if (image.presentDone)
				vkDestroySemaphore(m_device, image.presentDone, nullptr);
			if (image.presentFence)
				vkDestroyFence(m_device, image.presentFence, nullptr);
		}
		m_images.clear();
		for (auto &frame : m_frames)
		{
			DestroyImage(frame.depth);
			DestroyBuffer(frame.readback);
		}
		if (m_swapchain)
			vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
		m_hasFrame = false;
	}
	void VulkanRenderer::Resize(unsigned width, unsigned height)
	{
		m_width = width;
		m_height = height;
		if (!m_device)
			return;
		// resize 很少发生，直接等整个设备空闲，确保旧呈现/深度/截图资源都不再被使用。
		WaitIdle();
		DestroySwapchain();
		if (width && height)
			CreateSwapchain();
	}
	void VulkanRenderer::CreateSwapchain()
	{
		if (!m_width || !m_height)
			return;
		VkSurfaceCapabilitiesKHR capabilities{};
		Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical, m_surface, &capabilities), "SurfaceCapabilities");
		uint32_t count = 0;
		Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &count, nullptr), "SurfaceFormats");
		std::vector<VkSurfaceFormatKHR> formats(count);
		Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &count, formats.data()), "SurfaceFormats");
		VkSurfaceFormatKHR chosen{};
		for (auto format : formats)
			if ((format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_R8G8B8A8_UNORM) &&
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				chosen = format;
				break;
			}
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
			chosen = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		if (chosen.format == VK_FORMAT_UNDEFINED)
			throw std::runtime_error("Need RGBA8/BGRA8 UNORM swapchain with SRGB_NONLINEAR color space");
		m_format = chosen.format;
		VkFormatProperties colorProperties{};
		vkGetPhysicalDeviceFormatProperties(m_physical, m_format, &colorProperties);
		if (!(colorProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
			throw std::runtime_error("Swapchain format cannot be used as color attachment");
		if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
			throw std::runtime_error("Surface cannot be a color attachment");
		m_canCapture = (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
		if (!m_canCapture)
			Log("Vulkan screenshot unavailable: surface lacks TRANSFER_SRC usage");
		m_extent = capabilities.currentExtent;
		if (m_extent.width == std::numeric_limits<uint32_t>::max())
		{
			m_extent.width = std::clamp(m_width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			m_extent.height =
				std::clamp(m_height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}
		if (!m_extent.width || !m_extent.height)
			return;
		Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical, m_surface, &count, nullptr), "PresentModes");
		std::vector<VkPresentModeKHR> modes(count);
		Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical, m_surface, &count, modes.data()), "PresentModes");
		VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
		if (!m_vsync)
		{
			if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end())
				mode = VK_PRESENT_MODE_MAILBOX_KHR;
			else if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != modes.end())
				mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
		VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
		info.surface = m_surface;
		info.minImageCount = std::max(2u, capabilities.minImageCount);
		if (capabilities.maxImageCount)
			info.minImageCount = std::min(info.minImageCount, capabilities.maxImageCount);
		info.imageFormat = m_format;
		info.imageColorSpace = chosen.colorSpace;
		info.imageExtent = m_extent;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (m_canCapture ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
		info.preTransform = capabilities.currentTransform;
		for (auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
						   VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
			if (capabilities.supportedCompositeAlpha & alpha)
			{
				info.compositeAlpha = alpha;
				break;
			}
		info.presentMode = mode;
		info.clipped = true;
		Check(vkCreateSwapchainKHR(m_device, &info, nullptr, &m_swapchain), "CreateSwapchain");
		Check(vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr), "SwapchainImages");
		std::vector<VkImage> images(count);
		Check(vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, images.data()), "SwapchainImages");
		m_images.resize(count);
		for (uint32_t i = 0; i < count; ++i)
		{
			auto &image = m_images[i];
			image.handle = images[i];
			VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			view.image = image.handle;
			view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view.format = m_format;
			view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			Check(vkCreateImageView(m_device, &view, nullptr, &image.view), "SwapchainView");
			VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
			Check(vkCreateSemaphore(m_device, &semaphore, nullptr, &image.presentDone), "PresentSemaphore");
			VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
			Check(vkCreateFence(m_device, &fence, nullptr, &image.presentFence), "PresentFence");
		}
		for (auto &frame : m_frames)
		{
			CreateDepth(frame.depth);
			if (m_canCapture)
				CreateBuffer(VkDeviceSize(m_extent.width) * m_extent.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							 frame.readback);
		}
		CreatePipeline();
	}
	void VulkanRenderer::CreatePipeline()
	{
		if (!m_layout)
		{
			VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
									 sizeof(DrawConstants)};
			VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
			layout.setLayoutCount = 1;
			layout.pSetLayouts = &m_descriptorLayout;
			layout.pushConstantRangeCount = 1;
			layout.pPushConstantRanges = &push;
			Check(vkCreatePipelineLayout(m_device, &layout, nullptr, &m_layout), "PipelineLayout");
		}
		VkShaderModule vertex = VK_NULL_HANDLE, fragment = VK_NULL_HANDLE;
		try
		{
			auto shader = [&](const char *name, VkShaderModule &module)
			{
				auto bytes = ReadBytes(ShaderPath(name));
				if (bytes.empty() || bytes.size() % 4)
					throw std::runtime_error("Invalid SPIR-V file");
				std::vector<uint32_t> words(bytes.size() / 4);
				std::copy(bytes.begin(), bytes.end(), reinterpret_cast<unsigned char *>(words.data()));
				VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
				info.codeSize = bytes.size();
				info.pCode = words.data();
				Check(vkCreateShaderModule(m_device, &info, nullptr, &module), "ShaderModule");
			};
			shader("Basic.vert.spv", vertex);
			shader("Basic.frag.spv", fragment);
			VkPipelineShaderStageCreateInfo stages[2]{};
			stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						 nullptr,
						 0,
						 VK_SHADER_STAGE_VERTEX_BIT,
						 vertex,
						 "VSMain",
						 nullptr};
			stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						 nullptr,
						 0,
						 VK_SHADER_STAGE_FRAGMENT_BIT,
						 fragment,
						 "PSMain",
						 nullptr};
			VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
			VkVertexInputAttributeDescription attributes[] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
															  {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
															  {2, 0, VK_FORMAT_R32G32_SFLOAT, 24}};
			VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
			input.vertexBindingDescriptionCount = 1;
			input.pVertexBindingDescriptions = &binding;
			input.vertexAttributeDescriptionCount = 3;
			input.pVertexAttributeDescriptions = attributes;
			VkPipelineInputAssemblyStateCreateInfo assembly{
				VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
			assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
			viewport.viewportCount = viewport.scissorCount = 1;
			VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
			raster.polygonMode = VK_POLYGON_MODE_FILL;
			raster.cullMode = VK_CULL_MODE_NONE;
			raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			raster.lineWidth = 1;
			VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
			samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
			depth.depthTestEnable = depth.depthWriteEnable = true;
			depth.depthCompareOp = VK_COMPARE_OP_LESS;
			VkPipelineColorBlendAttachmentState attachment{};
			attachment.colorWriteMask = 15;
			VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
			blend.attachmentCount = 1;
			blend.pAttachments = &attachment;
			VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
			VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
			dynamic.dynamicStateCount = 2;
			dynamic.pDynamicStates = dynamicStates;
			// Dynamic Rendering 在 pipeline 声明 attachment 格式，不创建 RenderPass/Framebuffer。
			VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
			rendering.colorAttachmentCount = 1;
			rendering.pColorAttachmentFormats = &m_format;
			rendering.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
			VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
			pipeline.pNext = &rendering;
			pipeline.stageCount = 2;
			pipeline.pStages = stages;
			pipeline.pVertexInputState = &input;
			pipeline.pInputAssemblyState = &assembly;
			pipeline.pViewportState = &viewport;
			pipeline.pRasterizationState = &raster;
			pipeline.pMultisampleState = &samples;
			pipeline.pDepthStencilState = &depth;
			pipeline.pColorBlendState = &blend;
			pipeline.pDynamicState = &dynamic;
			pipeline.layout = m_layout;
			Check(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &m_pipeline),
				  "GraphicsPipeline");
		}
		catch (...)
		{
			if (vertex)
				vkDestroyShaderModule(m_device, vertex, nullptr);
			if (fragment)
				vkDestroyShaderModule(m_device, fragment, nullptr);
			throw;
		}
		vkDestroyShaderModule(m_device, vertex, nullptr);
		vkDestroyShaderModule(m_device, fragment, nullptr);
	}
} // namespace Lite
