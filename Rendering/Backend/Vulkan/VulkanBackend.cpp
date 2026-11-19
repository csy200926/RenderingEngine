#include "Platform/Log.h"
#include "Platform/Paths.h"
#include "Rendering/Backend/IRenderBackend.h"
#include "Rendering/TextureManager.h"
#include <windows.h>
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "ThirdParty/Vulkan/Include/vulkan/vulkan.h"
#include "VulkanFunctions.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <gtc/matrix_inverse.hpp>
#include <gtc/matrix_transform.hpp>
#include <limits>
#include <map>
#include <stdexcept>

namespace Rendering
{
	static void CheckVK(VkResult result, const char *operation)
	{
		if (result != VK_SUCCESS)
			throw std::runtime_error(std::string(operation) + ": VkResult=" + std::to_string(result));
	}
	// Constant for the whole frame: written once, bound as a plain uniform buffer.
	struct alignas(16) FrameConstants
	{
		glm::mat4 m_view, m_projection, m_spriteProjection, m_lightViewProjection;
		glm::vec4 m_viewPosition, m_sunDirection, m_sunColor;
		glm::vec4 m_pointPosition[4], m_pointColor[4], m_pointAttenuation[4];
		glm::vec4 m_shadowParams;
	};
	// Changes per object: one array element each, read from a storage buffer by index.
	struct alignas(16) ObjectConstants
	{
		glm::mat4 m_model, m_normal;
		glm::vec4 m_color, m_material, m_surface;
	};
	// 物体编号改走实例顶点流，这里只传本次绘制的模式。
	struct DrawPush
	{
		uint32_t m_mode;
	};
	static_assert(sizeof(DrawPush) == 4, "Must match DrawData in Default.hlsl");
	static_assert(sizeof(FrameConstants) == 512, "Must match PerFrame in Default.hlsl");
	static_assert(sizeof(ObjectConstants) == 176, "Must match ObjectData in Default.hlsl");
	class VulkanBackend final : public IRenderBackend
	{
	  public:
		~VulkanBackend() override { ShutDown(); }
		void Initilize(void *window, int width, int height, int samples, bool warp) override;
		void Resize(int width, int height) override;
		void Render(const RenderFrame &frame) override;
		void Present(bool vsync) override;
		void CaptureFrame(const std::filesystem::path &path) override;
		void ShutDown() override;
		void SetProfiling(bool enabled) override
		{
			m_profiling = enabled;
			if (!enabled)
				m_gpuTime = -1;
		}
		double GetGPUTimeMilliseconds() const override { return m_gpuTime; }

