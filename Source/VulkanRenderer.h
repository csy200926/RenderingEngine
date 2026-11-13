#pragma once
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "IRenderer.h"
#include <vulkan/vulkan.h>
#include "VulkanFunctions.h"
#include <array>
#include <stdexcept>

namespace Lite
{
	class VulkanRenderer final : public IRenderer
	{
	  public:
		~VulkanRenderer() override;
		void Initialize(HWND, unsigned, unsigned, const MeshData &) override;
		void Resize(unsigned, unsigned) override;
		void Render(const FrameData &, bool) override;
		void Capture(const std::filesystem::path &) override;
		void WaitIdle() override;

	  private:
		struct Buffer
		{
			VkBuffer handle = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			void *mapped = nullptr;
		};
		struct Image
		{
			VkImage handle = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
		};
		struct Frame
		{
			VkCommandPool pool = VK_NULL_HANDLE;
			VkCommandBuffer command = VK_NULL_HANDLE;
			VkSemaphore acquired = VK_NULL_HANDLE;
			VkDescriptorSet descriptor = VK_NULL_HANDLE;
			Buffer uniform, readback;
			Image depth;
			uint64_t finished = 0;
		};
		struct SwapImage
		{
			VkImage handle = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkSemaphore presentDone = VK_NULL_HANDLE;
			VkFence presentFence = VK_NULL_HANDLE;
			bool presentPending = false;
		};
		static void Check(VkResult, const char *);
		uint32_t MemoryType(uint32_t, VkMemoryPropertyFlags);
		void CreateBuffer(VkDeviceSize, VkBufferUsageFlags, Buffer &);
		void DestroyBuffer(Buffer &);
		void CreateDepth(Image &);
		void DestroyImage(Image &);
		void CreateSwapchain();
		void DestroySwapchain();
		void CreatePipeline();
		void WaitFrame(const Frame &);
		VkResult WaitPresentations();
		HMODULE m_library = nullptr;
		HWND m_window = nullptr;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_physical = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_queue = VK_NULL_HANDLE;
		uint32_t m_family = 0;
		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
		VkFormat m_format = VK_FORMAT_UNDEFINED;
		VkExtent2D m_extent{};
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_layout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
		VkSemaphore m_timeline = VK_NULL_HANDLE;
		uint64_t m_serial = 0;
		unsigned m_width = 0, m_height = 0, m_current = 0, m_latest = 0;
		bool m_vsync = true, m_canCapture = false, m_hasFrame = false;
		Buffer m_vertices, m_indices;
		std::array<Frame, 2> m_frames{};
		std::vector<SwapImage> m_images;
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
		PFN_vkCreateInstance vkCreateInstance = nullptr;
		PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = nullptr;
#define DECLARE(name) PFN_vk##name vk##name = nullptr;
		LITE_VK_INSTANCE(DECLARE)
		LITE_VK_DEVICE(DECLARE)
#undef DECLARE
	};
} // namespace Lite
