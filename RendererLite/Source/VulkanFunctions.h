#pragma once
// 入口从 vulkan-1.dll 取，声明和加载共用下面这两份列表。
#define LITE_VK_INSTANCE(X) \
 X(DestroyInstance) \
 X(CreateWin32SurfaceKHR) X(DestroySurfaceKHR) \
 X(EnumeratePhysicalDevices) X(GetPhysicalDeviceProperties) X(GetPhysicalDeviceFeatures2) \
 X(GetPhysicalDeviceQueueFamilyProperties) X(GetPhysicalDeviceSurfaceSupportKHR) \
 X(GetPhysicalDeviceSurfaceCapabilitiesKHR) X(GetPhysicalDeviceSurfaceFormatsKHR) \
 X(GetPhysicalDeviceSurfacePresentModesKHR) X(GetPhysicalDeviceMemoryProperties) \
 X(GetPhysicalDeviceFormatProperties) X(EnumerateDeviceExtensionProperties) \
 X(CreateDevice) X(GetDeviceProcAddr)

// device / swapchain / 同步 / 命令 / 资源
#define LITE_VK_DEVICE(X) \
 X(DestroyDevice) X(GetDeviceQueue) X(DeviceWaitIdle) \
 X(CreateSwapchainKHR) X(DestroySwapchainKHR) \
 X(GetSwapchainImagesKHR) X(AcquireNextImageKHR) X(QueuePresentKHR) \
 X(CreateSemaphore) X(DestroySemaphore) X(WaitSemaphores) X(QueueSubmit2) \
 X(CreateFence) X(DestroyFence) X(WaitForFences) X(ResetFences) \
 X(CreateCommandPool) X(DestroyCommandPool) X(AllocateCommandBuffers) X(ResetCommandPool) \
 X(BeginCommandBuffer) X(EndCommandBuffer) X(CmdPipelineBarrier2) X(CmdBeginRendering) X(CmdEndRendering) \
 X(CreateBuffer) X(DestroyBuffer) X(GetBufferMemoryRequirements) X(AllocateMemory) X(FreeMemory) \
 X(BindBufferMemory) X(MapMemory) X(UnmapMemory) X(CreateImage) X(DestroyImage) \
 X(GetImageMemoryRequirements) X(BindImageMemory) X(CreateImageView) X(DestroyImageView) \
 X(CreateDescriptorSetLayout) X(DestroyDescriptorSetLayout) X(CreateDescriptorPool) X(DestroyDescriptorPool) \
 X(AllocateDescriptorSets) X(UpdateDescriptorSets) X(CreatePipelineLayout) X(DestroyPipelineLayout) \
 X(CreateShaderModule) X(DestroyShaderModule) X(CreateGraphicsPipelines) X(DestroyPipeline) \
 X(CmdBindPipeline) X(CmdBindDescriptorSets) X(CmdBindVertexBuffers) X(CmdBindIndexBuffer) \
 X(CmdSetViewport) X(CmdSetScissor) X(CmdPushConstants) X(CmdDrawIndexed) X(CmdCopyImageToBuffer)