	  private:
#define DECLARE_VK(name) PFN_##name name = nullptr;
		VULKAN_INSTANCE_FUNCTIONS(DECLARE_VK)
		VULKAN_DEVICE_FUNCTIONS(DECLARE_VK)
#undef DECLARE_VK
		struct Buffer
		{
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkDeviceSize size = 0;
		};
		struct Image
		{
			VkImage image = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
		};
		struct MeshBuffers
		{
			Buffer vertices, indices;
			uint64_t revision = 0;
			bool terrain = false;
			std::weak_ptr<Mesh> owner;
		};
		HMODULE m_loader = nullptr;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_gpu = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_properties = {};
		VkPhysicalDeviceMemoryProperties m_memory = {};
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_queue = VK_NULL_HANDLE;
		uint32_t m_family = 0, m_timestampBits = 0;
		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
		VkFormat m_format = VK_FORMAT_UNDEFINED, m_depthFormat = VK_FORMAT_D32_SFLOAT;
		VkExtent2D m_extent = {};
		std::vector<VkImage> m_swapImages;
		uint32_t m_imageIndex = 0;
		Image m_color, m_depth, m_blackCube, m_shadow, m_shadowDepth;
		VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		bool m_shadowInitialized = false;
		VkRenderPass m_shadowPass = VK_NULL_HANDLE;
		VkFramebuffer m_shadowFramebuffer = VK_NULL_HANDLE;
		VkCommandPool m_commandPool = VK_NULL_HANDLE;
		VkCommandBuffer m_commands = VK_NULL_HANDLE;
		VkFence m_fence = VK_NULL_HANDLE;
		VkSemaphore m_acquired = VK_NULL_HANDLE, m_finished = VK_NULL_HANDLE;
		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
		// 三种布局共用一个 PipelineLayout。Set 的内容不变时，跨帧也能继续用。
		VkDescriptorSetLayout m_descriptorLayouts[3] = {};
		VkDescriptorSet m_sceneSet = VK_NULL_HANDLE, m_passSets[2] = {};
		VkDescriptorSet m_boundMaterial = VK_NULL_HANDLE;
		std::array<VkImageView, 3> m_passViews[2] = {};
		std::vector<VkDescriptorPool> m_materialPools;
		std::map<std::array<uint64_t, 3>, VkDescriptorSet> m_materialSets;
		uint32_t m_newMaterialSets = 0, m_passUpdates = 0;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_pipelines[5] = {};
		VkSampler m_sampler = VK_NULL_HANDLE, m_cubeSampler = VK_NULL_HANDLE;
		VkQueryPool m_queries = VK_NULL_HANDLE;
		Buffer m_frameConstants, m_objects, m_primitiveIds;
		FrameConstants *m_frameData = nullptr;
		ObjectConstants *m_objectData = nullptr;
		uint32_t m_drawIndex = 0;
		std::map<uint64_t, MeshBuffers> m_meshes;
		std::map<uint64_t, Image> m_textures, m_cubes;
		int m_width = 0, m_height = 0;
		bool m_vsync = true, m_profiling = false, m_pending = false, m_rendered = false, m_resize = false;
		bool m_rayQuery = false;
		double m_gpuTime = -1;
		static constexpr uint32_t MaterialsPerPool = 256;
		static constexpr uint32_t MaxObjects = 65536;
		static constexpr uint32_t ShadowSize = 2048;
		uint32_t MemoryType(uint32_t bits, VkMemoryPropertyFlags flags);
		Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void *data = nullptr);
		Image CreateImage(int width, int height, uint32_t layers, VkFormat format, VkImageUsageFlags usage,
						  bool cube = false, uint32_t mipLevels = 1);
		void DestroyBuffer(Buffer &buffer);
		void DestroyImage(Image &image);
		VkCommandBuffer BeginUpload();
		void EndUpload(VkCommandBuffer commands);
		void Barrier(VkCommandBuffer commands, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
					 VkAccessFlags source, VkAccessFlags target, VkPipelineStageFlags sourceStage,
					 VkPipelineStageFlags targetStage, uint32_t layers = 1, uint32_t mipLevels = 1);
		Image UploadImage(int width, int height, uint32_t layers, const unsigned char *pixels);
		MeshBuffers &GetMesh(const MeshPtr &mesh);
		Image &GetTexture(const TexturePtr &texture);
		Image &GetCube(const TextureCubePtr &cube);
		void CreateSwapchain();
		void DestroySwapchain();
		void CreatePipelines();
		void CreateShadowMap();
		void CreateDescriptors();
		void UpdatePassDescriptors(const RenderFrame &i_frame);
		VkDescriptorSet GetMaterialDescriptors(const MaterialPtr &i_material);
		VkShaderModule LoadShader(const char *path);
		ObjectConstants MakeObject(const DrawItem &item);
		void WriteConstants(const RenderFrame &frame);
		void Draw(const DrawItem &item, int mode, uint32_t object);
	};

	uint32_t VulkanBackend::MemoryType(uint32_t bits, VkMemoryPropertyFlags flags)
	{
		for (uint32_t i = 0; i < m_memory.memoryTypeCount; ++i)
			if ((bits & (1u << i)) && (m_memory.memoryTypes[i].propertyFlags & flags) == flags)
				return i;
		throw std::runtime_error("Vulkan: no compatible memory type");
	}
	VulkanBackend::Buffer VulkanBackend::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void *data)
	{
		// 第一版网格、常量和 staging 都用 coherent CPU 可见内存：写入后无需手工 flush。
		// 图像仍放 device-local 显存；静态网格后续可改 staging -> device-local，接口不变。
		Buffer result;
		result.size = std::max<VkDeviceSize>(size, 4);
		VkBufferCreateInfo info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		info.size = result.size;
		info.usage = usage;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CheckVK(vkCreateBuffer(m_device, &info, nullptr, &result.buffer), "Create buffer");
		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(m_device, result.buffer, &requirements);
		VkMemoryAllocateInfo allocation = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		allocation.allocationSize = requirements.size;
		allocation.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
																				 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		CheckVK(vkAllocateMemory(m_device, &allocation, nullptr, &result.memory), "Allocate buffer memory");
		CheckVK(vkBindBufferMemory(m_device, result.buffer, result.memory, 0), "Bind buffer memory");
		if (data && size)
		{
			void *mapped;
			CheckVK(vkMapMemory(m_device, result.memory, 0, size, 0, &mapped), "Map upload buffer");
			std::memcpy(mapped, data, static_cast<size_t>(size));
			vkUnmapMemory(m_device, result.memory);
		}
		return result;
	}
	VulkanBackend::Image VulkanBackend::CreateImage(int width, int height, uint32_t layers, VkFormat format,
													VkImageUsageFlags usage, bool cube, uint32_t mipLevels)
	{
		Image result;
		VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		info.flags = cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = format;
		info.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
		info.mipLevels = mipLevels;
		info.arrayLayers = layers;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = usage;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CheckVK(vkCreateImage(m_device, &info, nullptr, &result.image), "Create image");
		VkMemoryRequirements requirements;
		vkGetImageMemoryRequirements(m_device, result.image, &requirements);
		VkMemoryAllocateInfo allocation = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		allocation.allocationSize = requirements.size;
		allocation.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CheckVK(vkAllocateMemory(m_device, &allocation, nullptr, &result.memory), "Allocate image memory");
		CheckVK(vkBindImageMemory(m_device, result.image, result.memory, 0), "Bind image memory");
		VkImageViewCreateInfo view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		view.image = result.image;
		view.viewType = cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.subresourceRange = {static_cast<VkImageAspectFlags>(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
																	 ? VK_IMAGE_ASPECT_DEPTH_BIT
																	 : VK_IMAGE_ASPECT_COLOR_BIT),
								 0, mipLevels, 0, layers};
		CheckVK(vkCreateImageView(m_device, &view, nullptr, &result.view), "Create image view");
		return result;
	}
	void VulkanBackend::DestroyBuffer(Buffer &buffer)
	{
		if (buffer.buffer)
			vkDestroyBuffer(m_device, buffer.buffer, nullptr);
		if (buffer.memory)
			vkFreeMemory(m_device, buffer.memory, nullptr);
		buffer = {};
	}
	void VulkanBackend::DestroyImage(Image &image)
	{
		if (image.view)
			vkDestroyImageView(m_device, image.view, nullptr);
		if (image.image)
			vkDestroyImage(m_device, image.image, nullptr);
		if (image.memory)
			vkFreeMemory(m_device, image.memory, nullptr);
		image = {};
	}
	VkCommandBuffer VulkanBackend::BeginUpload()
	{
		VkCommandBufferAllocateInfo allocation = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocation.commandPool = m_commandPool;
		allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocation.commandBufferCount = 1;
		VkCommandBuffer commands;
		CheckVK(vkAllocateCommandBuffers(m_device, &allocation, &commands), "Allocate upload commands");
		VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		CheckVK(vkBeginCommandBuffer(commands, &begin), "Begin upload");
		return commands;
	}
	void VulkanBackend::EndUpload(VkCommandBuffer commands)
	{
		CheckVK(vkEndCommandBuffer(commands), "End upload");
		VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &commands;
		CheckVK(vkQueueSubmit(m_queue, 1, &submit, VK_NULL_HANDLE), "Submit upload");
		CheckVK(vkQueueWaitIdle(m_queue), "Wait upload");
		vkFreeCommandBuffers(m_device, m_commandPool, 1, &commands);
	}
	void VulkanBackend::Barrier(VkCommandBuffer commands, VkImage image, VkImageLayout oldLayout,
								VkImageLayout newLayout, VkAccessFlags source, VkAccessFlags target,
								VkPipelineStageFlags sourceStage, VkPipelineStageFlags targetStage, uint32_t layers,
								uint32_t mipLevels)
	{
		VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		barrier.srcAccessMask = source;
		barrier.dstAccessMask = target;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, layers};
		vkCmdPipelineBarrier(commands, sourceStage, targetStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}
	VulkanBackend::Image VulkanBackend::UploadImage(int width, int height, uint32_t layers, const unsigned char *pixels)
	{
		if (width < 1 || height < 1 || !layers || !pixels)
			throw std::runtime_error("Vulkan: invalid upload image");
		// 上传时一次生成完整 mip 链，缩小时用低分辨率层，减少远处纹理闪烁。
		// CPU box filter 不要求设备支持线性 blit，也不在每帧重新生成。
		// 每个 mip 的六层立方体仍按 +X,-X,+Y,-Y,+Z,-Z 连续存储。
		size_t baseSize = static_cast<size_t>(width) * height * layers * 4;
		std::vector<unsigned char> upload(pixels, pixels + baseSize);
		std::vector<VkBufferImageCopy> regions;
		uint32_t mipLevels = 0;
		int mipWidth = width, mipHeight = height;
		size_t mipOffset = 0;
		for (;;)
		{
			VkBufferImageCopy region = {};
			region.bufferOffset = mipOffset;
			region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, mipLevels++, 0, layers};
			region.imageExtent = {static_cast<uint32_t>(mipWidth), static_cast<uint32_t>(mipHeight), 1};
			regions.push_back(region);
			if (mipWidth == 1 && mipHeight == 1)
				break;
			int nextWidth = std::max(1, mipWidth / 2), nextHeight = std::max(1, mipHeight / 2);
			size_t nextOffset = upload.size();
			upload.resize(nextOffset + static_cast<size_t>(nextWidth) * nextHeight * layers * 4);
			for (uint32_t layer = 0; layer < layers; ++layer)
				for (int y = 0; y < nextHeight; ++y)
					for (int x = 0; x < nextWidth; ++x)
					{
						// 面积区间覆盖奇数边长的最后一行/列，避免 3->1 时丢掉边缘。
						int x0 = x * mipWidth / nextWidth, x1 = (x + 1) * mipWidth / nextWidth;
						int y0 = y * mipHeight / nextHeight, y1 = (y + 1) * mipHeight / nextHeight;
						unsigned sums[4] = {};
						for (int sourceY = y0; sourceY < y1; ++sourceY)
							for (int sourceX = x0; sourceX < x1; ++sourceX)
							{
								size_t source =
									mipOffset +
									((static_cast<size_t>(layer) * mipHeight + sourceY) * mipWidth + sourceX) * 4;
								for (int channel = 0; channel < 4; ++channel)
									sums[channel] += upload[source + channel];
							}
						size_t target =
							nextOffset + ((static_cast<size_t>(layer) * nextHeight + y) * nextWidth + x) * 4;
						unsigned count = static_cast<unsigned>((x1 - x0) * (y1 - y0));
						for (int channel = 0; channel < 4; ++channel)
							upload[target + channel] = static_cast<unsigned char>((sums[channel] + count / 2) / count);
					}
			mipOffset = nextOffset;
			mipWidth = nextWidth;
			mipHeight = nextHeight;
		}
		auto staging = CreateBuffer(upload.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, upload.data());
		auto image = CreateImage(width, height, layers, VK_FORMAT_R8G8B8A8_UNORM,
								 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, layers == 6, mipLevels);
		auto commands = BeginUpload();
		Barrier(commands, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, layers,
				mipLevels);
		vkCmdCopyBufferToImage(commands, staging.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							   static_cast<uint32_t>(regions.size()), regions.data());
		Barrier(commands, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, layers, mipLevels);
		EndUpload(commands);
		DestroyBuffer(staging);
		return image;
	}
	VulkanBackend::MeshBuffers &VulkanBackend::GetMesh(const MeshPtr &mesh)
	{
		auto found = m_meshes.find(mesh->GetHandle());
		if (found != m_meshes.end() && found->second.revision == mesh->m_revision)
			return found->second;
		if (found != m_meshes.end())
		{
			// 单帧串行后端：上一帧fence完成才销毁旧缓冲，缓存键保持不变。
			CheckVK(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX), "Wait terrain update fence");
			DestroyBuffer(found->second.vertices);
			DestroyBuffer(found->second.indices);
			m_meshes.erase(found);
		}
		MeshBuffers result;
		result.revision = mesh->m_revision;
		result.terrain = bool(mesh->m_terrain);
		result.owner = mesh;
		result.vertices = CreateBuffer(mesh->m_vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
									   mesh->m_vertices.data());
		result.indices = CreateBuffer(mesh->m_indices.size() * sizeof(unsigned), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
									  mesh->m_indices.data());
		return m_meshes.emplace(mesh->GetHandle(), result).first->second;
	}
	VulkanBackend::Image &VulkanBackend::GetTexture(const TexturePtr &input)
	{
		auto texture = input ? input : TextureManager::GetInstance()->GetWhiteTexture();
		auto found = m_textures.find(texture->GetHandle());
		if (found != m_textures.end())
			return found->second;
		return m_textures
			.emplace(texture->GetHandle(),
					 UploadImage(texture->m_width, texture->m_height, 1, texture->m_pixels.data()))
			.first->second;
	}
	VulkanBackend::Image &VulkanBackend::GetCube(const TextureCubePtr &cube)
	{
		if (!cube)
			return m_blackCube;
		auto found = m_cubes.find(cube->GetHandle());
		if (found != m_cubes.end())
			return found->second;
		// 先检查所有指针，再访问第 0 面，损坏资源报错而不是空指针崩溃。
		for (const auto &face : cube->m_faces)
			if (!face)
				throw std::runtime_error("Vulkan: missing cubemap face");
		int width = cube->m_faces[0]->m_width, height = cube->m_faces[0]->m_height;
		if (width < 1 || height != width)
			throw std::runtime_error("Vulkan: cubemap faces must have positive square dimensions");
		size_t faceSize = static_cast<size_t>(width) * height * 4;
		for (int i = 0; i < 6; ++i)
		{
			if (cube->m_faces[i]->m_width != width || cube->m_faces[i]->m_height != height ||
				cube->m_faces[i]->m_pixels.size() < faceSize)
				throw std::runtime_error("Vulkan: invalid cubemap face");
		}
		std::vector<unsigned char> pixels(faceSize * 6);
		for (int i = 0; i < 6; ++i)
		{
			std::memcpy(pixels.data() + i * faceSize, cube->m_faces[i]->m_pixels.data(), faceSize);
		}
		return m_cubes.emplace(cube->GetHandle(), UploadImage(width, height, 6, pixels.data())).first->second;
	}

	void VulkanBackend::Initilize(void *window, int width, int height, int samples, bool warp)
	{
		// 先选择同时支持图形和 Win32 present 的队列，避免跨队列所有权转换。
		if (warp)
			throw std::runtime_error("Vulkan backend does not support DX11 --warp; select a Vulkan ICD explicitly");
		m_width = width;
		m_height = height;
		m_loader = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!m_loader)
			throw std::runtime_error("Vulkan system loader is missing; install the GPU Vulkan driver");
		auto getInstanceProc =
			reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(m_loader, "vkGetInstanceProcAddr"));
		if (!getInstanceProc)
			throw std::runtime_error("Vulkan loader has no vkGetInstanceProcAddr");
		auto createInstance = reinterpret_cast<PFN_vkCreateInstance>(getInstanceProc(nullptr, "vkCreateInstance"));
		if (!createInstance)
			throw std::runtime_error("Vulkan loader has no vkCreateInstance");
		VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
		app.pApplicationName = "Renderer";
		app.apiVersion = VK_API_VERSION_1_1;
		const char *extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
		VkInstanceCreateInfo instance = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		instance.pApplicationInfo = &app;
		instance.enabledExtensionCount = 2;
		instance.ppEnabledExtensionNames = extensions;
		CheckVK(createInstance(&instance, nullptr, &m_instance), "Create Vulkan 1.1 instance");
