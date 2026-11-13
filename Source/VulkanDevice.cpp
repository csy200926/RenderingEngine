#include "VulkanRenderer.h"
#include "Files.h"
#include <cstring>
#include <string>

namespace Lite
{
	void VulkanRenderer::Check(VkResult result, const char *operation)
	{
		if (result != VK_SUCCESS)
			throw std::runtime_error(std::string(operation) + " failed: VkResult=" + std::to_string(result));
	}

	void VulkanRenderer::Initialize(HWND window, unsigned width, unsigned height, const MeshData &mesh)
	{
		m_window = window;
		m_width = width;
		m_height = height;
		// 1. 只依赖驱动安装的系统 loader，不要求用户安装 Vulkan SDK。
		m_library = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!m_library)
			throw std::runtime_error("Vulkan loader vulkan-1.dll is unavailable");
		vkGetInstanceProcAddr =
			reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(m_library, "vkGetInstanceProcAddr"));
		if (!vkGetInstanceProcAddr)
			throw std::runtime_error("Missing vkGetInstanceProcAddr");
		vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance"));
		vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
			vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
		uint32_t version = VK_API_VERSION_1_0;
		if (vkEnumerateInstanceVersion)
			Check(vkEnumerateInstanceVersion(&version), "EnumerateInstanceVersion");
		if (!vkCreateInstance || version < VK_API_VERSION_1_3)
			throw std::runtime_error("Vulkan 1.3 loader is required");
		VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
		app.pApplicationName = "RendererLite";
		app.apiVersion = VK_API_VERSION_1_3;
		// maintenance1 的实例依赖也必须启用，不能只启用设备扩展。
		const char *extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
									VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
									VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME};
		auto enumerateExtensions = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
			vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties"));
		if (!enumerateExtensions)
			throw std::runtime_error("Missing vkEnumerateInstanceExtensionProperties");
		uint32_t instanceExtensionCount = 0;
		Check(enumerateExtensions(nullptr, &instanceExtensionCount, nullptr), "InstanceExtensions");
		std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
		Check(enumerateExtensions(nullptr, &instanceExtensionCount, instanceExtensions.data()), "InstanceExtensions");
		for (const char *required : extensions)
		{
			bool found = false;
			for (const auto &available : instanceExtensions)
				found |= std::strcmp(required, available.extensionName) == 0;
			if (!found)
				throw std::runtime_error(std::string("Required Vulkan instance extension unavailable: ") + required);
		}
		VkInstanceCreateInfo instance{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		instance.pApplicationInfo = &app;
		instance.enabledExtensionCount = 4;
		instance.ppEnabledExtensionNames = extensions;
		Check(vkCreateInstance(&instance, nullptr, &m_instance), "CreateInstance");
#define LOAD_INSTANCE(name)                                                                                            \
	vk##name = reinterpret_cast<PFN_vk##name>(vkGetInstanceProcAddr(m_instance, "vk" #name));                          \
	if (!vk##name)                                                                                                     \
		throw std::runtime_error("Missing vk" #name);
		LITE_VK_INSTANCE(LOAD_INSTANCE)
#undef LOAD_INSTANCE
		VkWin32SurfaceCreateInfoKHR surface{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
		surface.hinstance = GetModuleHandleW(nullptr);
		surface.hwnd = window;
		Check(vkCreateWin32SurfaceKHR(m_instance, &surface, nullptr, &m_surface), "CreateWin32Surface");

		// 2. 明确检查版本、必要 feature、交换链扩展，以及同时支持绘制和呈现的队列。
		uint32_t count = 0;
		Check(vkEnumeratePhysicalDevices(m_instance, &count, nullptr), "EnumeratePhysicalDevices");
		std::vector<VkPhysicalDevice> devices(count);
		Check(vkEnumeratePhysicalDevices(m_instance, &count, devices.data()), "EnumeratePhysicalDevices");
		for (auto device : devices)
		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(device, &properties);
			if (properties.apiVersion < VK_API_VERSION_1_3)
				continue;
			VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
			VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenance{
				VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT};
			f13.pNext = &maintenance;
			VkPhysicalDeviceVulkan12Features f12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
			f12.pNext = &f13;
			VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
			features.pNext = &f12;
			vkGetPhysicalDeviceFeatures2(device, &features);
			if (!f12.timelineSemaphore || !f13.dynamicRendering || !f13.synchronization2 ||
				!maintenance.swapchainMaintenance1)
				continue;
			uint32_t extensionCount = 0;
			Check(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr), "DeviceExtensions");
			std::vector<VkExtensionProperties> available(extensionCount);
			Check(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data()),
				  "DeviceExtensions");
			bool swapchain = false, presentFences = false;
			for (auto &extension : available)
			{
				swapchain |= std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
				presentFences |=
					std::strcmp(extension.extensionName, VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME) == 0;
			}
			if (!swapchain || !presentFences)
				continue;
			uint32_t families = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &families, nullptr);
			std::vector<VkQueueFamilyProperties> queues(families);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &families, queues.data());
			for (uint32_t i = 0; i < families; ++i)
			{
				VkBool32 present = false;
				Check(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present), "SurfaceSupport");
				if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present)
				{
					m_physical = device;
					m_family = i;
					Log(std::string("Vulkan GPU: ") + properties.deviceName);
					break;
				}
			}
			if (m_physical)
				break;
		}
		if (!m_physical)
			throw std::runtime_error(
				"Need Vulkan 1.3 + dynamicRendering + synchronization2 + timelineSemaphore + graphics/present queue + "
				"VK_EXT_swapchain_maintenance1 (swapchainMaintenance1 feature)");
		VkFormatProperties depthProperties{};
		vkGetPhysicalDeviceFormatProperties(m_physical, VK_FORMAT_D32_SFLOAT, &depthProperties);
		if (!(depthProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
			throw std::runtime_error("D32_SFLOAT depth attachment is unsupported");
		float priority = 1;
		VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
		queue.queueFamilyIndex = m_family;
		queue.queueCount = 1;
		queue.pQueuePriorities = &priority;
		VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
		f13.dynamicRendering = f13.synchronization2 = true;
		VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenance{
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT};
		maintenance.swapchainMaintenance1 = true;
		f13.pNext = &maintenance;
		VkPhysicalDeviceVulkan12Features f12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
		f12.pNext = &f13;
		f12.timelineSemaphore = true;
		const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
										  VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};
		VkDeviceCreateInfo device{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
		device.pNext = &f12;
		device.queueCreateInfoCount = 1;
		device.pQueueCreateInfos = &queue;
		device.enabledExtensionCount = 2;
		device.ppEnabledExtensionNames = deviceExtensions;
		Check(vkCreateDevice(m_physical, &device, nullptr, &m_device), "CreateDevice");
#define LOAD_DEVICE(name)                                                                                              \
	vk##name = reinterpret_cast<PFN_vk##name>(vkGetDeviceProcAddr(m_device, "vk" #name));                              \
	if (!vk##name)                                                                                                     \
		throw std::runtime_error("Missing vk" #name);
		LITE_VK_DEVICE(LOAD_DEVICE)
#undef LOAD_DEVICE
		vkGetDeviceQueue(m_device, m_family, 0, &m_queue);
		VkSemaphoreTypeCreateInfo timelineType{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
		timelineType.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		semaphore.pNext = &timelineType;
		Check(vkCreateSemaphore(m_device, &semaphore, nullptr, &m_timeline), "CreateTimeline");
		semaphore.pNext = nullptr;

		// 3. 帧数据和命令池各有两份，CPU 仅在 timeline 达到该帧序号后重用。
		VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
											 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
		VkDescriptorSetLayoutCreateInfo descriptor{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		descriptor.bindingCount = 1;
		descriptor.pBindings = &binding;
		Check(vkCreateDescriptorSetLayout(m_device, &descriptor, nullptr, &m_descriptorLayout), "DescriptorLayout");
		VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2};
		VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
		pool.maxSets = 2;
		pool.poolSizeCount = 1;
		pool.pPoolSizes = &size;
		Check(vkCreateDescriptorPool(m_device, &pool, nullptr, &m_descriptorPool), "DescriptorPool");
		for (auto &frame : m_frames)
		{
			VkCommandPoolCreateInfo commands{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
			commands.queueFamilyIndex = m_family;
			Check(vkCreateCommandPool(m_device, &commands, nullptr, &frame.pool), "CommandPool");
			VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			allocate.commandPool = frame.pool;
			allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocate.commandBufferCount = 1;
			Check(vkAllocateCommandBuffers(m_device, &allocate, &frame.command), "CommandBuffer");
			Check(vkCreateSemaphore(m_device, &semaphore, nullptr, &frame.acquired), "AcquireSemaphore");
			CreateBuffer(sizeof(FrameConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, frame.uniform);
			VkDescriptorSetAllocateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
			set.descriptorPool = m_descriptorPool;
			set.descriptorSetCount = 1;
			set.pSetLayouts = &m_descriptorLayout;
			Check(vkAllocateDescriptorSets(m_device, &set, &frame.descriptor), "DescriptorSet");
			VkDescriptorBufferInfo buffer{frame.uniform.handle, 0, sizeof(FrameConstants)};
			VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			write.dstSet = frame.descriptor;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.pBufferInfo = &buffer;
			vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
		}
		// 小型静态示例直接使用 host-visible coherent 网格，避免上传队列/暂存资源干扰阅读。
		CreateBuffer(mesh.m_vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vertices);
		CreateBuffer(mesh.m_indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_indices);
		std::memcpy(m_vertices.mapped, mesh.m_vertices.data(), mesh.m_vertices.size() * sizeof(Vertex));
		std::memcpy(m_indices.mapped, mesh.m_indices.data(), mesh.m_indices.size() * sizeof(uint32_t));
		CreateSwapchain();
	}

	void VulkanRenderer::WaitIdle()
	{
		if (m_device)
			Check(vkDeviceWaitIdle(m_device), "DeviceWaitIdle");
		Check(WaitPresentations(), "WaitPresentFences");
	}
	VkResult VulkanRenderer::WaitPresentations()
	{
		// Timeline/DeviceWaitIdle 证明绘制完成；present fence 另行证明呈现资源可销毁。
		// fence 不承诺像素已显示，只承诺交换链/等待信号量的生命周期已安全。
		for (auto &image : m_images)
		{
			if (!image.presentPending)
				continue;
			VkResult result = vkWaitForFences(m_device, 1, &image.presentFence, true, UINT64_MAX);
			if (result != VK_SUCCESS)
				return result;
			image.presentPending = false;
		}
		return VK_SUCCESS;
	}
	VulkanRenderer::~VulkanRenderer()
	{
		// 析构不抛异常；也覆盖 Initialize 中途失败时已经建立的对象。
		if (m_device && vkDeviceWaitIdle)
			vkDeviceWaitIdle(m_device);
		// 析构仍不抛异常；设备丢失时等待返回错误，继续释放失效设备的对象。
		WaitPresentations();
		DestroySwapchain();
		if (m_device)
		{
			DestroyBuffer(m_vertices);
			DestroyBuffer(m_indices);
			for (auto &frame : m_frames)
			{
				DestroyBuffer(frame.uniform);
				if (frame.pool)
					vkDestroyCommandPool(m_device, frame.pool, nullptr);
				if (frame.acquired)
					vkDestroySemaphore(m_device, frame.acquired, nullptr);
			}
			if (m_layout)
				vkDestroyPipelineLayout(m_device, m_layout, nullptr);
			if (m_descriptorPool)
				vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
			if (m_descriptorLayout)
				vkDestroyDescriptorSetLayout(m_device, m_descriptorLayout, nullptr);
			if (m_timeline)
				vkDestroySemaphore(m_device, m_timeline, nullptr);
			if (vkDestroyDevice)
				vkDestroyDevice(m_device, nullptr);
		}
		if (m_surface && vkDestroySurfaceKHR)
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		if (m_instance && vkDestroyInstance)
			vkDestroyInstance(m_instance, nullptr);
		if (m_library)
			FreeLibrary(m_library);
	}
	std::unique_ptr<IRenderer> CreateVulkanRenderer()
	{
		return std::make_unique<VulkanRenderer>();
	}
} // namespace Lite
