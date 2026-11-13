#pragma once
#include "Scene.h"
#include <windows.h>
#include <filesystem>
#include <memory>

namespace Lite
{
	// 只统一应用需要的五个动作，不把 Vulkan/DX12 的类型藏进一套大框架。
	class IRenderer
	{
	  public:
		virtual ~IRenderer() = default;
		virtual void Initialize(HWND i_window, unsigned i_width, unsigned i_height, const MeshData &i_mesh) = 0;
		virtual void Resize(unsigned i_width, unsigned i_height) = 0;
		virtual void Render(const FrameData &i_frame, bool i_vsync) = 0;
		virtual void Capture(const std::filesystem::path &i_path) = 0;
		virtual void WaitIdle() = 0;
	};
	std::unique_ptr<IRenderer> CreateVulkanRenderer();
	std::unique_ptr<IRenderer> CreateDX12Renderer();
} // namespace Lite