#define LOAD_INSTANCE(name)                                                                                            \
	name = reinterpret_cast<PFN_##name>(getInstanceProc(m_instance, #name));                                           \
	if (!name)                                                                                                         \
		throw std::runtime_error("Missing Vulkan entry: " #name);
		VULKAN_INSTANCE_FUNCTIONS(LOAD_INSTANCE)
#undef LOAD_INSTANCE
		VkWin32SurfaceCreateInfoKHR surface = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
		surface.hinstance = GetModuleHandleW(nullptr);
		surface.hwnd = static_cast<HWND>(window);
		CheckVK(vkCreateWin32SurfaceKHR(m_instance, &surface, nullptr, &m_surface), "Create Win32 surface");
		uint32_t count = 0;
		CheckVK(vkEnumeratePhysicalDevices(m_instance, &count, nullptr), "Count Vulkan devices");
		std::vector<VkPhysicalDevice> devices(count);
		CheckVK(vkEnumeratePhysicalDevices(m_instance, &count, devices.data()), "Enumerate Vulkan devices");
		int bestScore = -1;
		for (auto gpu : devices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpu, &properties);
			if (properties.apiVersion < VK_API_VERSION_1_1)
				continue;
			uint32_t extensionCount = 0;
			CheckVK(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr),
					"Count device extensions");
			std::vector<VkExtensionProperties> supported(extensionCount);
			CheckVK(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, supported.data()),
					"Enumerate device extensions");
			bool hasSwapchain = false;
			bool hasRayQuery = false, hasAccelerationStructure = false;
			for (auto &extension : supported)
			{
				if (!std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
					hasSwapchain = true;
				if (!std::strcmp(extension.extensionName, VK_KHR_RAY_QUERY_EXTENSION_NAME))
					hasRayQuery = true;
				if (!std::strcmp(extension.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
					hasAccelerationStructure = true;
			}
			if (!hasSwapchain)
				continue;
			uint32_t familyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
			std::vector<VkQueueFamilyProperties> families(familyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());
			for (uint32_t i = 0; i < familyCount; ++i)
			{
				VkBool32 present = false;
				CheckVK(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, m_surface, &present), "Check present support");
				int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 2 : 1;
				if (present && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && score > bestScore)
				{
					m_gpu = gpu;
					m_family = i;
					m_timestampBits = families[i].timestampValidBits;
					m_properties = properties;
					bestScore = score;
					// 只查询实际 feature，不在当前设备上启用或把普通光照冒充硬件光追。
					m_rayQuery = false;
					if (hasRayQuery && hasAccelerationStructure)
					{
						VkPhysicalDeviceRayQueryFeaturesKHR ray = {
							VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
						VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration = {
							VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
						acceleration.pNext = &ray;
						VkPhysicalDeviceFeatures2 features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
						features.pNext = &acceleration;
						vkGetPhysicalDeviceFeatures2(gpu, &features);
						m_rayQuery = ray.rayQuery && acceleration.accelerationStructure;
					}
				}
			}
		}
		if (!m_gpu)
			throw std::runtime_error("No Vulkan 1.1 graphics device supports this window");
		vkGetPhysicalDeviceMemoryProperties(m_gpu, &m_memory);
		for (auto format : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM})
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(m_gpu, format, &properties);
			if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				m_depthFormat = format;
				break;
			}
		}
		float priority = 1;
		VkDeviceQueueCreateInfo queue = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		queue.queueFamilyIndex = m_family;
		queue.queueCount = 1;
		queue.pQueuePriorities = &priority;
		const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		VkDeviceCreateInfo device = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
		device.queueCreateInfoCount = 1;
		device.pQueueCreateInfos = &queue;
		device.enabledExtensionCount = 1;
		device.ppEnabledExtensionNames = deviceExtensions;
		CheckVK(vkCreateDevice(m_gpu, &device, nullptr, &m_device), "Create Vulkan device");
#define LOAD_DEVICE(name)                                                                                              \
	name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(m_device, #name));                                         \
	if (!name)                                                                                                         \
		throw std::runtime_error("Missing Vulkan entry: " #name);
		VULKAN_DEVICE_FUNCTIONS(LOAD_DEVICE)
#undef LOAD_DEVICE
		vkGetDeviceQueue(m_device, m_family, 0, &m_queue);
		VkCommandPoolCreateInfo pool = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
		pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool.queueFamilyIndex = m_family;
		CheckVK(vkCreateCommandPool(m_device, &pool, nullptr, &m_commandPool), "Create command pool");
		VkCommandBufferAllocateInfo allocation = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocation.commandPool = m_commandPool;
		allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocation.commandBufferCount = 1;
		CheckVK(vkAllocateCommandBuffers(m_device, &allocation, &m_commands), "Allocate frame commands");
		VkFenceCreateInfo fence = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		CheckVK(vkCreateFence(m_device, &fence, nullptr, &m_fence), "Create frame fence");
		VkSemaphoreCreateInfo semaphore = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		CheckVK(vkCreateSemaphore(m_device, &semaphore, nullptr, &m_acquired), "Create acquire semaphore");
		CheckVK(vkCreateSemaphore(m_device, &semaphore, nullptr, &m_finished), "Create render semaphore");
		// One small uniform buffer for the frame, one storage buffer holding every object.
		m_frameConstants = CreateBuffer(sizeof(FrameConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		m_objects = CreateBuffer(sizeof(ObjectConstants) * MaxObjects, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		// 每个元素只存物体数组下标。绘制时绑定对应偏移，按实例读取一个 uint。
		std::vector<uint32_t> primitiveIds(MaxObjects);
		for (uint32_t i = 0; i < MaxObjects; i++)
			primitiveIds[i] = i;
		m_primitiveIds = CreateBuffer(sizeof(uint32_t) * MaxObjects, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			primitiveIds.data());
		void *mapped = nullptr;
		CheckVK(vkMapMemory(m_device, m_frameConstants.memory, 0, VK_WHOLE_SIZE, 0, &mapped), "Map frame constants");
		m_frameData = static_cast<FrameConstants *>(mapped);
		CheckVK(vkMapMemory(m_device, m_objects.memory, 0, VK_WHOLE_SIZE, 0, &mapped), "Map object constants");
		m_objectData = static_cast<ObjectConstants *>(mapped);
		VkSamplerCreateInfo sampler = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		sampler.magFilter = sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.maxLod = VK_LOD_CLAMP_NONE;
		CheckVK(vkCreateSampler(m_device, &sampler, nullptr, &m_sampler), "Create texture sampler");
		sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		CheckVK(vkCreateSampler(m_device, &sampler, nullptr, &m_cubeSampler), "Create cube sampler");
		CreateDescriptors();
		if (m_timestampBits)
		{
			VkQueryPoolCreateInfo query = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
			query.queryType = VK_QUERY_TYPE_TIMESTAMP;
			query.queryCount = 2;
			CheckVK(vkCreateQueryPool(m_device, &query, nullptr, &m_queries), "Create timestamp queries");
		}
		// 无天空/环境时采样黑色，表示没有环境辐射；二维基础贴图仍使用 TextureManager 白色回退。
		std::array<unsigned char, 24> black = {};
		for (size_t i = 3; i < black.size(); i += 4)
			black[i] = 255;
		m_blackCube = UploadImage(1, 1, 6, black.data());
		CreateSwapchain();
		Platform::Log(std::string("Renderer: Vulkan 1.1, GPU=") + m_properties.deviceName +
					  ", samples=1 (requested=" + std::to_string(samples) + "), color/depth sample mask=" +
					  std::to_string(m_properties.limits.framebufferColorSampleCounts &
									 m_properties.limits.framebufferDepthSampleCounts));
		Platform::Log("Vulkan: single frame in flight, FIFO present initially; 2048 shadow map enabled; MSAA and hardware ray "
					  "queries are not enabled; shaders=offline DXC SPIR-V");
		Platform::Log("Vulkan textures: complete CPU-generated RGBA8 mip chains with trilinear sampling");
		Platform::Log(std::string("Vulkan ray-query/acceleration-structure features: ") +
					  (m_rayQuery ? "supported, not enabled" : "not supported") +
					  "; no ray tracing is executed by this backend");
	}

	void VulkanBackend::CreateDescriptors()
	{
		// Set 0：每个物体都用同一份相机、物体数组和采样器。
		// Buffer 内的数据会变，但句柄、范围没有变，所以这里只需填写一次。
		VkDescriptorSetLayoutBinding sceneBindings[] = {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{3, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
		VkDescriptorSetLayoutCreateInfo layout = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		layout.bindingCount = 4;
		layout.pBindings = sceneBindings;
		CheckVK(vkCreateDescriptorSetLayout(m_device, &layout, nullptr, &m_descriptorLayouts[0]), "Create scene layout");

		// Set 1：颜色、透明遮罩、法线。Set 2：环境、天空、阴影。
		// 两组恰好都是三张图，但保留各自的布局，方便读出它们的用途。
		VkDescriptorSetLayoutBinding imageBindings[3] = {};
		for (uint32_t i = 0; i < 3; i++)
			imageBindings[i] = {i, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
		layout.bindingCount = 3;
		layout.pBindings = imageBindings;
		CheckVK(vkCreateDescriptorSetLayout(m_device, &layout, nullptr, &m_descriptorLayouts[1]), "Create material layout");
		CheckVK(vkCreateDescriptorSetLayout(m_device, &layout, nullptr, &m_descriptorLayouts[2]), "Create pass layout");
		VkPipelineLayoutCreateInfo pipelineLayout = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayout.setLayoutCount = 3;
		pipelineLayout.pSetLayouts = m_descriptorLayouts;
		VkPushConstantRange push = {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DrawPush)};
		pipelineLayout.pushConstantRangeCount = 1;
		pipelineLayout.pPushConstantRanges = &push;
		CheckVK(vkCreatePipelineLayout(m_device, &pipelineLayout, nullptr, &m_pipelineLayout), "Create pipeline layout");

		// 固定池只分配一个公共 Set 和两个 Pass Set，不再按 Draw 数预留几十万条描述符。
		VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_SAMPLER, 2},
			{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 6}};
		VkDescriptorPoolCreateInfo pool = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		pool.maxSets = 3;
		pool.poolSizeCount = 4;
		pool.pPoolSizes = sizes;
		CheckVK(vkCreateDescriptorPool(m_device, &pool, nullptr, &m_descriptorPool), "Create scene/pass pool");
		VkDescriptorSetLayout layouts[] = {m_descriptorLayouts[0], m_descriptorLayouts[2], m_descriptorLayouts[2]};
		VkDescriptorSet sets[3] = {};
		VkDescriptorSetAllocateInfo allocation = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		allocation.descriptorPool = m_descriptorPool;
		allocation.descriptorSetCount = 3;
		allocation.pSetLayouts = layouts;
		CheckVK(vkAllocateDescriptorSets(m_device, &allocation, sets), "Allocate scene/pass sets");
		m_sceneSet = sets[0];
		m_passSets[0] = sets[1];
		m_passSets[1] = sets[2];

		VkDescriptorBufferInfo buffers[] = {{m_frameConstants.buffer, 0, sizeof(FrameConstants)},
			{m_objects.buffer, 0, VK_WHOLE_SIZE}};
		VkDescriptorImageInfo samplers[2] = {};
		samplers[0].sampler = m_sampler;
		samplers[1].sampler = m_cubeSampler;
		VkWriteDescriptorSet writes[4] = {};
		for (uint32_t i = 0; i < 4; i++)
		{
			writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[i].dstSet = m_sceneSet;
			writes[i].dstBinding = i;
			writes[i].descriptorCount = 1;
			writes[i].descriptorType = sceneBindings[i].descriptorType;
			if (i < 2)
				writes[i].pBufferInfo = &buffers[i];
			else
				writes[i].pImageInfo = &samplers[i - 2];
		}
		vkUpdateDescriptorSets(m_device, 4, writes, 0, nullptr);
	}

	VkDescriptorSet VulkanBackend::GetMaterialDescriptors(const MaterialPtr &i_material)
	{
		// 颜色/粗糙度等标量仍在物体 SSBO。这里只按实际贴图组合查缓存，换标量不用建新 Set。
		auto white = TextureManager::GetInstance()->GetWhiteTexture();
		TexturePtr textures[] = {i_material && i_material->m_texture ? i_material->m_texture : white,
			i_material && i_material->m_opacityTexture ? i_material->m_opacityTexture : white,
			i_material && i_material->m_normalTexture ? i_material->m_normalTexture : white};
		std::array<uint64_t, 3> key = {textures[0]->GetHandle(), textures[1]->GetHandle(), textures[2]->GetHandle()};
		auto found = m_materialSets.find(key);
		if (found != m_materialSets.end())
			return found->second;

		// 每池容纳256种贴图组合，用完再加池；已有 Set 保持有效，不因每帧结束就释放。
		if (m_materialSets.size() % MaterialsPerPool == 0)
		{
			VkDescriptorPoolSize size = {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaterialsPerPool * 3};
			VkDescriptorPoolCreateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
			info.maxSets = MaterialsPerPool;
			info.poolSizeCount = 1;
			info.pPoolSizes = &size;
			VkDescriptorPool pool;
			CheckVK(vkCreateDescriptorPool(m_device, &info, nullptr, &pool), "Create material pool");
			m_materialPools.push_back(pool);
		}
		VkDescriptorSetAllocateInfo allocation = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		allocation.descriptorPool = m_materialPools.back();
		allocation.descriptorSetCount = 1;
		allocation.pSetLayouts = &m_descriptorLayouts[1];
		VkDescriptorSet set;
		CheckVK(vkAllocateDescriptorSets(m_device, &allocation, &set), "Allocate material set");
		VkDescriptorImageInfo images[3] = {};
		VkWriteDescriptorSet writes[3] = {};
		for (uint32_t i = 0; i < 3; i++)
		{
			images[i].imageView = GetTexture(textures[i]).view;
			images[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[i].dstSet = set;
			writes[i].dstBinding = i;
			writes[i].descriptorCount = 1;
			writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[i].pImageInfo = &images[i];
		}
		vkUpdateDescriptorSets(m_device, 3, writes, 0, nullptr);
		m_materialSets.emplace(key, set);
		++m_newMaterialSets;
		// 与现有 GPU 贴图缓存一起保留到 ShutDown，尚未做场景卸载时的资源淘汰。
		return set;
	}

	void VulkanBackend::UpdatePassDescriptors(const RenderFrame &i_frame)
	{
		// 调用前已经等待上一帧 fence。只在换天空/环境或重建阴影图时更新，普通帧不写描述符。
		for (int pass = 0; pass < 2; pass++)
		{
			std::array<VkImageView, 3> views = {GetCube(i_frame.m_irradiance).view, GetCube(i_frame.m_sky).view,
				pass == 0 ? GetTexture(nullptr).view : m_shadow.view};
			if (views == m_passViews[pass])
				continue;
			// 阴影 Pass 绑定白图，不能采样正在作为附件写入的阴影图。
			VkDescriptorImageInfo images[3] = {};
			VkWriteDescriptorSet writes[3] = {};
			for (uint32_t i = 0; i < 3; i++)
			{
				images[i].imageView = views[i];
				images[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[i].dstSet = m_passSets[pass];
				writes[i].dstBinding = i;
				writes[i].descriptorCount = 1;
				writes[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				writes[i].pImageInfo = &images[i];
			}
			vkUpdateDescriptorSets(m_device, 3, writes, 0, nullptr);
			m_passViews[pass] = views;
			++m_passUpdates;
		}
	}

	void VulkanBackend::Resize(int width, int height)
	{
		if (width < 1 || height < 1)
			return;
		m_width = width;
		m_height = height;
		m_resize = true;
	}
	void VulkanBackend::DestroySwapchain()
	{
		// 即使驱动重用了旧 imageView 数值，下次也必须刷新指向新阴影图的 Pass Set。
		m_passViews[0] = m_passViews[1] = {};
		if (m_shadowFramebuffer)
			vkDestroyFramebuffer(m_device, m_shadowFramebuffer, nullptr);
		m_shadowFramebuffer = VK_NULL_HANDLE;
		if (m_framebuffer)
			vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
		m_framebuffer = VK_NULL_HANDLE;
		for (auto &pipeline : m_pipelines)
		{
			if (pipeline)
				vkDestroyPipeline(m_device, pipeline, nullptr);
			pipeline = VK_NULL_HANDLE;
		}
		if (m_renderPass)
			vkDestroyRenderPass(m_device, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
		if (m_shadowPass)
			vkDestroyRenderPass(m_device, m_shadowPass, nullptr);
		m_shadowPass = VK_NULL_HANDLE;
		DestroyImage(m_shadow);
		DestroyImage(m_shadowDepth);
		DestroyImage(m_color);
		DestroyImage(m_depth);
		if (m_swapchain)
			vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
		m_swapImages.clear();
		m_rendered = false;
	}
	void VulkanBackend::CreateSwapchain()
	{
		CheckVK(vkDeviceWaitIdle(m_device), "Wait before swapchain recreation");
		DestroySwapchain();
		VkSurfaceCapabilitiesKHR caps;
		CheckVK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu, m_surface, &caps), "Get surface capabilities");
		if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			throw std::runtime_error("Vulkan surface does not support transfer destination images");
		uint32_t count = 0;
		CheckVK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &count, nullptr), "Count surface formats");
		std::vector<VkSurfaceFormatKHR> formats(count);
		CheckVK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &count, formats.data()), "Get surface formats");
		VkSurfaceFormatKHR format = {};
		bool found = false;
		for (auto candidate : formats)
			if ((candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_R8G8B8A8_UNORM) &&
				candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				format = candidate;
				found = true;
				break;
			}
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		{
			format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
			found = true;
		}
		if (!found)
			throw std::runtime_error("Vulkan surface has no supported RGBA/BGRA UNORM format");
		m_format = format.format;
		m_extent = caps.currentExtent;
		if (m_extent.width == std::numeric_limits<uint32_t>::max())
		{
			m_extent.width =
				std::clamp(static_cast<uint32_t>(m_width), caps.minImageExtent.width, caps.maxImageExtent.width);
			m_extent.height =
				std::clamp(static_cast<uint32_t>(m_height), caps.minImageExtent.height, caps.maxImageExtent.height);
		}
		if (!m_extent.width || !m_extent.height)
		{
			m_resize = true;
			return;
		}
		VkPresentModeKHR present = VK_PRESENT_MODE_FIFO_KHR;
		CheckVK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &count, nullptr), "Count present modes");
		std::vector<VkPresentModeKHR> modes(count);
		CheckVK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &count, modes.data()), "Get present modes");
		if (!m_vsync)
		{
			if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end())
				present = VK_PRESENT_MODE_MAILBOX_KHR;
			else if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != modes.end())
				present = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
		VkSwapchainCreateInfoKHR swap = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
		swap.surface = m_surface;
		swap.minImageCount = std::max(2u, caps.minImageCount);
		if (caps.maxImageCount)
			swap.minImageCount = std::min(swap.minImageCount, caps.maxImageCount);
		swap.imageFormat = m_format;
		swap.imageColorSpace = format.colorSpace;
		swap.imageExtent = m_extent;
		swap.imageArrayLayers = 1;
		swap.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swap.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swap.preTransform = caps.currentTransform;
		swap.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		for (auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
						   VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
			if (caps.supportedCompositeAlpha & alpha)
			{
				swap.compositeAlpha = alpha;
				break;
			}
		swap.presentMode = present;
		swap.clipped = VK_TRUE;
		CheckVK(vkCreateSwapchainKHR(m_device, &swap, nullptr, &m_swapchain), "Create swapchain");
		CheckVK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr), "Count swapchain images");
		m_swapImages.resize(count);
		CheckVK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, m_swapImages.data()), "Get swapchain images");
		m_color = CreateImage(m_extent.width, m_extent.height, 1, m_format,
							  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		m_depth =
			CreateImage(m_extent.width, m_extent.height, 1, m_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		VkAttachmentDescription attachments[2] = {};
		attachments[0].format = m_format;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		attachments[1].format = m_depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference color = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
							  depth = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color;
		subpass.pDepthStencilAttachment = &depth;
		VkSubpassDependency dependencies[2] = {};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		VkRenderPassCreateInfo pass = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		pass.attachmentCount = 2;
		pass.pAttachments = attachments;
		pass.subpassCount = 1;
		pass.pSubpasses = &subpass;
		pass.dependencyCount = 2;
		pass.pDependencies = dependencies;
		CheckVK(vkCreateRenderPass(m_device, &pass, nullptr, &m_renderPass), "Create render pass");
		VkImageView views[] = {m_color.view, m_depth.view};
		VkFramebufferCreateInfo framebuffer = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		framebuffer.renderPass = m_renderPass;
		framebuffer.attachmentCount = 2;
		framebuffer.pAttachments = views;
		framebuffer.width = m_extent.width;
		framebuffer.height = m_extent.height;
		framebuffer.layers = 1;
		CheckVK(vkCreateFramebuffer(m_device, &framebuffer, nullptr, &m_framebuffer), "Create framebuffer");
		CreateShadowMap();
		CreatePipelines();
		m_resize = false;
		Platform::Log("Vulkan swapchain: " + std::to_string(m_extent.width) + "x" + std::to_string(m_extent.height) +
					  ", presentMode=" + std::to_string(present));
	}

	void VulkanBackend::CreateShadowMap()
	{
		m_shadowInitialized = false;
		// 用颜色附件保存深度，普通纹理读取即可完成手动 PCF。
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_gpu, VK_FORMAT_R32_SFLOAT, &properties);
		const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		if ((properties.optimalTilingFeatures & required) != required)
			throw std::runtime_error("Vulkan: R32_SFLOAT shadow attachment is unsupported");
		m_shadow = CreateImage(ShadowSize, ShadowSize, 1, VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		m_shadowDepth = CreateImage(ShadowSize, ShadowSize, 1, m_depthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		// 即使第一帧关闭阴影，主 pass 的描述符也始终引用有效布局。
		auto commands = BeginUpload();
		Barrier(commands, m_shadow.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		EndUpload(commands);
		VkAttachmentDescription attachments[2] = {};
		attachments[0].format = VK_FORMAT_R32_SFLOAT;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachments[1].format = m_depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference color = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		VkAttachmentReference depth = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color;
		subpass.pDepthStencilAttachment = &depth;
		VkSubpassDependency dependencies[2] = {};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		VkRenderPassCreateInfo pass = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		pass.attachmentCount = 2;
		pass.pAttachments = attachments;
		pass.subpassCount = 1;
		pass.pSubpasses = &subpass;
		pass.dependencyCount = 2;
		pass.pDependencies = dependencies;
		CheckVK(vkCreateRenderPass(m_device, &pass, nullptr, &m_shadowPass), "Create shadow render pass");
		VkImageView views[] = {m_shadow.view, m_shadowDepth.view};
		VkFramebufferCreateInfo framebuffer = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		framebuffer.renderPass = m_shadowPass;
		framebuffer.attachmentCount = 2;
		framebuffer.pAttachments = views;
		framebuffer.width = framebuffer.height = ShadowSize;
		framebuffer.layers = 1;
		CheckVK(vkCreateFramebuffer(m_device, &framebuffer, nullptr, &m_shadowFramebuffer), "Create shadow framebuffer");
	}

	VkShaderModule VulkanBackend::LoadShader(const char *path)
	{
		std::ifstream file(Platform::GetAssetPath(path), std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error(std::string("Missing offline Vulkan shader: ") + path);
		auto size = file.tellg();
		if (size < 20 || static_cast<size_t>(size) % 4)
			throw std::runtime_error("Invalid SPIR-V size");
		std::vector<uint32_t> words(static_cast<size_t>(size) / 4);
		file.seekg(0);
		file.read(reinterpret_cast<char *>(words.data()), size);
		if (!file || words[0] != 0x07230203)
			throw std::runtime_error("Invalid SPIR-V module");
		VkShaderModuleCreateInfo info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		info.codeSize = words.size() * 4;
		info.pCode = words.data();
		VkShaderModule module;
		CheckVK(vkCreateShaderModule(m_device, &info, nullptr, &module), "Create shader module");
		return module;
	}
	void VulkanBackend::CreatePipelines()
	{
		VkShaderModule vertex = LoadShader("Shaders/Vulkan/Default.vert.spv"), pixel = VK_NULL_HANDLE;
		try
		{
			pixel = LoadShader("Shaders/Vulkan/Default.frag.spv");
			VkPipelineShaderStageCreateInfo stages[2] = {};
			stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			stages[0].module = vertex;
			stages[0].pName = "VSMain";
			stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			stages[1].module = pixel;
			stages[1].pName = "PSMain";
			VkVertexInputBindingDescription bindings[] = {
				{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
				{1, sizeof(uint32_t), VK_VERTEX_INPUT_RATE_INSTANCE}};
			VkVertexInputAttributeDescription attributes[] = {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, _position)},
				{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, _normal)},
				{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, _texCoords)},
				{3, 1, VK_FORMAT_R32_UINT, 0}};
			VkPipelineVertexInputStateCreateInfo input = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
			input.vertexBindingDescriptionCount = 2;
			input.pVertexBindingDescriptions = bindings;
			input.vertexAttributeDescriptionCount = 4;
			input.pVertexAttributeDescriptions = attributes;
			VkPipelineInputAssemblyStateCreateInfo assembly = {
				VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
			assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkPipelineViewportStateCreateInfo viewport = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
			viewport.viewportCount = viewport.scissorCount = 1;
			VkPipelineRasterizationStateCreateInfo raster = {
				VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
			raster.polygonMode = VK_POLYGON_MODE_FILL;
			raster.cullMode = VK_CULL_MODE_NONE;
			raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			raster.lineWidth = 1;
			VkPipelineMultisampleStateCreateInfo samples = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
			samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			VkPipelineDepthStencilStateCreateInfo depth = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
			depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			VkPipelineColorBlendAttachmentState target = {};
			target.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
									VK_COLOR_COMPONENT_A_BIT;
			target.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			target.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			target.colorBlendOp = VK_BLEND_OP_ADD;
			target.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			target.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			target.alphaBlendOp = VK_BLEND_OP_ADD;
			VkPipelineColorBlendStateCreateInfo blend = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
			blend.attachmentCount = 1;
			blend.pAttachments = &target;
			VkDynamicState states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
			VkPipelineDynamicStateCreateInfo dynamic = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
			dynamic.dynamicStateCount = 2;
			dynamic.pDynamicStates = states;
			VkGraphicsPipelineCreateInfo pipeline = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
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
			pipeline.layout = m_pipelineLayout;
			pipeline.renderPass = m_renderPass;
			// 0=实体，1=天空，2=透明，3=精灵。材质仍使用共享 PBR 参数，不扩展常量缓冲。
			for (int i = 0; i < 4; ++i)
			{
				depth.depthTestEnable = i != 3;
				depth.depthWriteEnable = i == 0;
				target.blendEnable = i >= 2;
				CheckVK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &m_pipelines[i]),
						"Create graphics pipeline");
			}
			// 阴影沿用共享着色器的 mode 3，保留原有透明和精灵管线索引。
			pipeline.renderPass = m_shadowPass;
			depth.depthCompareOp = VK_COMPARE_OP_LESS;
			depth.depthTestEnable = depth.depthWriteEnable = VK_TRUE;
			target.blendEnable = VK_FALSE;
			target.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
			CheckVK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &m_pipelines[4]),
					"Create shadow pipeline");
		}
		catch (...)
		{
			if (pixel)
				vkDestroyShaderModule(m_device, pixel, nullptr);
			vkDestroyShaderModule(m_device, vertex, nullptr);
			throw;
		}
		vkDestroyShaderModule(m_device, pixel, nullptr);
		vkDestroyShaderModule(m_device, vertex, nullptr);
	}
	ObjectConstants VulkanBackend::MakeObject(const DrawItem &item)
	{
		ObjectConstants result = {};
		auto *material = item.m_material.get();
		result.m_model = item.m_transform;
		result.m_normal = glm::inverseTranspose(result.m_model);
		result.m_color = material ? material->m_color : glm::vec4(1);
		result.m_material = glm::vec4(material ? material->m_metallic : 0,
									  material ? glm::clamp(material->m_roughness, .045f, 1.f) : .5f,
									  material ? material->m_ao : 1, 0);
		result.m_surface = glm::vec4(material ? material->m_alphaCutoff : 0,
									 material && material->m_opacityTexture ? 1.f : 0.f,
									 material && material->m_normalTexture ? 1.f : 0.f, 0);
		return result;
	}
	void VulkanBackend::WriteConstants(const RenderFrame &frame)
	{
		// Frame data once. The shadow pass and the main pass both read this same block.
		FrameConstants constants = {};
		constants.m_view = frame.m_camera.WorldToView_Matrix;
		constants.m_projection = frame.m_camera.Projective_Matrix;
		constants.m_spriteProjection = frame.m_spriteProjection;
		constants.m_lightViewProjection = frame.m_lightViewProjection;
		constants.m_viewPosition = glm::vec4(frame.m_camera.cameraPos, 1);
		constants.m_sunDirection = glm::vec4(glm::normalize(frame.m_sunDirection), 0);
		constants.m_sunColor = glm::vec4(frame.m_sunColor, 0);
		for (int i = 0; i < 4; ++i)
		{
			constants.m_pointPosition[i] = frame.m_pointLights[i].m_position;
			constants.m_pointColor[i] = frame.m_pointLights[i].m_color;
			constants.m_pointAttenuation[i] = frame.m_pointLights[i].m_attenuation;
		}
		constants.m_shadowParams = frame.m_shadowParams;
		*m_frameData = constants;
		// Object data once per object, however many passes draw it. One slot is kept for the sky.
		if (frame.m_items.size() + 1 > MaxObjects)
			throw std::runtime_error("Vulkan: object count exceeds 65535");
		for (size_t i = 0; i < frame.m_items.size(); ++i)
			m_objectData[i] = MakeObject(frame.m_items[i]);
	}
	void VulkanBackend::Draw(const DrawItem &item, int mode, uint32_t object)
	{
		// 共用 CPU 矩阵和 PBR 参数。Vulkan 深度 0..1 与屏幕 Y 翻转在离线顶点 Shader 完成。
		if (!item.m_mesh || item.m_submesh >= item.m_mesh->m_subMeshes.size())
			return;
		if (object >= MaxObjects)
			throw std::runtime_error("Vulkan: primitive index exceeds object buffer");
		++m_drawIndex;
		auto material = item.m_material;
		bool transparent = material && material->m_transparent;
		int pipeline = mode == 3 ? 4 : mode == 1 ? 3 : mode == 2 ? 1 : transparent ? 2 : 0;
		vkCmdBindPipeline(m_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[pipeline]);
		// Set 0、2 已经在帧/Pass 开头绑定。这里只在贴图组合改变时换 Set 1。
		auto set = GetMaterialDescriptors(material);
		if (set != m_boundMaterial)
		{
			vkCmdBindDescriptorSets(m_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &set, 0, nullptr);
			m_boundMaterial = set;
		}
		DrawPush push = {static_cast<uint32_t>(mode)};
		vkCmdPushConstants(m_commands, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
						   sizeof(push), &push);
		auto &mesh = GetMesh(item.m_mesh);
		VkDeviceSize vertexOffset = 0;
		vkCmdBindVertexBuffers(m_commands, 0, 1, &mesh.vertices.buffer, &vertexOffset);
		// Binding 0 读模型顶点；binding 1 读该物体的编号。firstInstance 保持0，避免重复加偏移。
		VkDeviceSize primitiveOffset = VkDeviceSize(object) * sizeof(uint32_t);
		vkCmdBindVertexBuffers(m_commands, 1, 1, &m_primitiveIds.buffer, &primitiveOffset);
		vkCmdBindIndexBuffer(m_commands, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		auto &part = item.m_mesh->m_subMeshes[item.m_submesh];
		vkCmdDrawIndexed(m_commands, part.m_indexCount, 1, part.m_firstIndex, 0, 0);
	}
	void VulkanBackend::Render(const RenderFrame &frame)
	{
		if (!m_device || m_pending)
			return;
		CheckVK(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX), "Wait frame fence");
		// 上一帧已经结束，单帧串行后端可直接销毁不再被CPU持有的地形。
		for (auto it = m_meshes.begin(); it != m_meshes.end();)
		{
			if (it->second.terrain && it->second.owner.expired())
			{
				DestroyBuffer(it->second.vertices);
				DestroyBuffer(it->second.indices);
				it = m_meshes.erase(it);
			}
			else
				++it;
		}

		if (m_resize)
			CreateSwapchain();
		if (!m_swapchain)
			return;
		m_newMaterialSets = m_passUpdates = 0;
		// 所有资源先上传完毕再记录绘制；上传命令不会插进正在记录的 render pass。
		GetTexture(nullptr);
		GetCube(frame.m_irradiance);
		GetCube(frame.m_sky);
		GetMaterialDescriptors(nullptr);
		for (auto &item : frame.m_items)
		{
			if (!item.m_mesh)
				continue;
			GetMesh(item.m_mesh);
			auto material = item.m_material;
			GetTexture(material ? material->m_texture : nullptr);
			GetTexture(material ? material->m_opacityTexture : nullptr);
			GetTexture(material ? material->m_normalTexture : nullptr);
			GetMaterialDescriptors(material);
		}
		if (frame.m_drawSky && frame.m_sky && frame.m_skyMesh)
			GetMesh(frame.m_skyMesh);
		UpdatePassDescriptors(frame);
		auto acquired =
			vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_acquired, VK_NULL_HANDLE, &m_imageIndex);
		if (acquired == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_resize = true;
			return;
		}
		if (acquired == VK_SUBOPTIMAL_KHR)
			m_resize = true;
		else
			CheckVK(acquired, "Acquire swapchain image");
		CheckVK(vkResetCommandBuffer(m_commands, 0), "Reset frame commands");
		VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		CheckVK(vkBeginCommandBuffer(m_commands, &begin), "Begin frame");
		if (m_profiling && m_queries)
		{
			vkCmdResetQueryPool(m_commands, m_queries, 0, 2);
			vkCmdWriteTimestamp(m_commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queries, 0);
		}
		m_drawIndex = 0;
		WriteConstants(frame);
		// 当前后端只有一帧在飞，公共 Set 跨帧保留，每次开始记录命令时重新绑定一次。
		m_boundMaterial = VK_NULL_HANDLE;
		vkCmdBindDescriptorSets(m_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_sceneSet, 0, nullptr);
		if (frame.m_shadowParams.w > 0 || !m_shadowInitialized)
		{
			vkCmdBindDescriptorSets(m_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1, &m_passSets[0], 0, nullptr);
			VkClearValue shadowClear[2] = {};
			shadowClear[0].color = {{1, 1, 1, 1}};
			shadowClear[1].depthStencil = {1, 0};
			VkRenderPassBeginInfo shadowPass = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			shadowPass.renderPass = m_shadowPass;
			shadowPass.framebuffer = m_shadowFramebuffer;
			shadowPass.renderArea.extent = {ShadowSize, ShadowSize};
			shadowPass.clearValueCount = 2;
			shadowPass.pClearValues = shadowClear;
			vkCmdBeginRenderPass(m_commands, &shadowPass, VK_SUBPASS_CONTENTS_INLINE);
			VkViewport shadowViewport = {0, 0, float(ShadowSize), float(ShadowSize), 0, 1};
			VkRect2D shadowScissor = {{0, 0}, {ShadowSize, ShadowSize}};
			vkCmdSetViewport(m_commands, 0, 1, &shadowViewport);
			vkCmdSetScissor(m_commands, 0, 1, &shadowScissor);
			for (size_t i = 0; i < frame.m_items.size(); ++i)
				if (CastsShadow(frame, frame.m_items[i]))
					Draw(frame.m_items[i], 3, static_cast<uint32_t>(i));
			vkCmdEndRenderPass(m_commands);
			m_shadowInitialized = true;
		}
		VkClearValue clear[2] = {};
		vkCmdBindDescriptorSets(m_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1, &m_passSets[1], 0, nullptr);
		clear[0].color = {{.025f, .04f, .065f, 1}};
		clear[1].depthStencil = {1, 0};
		VkRenderPassBeginInfo pass = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		pass.renderPass = m_renderPass;
		pass.framebuffer = m_framebuffer;
		pass.renderArea.extent = m_extent;
		pass.clearValueCount = 2;
		pass.pClearValues = clear;
		vkCmdBeginRenderPass(m_commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport = {0, 0, static_cast<float>(m_extent.width), static_cast<float>(m_extent.height), 0, 1};
		VkRect2D scissor = {{0, 0}, m_extent};
		vkCmdSetViewport(m_commands, 0, 1, &viewport);
		vkCmdSetScissor(m_commands, 0, 1, &scissor);
		if (frame.m_drawSky && frame.m_sky && frame.m_skyMesh)
		{
			DrawItem sky;
			sky.m_mesh = frame.m_skyMesh;
			sky.m_transform =
				glm::translate(glm::mat4(1), frame.m_camera.cameraPos) * glm::scale(glm::mat4(1), glm::vec3(100));
			// The sky uses the element after the scene objects.
			uint32_t skyIndex = static_cast<uint32_t>(frame.m_items.size());
			m_objectData[skyIndex] = MakeObject(sky);
			Draw(sky, 2, skyIndex);
		}
		for (size_t i = 0; i < frame.m_items.size(); ++i)
			Draw(frame.m_items[i], frame.m_items[i].m_sprite ? 1 : 0, static_cast<uint32_t>(i));
		vkCmdEndRenderPass(m_commands);
		// 交换链图像获取后放弃旧内容，完整覆盖并在交还 present 前转换布局。
		Barrier(m_commands, m_swapImages[m_imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		VkImageCopy copy = {};
		copy.srcSubresource = copy.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy.extent = {m_extent.width, m_extent.height, 1};
		vkCmdCopyImage(m_commands, m_color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swapImages[m_imageIndex],
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
		Barrier(m_commands, m_swapImages[m_imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
		if (m_profiling && m_queries)
			vkCmdWriteTimestamp(m_commands, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queries, 1);
		CheckVK(vkEndCommandBuffer(m_commands), "End frame");
		CheckVK(vkResetFences(m_device, 1, &m_fence), "Reset frame fence");
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &m_acquired;
		submit.pWaitDstStageMask = &waitStage;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &m_commands;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &m_finished;
		CheckVK(vkQueueSubmit(m_queue, 1, &submit, m_fence), "Submit frame");
		m_pending = true;
		m_rendered = true;
		if (m_profiling)
			Platform::Log("Vulkan descriptors: draws=" + std::to_string(m_drawIndex) +
				", cached-material-sets=" + std::to_string(m_materialSets.size()) +
				", new-material-sets=" + std::to_string(m_newMaterialSets) +
				", pass-set-updates=" + std::to_string(m_passUpdates));
		if (m_profiling && m_queries)
		{
			CheckVK(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX), "Wait GPU timestamps");
			uint64_t ticks[2];
			CheckVK(vkGetQueryPoolResults(m_device, m_queries, 0, 2, sizeof(ticks), ticks, sizeof(uint64_t),
										  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT),
					"Read GPU timestamps");
			uint64_t mask = m_timestampBits == 64 ? UINT64_MAX : ((uint64_t(1) << m_timestampBits) - 1);
			m_gpuTime =
				((ticks[1] - ticks[0]) & mask) * static_cast<double>(m_properties.limits.timestampPeriod) / 1000000.0;
		}
	}
	void VulkanBackend::Present(bool vsync)
	{
		if (m_pending)
		{
			VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
			present.waitSemaphoreCount = 1;
			present.pWaitSemaphores = &m_finished;
			present.swapchainCount = 1;
			present.pSwapchains = &m_swapchain;
			present.pImageIndices = &m_imageIndex;
			auto result = vkQueuePresentKHR(m_queue, &present);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
				m_resize = true;
			else
				CheckVK(result, "Present frame");
			// 单帧模式显式等待 present 消费 binary semaphore，下一帧才复用，避免重置时机依赖驱动。
			CheckVK(vkQueueWaitIdle(m_queue), "Wait present semaphore consumption");
			m_pending = false;
		}
		if (vsync != m_vsync)
		{
			m_vsync = vsync;
			m_resize = true;
		}
	}
	void VulkanBackend::CaptureFrame(const std::filesystem::path &path)
	{
		// 离屏颜色图像一直保留 transfer-src 布局，因此截图与交换链是否已 Present 无关。
		// 等 fence 后复制到 coherent readback，输出给现有 WriteBMP 的统一 RGBA 顶行格式。
		if (!m_rendered)
			throw std::runtime_error("Vulkan capture requires a rendered frame");
		CheckVK(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX), "Wait capture frame");
		VkDeviceSize size = static_cast<VkDeviceSize>(m_extent.width) * m_extent.height * 4;
		auto readback = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		auto commands = BeginUpload();
		VkBufferImageCopy copy = {};
		copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy.imageExtent = {m_extent.width, m_extent.height, 1};
		vkCmdCopyImageToBuffer(commands, m_color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1,
							   &copy);
		EndUpload(commands);
		void *mapped;
		CheckVK(vkMapMemory(m_device, readback.memory, 0, size, 0, &mapped), "Map frame capture");
		std::vector<unsigned char> pixels(static_cast<size_t>(size));
		std::memcpy(pixels.data(), mapped, pixels.size());
		vkUnmapMemory(m_device, readback.memory);
		DestroyBuffer(readback);
		if (m_format == VK_FORMAT_B8G8R8A8_UNORM)
			for (size_t i = 0; i < pixels.size(); i += 4)
				std::swap(pixels[i], pixels[i + 2]);
		Platform::WriteBMP(path, m_extent.width, m_extent.height, pixels.data(), false);
		Platform::Log("Captured Vulkan frame: " + path.u8string());
	}
	void VulkanBackend::ShutDown()
	{
		// GPU 完全空闲后按依赖逆序释放。每个句柄归零，析构和显式 ShutDown 可重复调用。
		if (m_device && vkDeviceWaitIdle)
			vkDeviceWaitIdle(m_device);
		if (m_device && vkDestroyImage)
		{
			// GPU 已空闲，先释放引用资源的 Set，再释放纹理、Buffer、Sampler。
			for (auto pool : m_materialPools)
				vkDestroyDescriptorPool(m_device, pool, nullptr);
			m_materialPools.clear();
			m_materialSets.clear();
			if (m_descriptorPool)
				vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
			m_descriptorPool = VK_NULL_HANDLE;
			m_sceneSet = m_boundMaterial = VK_NULL_HANDLE;
			m_passSets[0] = m_passSets[1] = VK_NULL_HANDLE;
			DestroySwapchain();
			for (auto &entry : m_meshes)
			{
				DestroyBuffer(entry.second.vertices);
				DestroyBuffer(entry.second.indices);
			}
			m_meshes.clear();
			for (auto &entry : m_textures)
				DestroyImage(entry.second);
			m_textures.clear();
			for (auto &entry : m_cubes)
				DestroyImage(entry.second);
			m_cubes.clear();
			DestroyImage(m_blackCube);
			if (m_frameData)
				vkUnmapMemory(m_device, m_frameConstants.memory);
			if (m_objectData)
				vkUnmapMemory(m_device, m_objects.memory);
			m_frameData = nullptr;
			m_objectData = nullptr;
			DestroyBuffer(m_frameConstants);
			DestroyBuffer(m_objects);
			DestroyBuffer(m_primitiveIds);
			if (m_queries)
				vkDestroyQueryPool(m_device, m_queries, nullptr);
			m_queries = VK_NULL_HANDLE;
			if (m_sampler)
				vkDestroySampler(m_device, m_sampler, nullptr);
			m_sampler = VK_NULL_HANDLE;
			if (m_cubeSampler)
				vkDestroySampler(m_device, m_cubeSampler, nullptr);
			m_cubeSampler = VK_NULL_HANDLE;
			if (m_pipelineLayout)
				vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
			m_pipelineLayout = VK_NULL_HANDLE;
			for (auto &layout : m_descriptorLayouts)
			{
				if (layout)
					vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
				layout = VK_NULL_HANDLE;
			}
			if (m_fence)
				vkDestroyFence(m_device, m_fence, nullptr);
			m_fence = VK_NULL_HANDLE;
			if (m_acquired)
				vkDestroySemaphore(m_device, m_acquired, nullptr);
			m_acquired = VK_NULL_HANDLE;
			if (m_finished)
				vkDestroySemaphore(m_device, m_finished, nullptr);
			m_finished = VK_NULL_HANDLE;
			if (m_commandPool)
				vkDestroyCommandPool(m_device, m_commandPool, nullptr);
			m_commandPool = VK_NULL_HANDLE;
			m_commands = VK_NULL_HANDLE;
		}
		if (m_device && vkDestroyDevice)
			vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
		if (m_surface && vkDestroySurfaceKHR)
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
		if (m_instance && vkDestroyInstance)
			vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
		if (m_loader)
			FreeLibrary(m_loader);
		m_loader = nullptr;
		m_pending = m_rendered = false;
		m_gpu = VK_NULL_HANDLE;
	}
	std::unique_ptr<IRenderBackend> CreateVulkanBackend()
	{
		return std::make_unique<VulkanBackend>();
	}
} // namespace Rendering
