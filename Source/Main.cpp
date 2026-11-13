#include "Window.h"
#include "IRenderer.h"
#include "Files.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cmath>

int wmain(int i_argc, wchar_t **i_argv)
{
	try
	{
		std::wstring api = L"vulkan";
		std::filesystem::path capture, log;
		unsigned width = 1280, height = 800;
		int frames = 0;
		bool hidden = false, vsync = true, animate = false;
		for (int i = 1; i < i_argc; ++i)
		{
			std::wstring arg = i_argv[i];
			auto value = [&]()
			{
				if (i + 1 >= i_argc)
					throw std::invalid_argument("Missing command-line value");
				return std::wstring(i_argv[++i]);
			};
			if (arg == L"--help")
			{
				std::cout << "RendererLite --renderer vulkan|dx12 --frames N --capture file.bmp --log file.log\n"
							 "--width N --height N --hidden --no-vsync --animate\n"
							 "WASD: move, Q/E: down/up, Shift: faster, right mouse: look, Esc: exit\n";
				return 0;
			}
			else if (arg == L"--renderer")
				api = value();
			else if (arg == L"--frames")
			{
				frames = std::stoi(value());
				if (frames < 1)
					throw std::invalid_argument("Frames must be positive");
			}
			else if (arg == L"--width")
			{
				auto n = std::stoi(value());
				if (n < 320 || n > 7680)
					throw std::invalid_argument("Invalid width");
				width = n;
			}
			else if (arg == L"--height")
			{
				auto n = std::stoi(value());
				if (n < 240 || n > 4320)
					throw std::invalid_argument("Invalid height");
				height = n;
			}
			else if (arg == L"--capture")
				capture = std::filesystem::absolute(value());
			else if (arg == L"--log")
				log = std::filesystem::absolute(value());
			else if (arg == L"--hidden")
				hidden = true;
			else if (arg == L"--no-vsync")
				vsync = false;
			else if (arg == L"--animate")
				animate = true;
			else
				throw std::invalid_argument("Unknown command-line option");
		}
		if (api != L"vulkan" && api != L"dx12")
			throw std::invalid_argument("Renderer must be vulkan or dx12");
		if (hidden && frames == 0)
			throw std::invalid_argument("Hidden mode requires --frames to exit automatically");
		if (!log.empty())
			Lite::SetLogFile(log);
		// 第一步：窗口和 CPU 场景独立创建。小项目没有 Actor/Director/资源管理器。
		Lite::Window window;
		window.Initialize(width, height, hidden,
						  api == L"vulkan" ? L"RendererLite / Vulkan 1.3" : L"RendererLite / DX12");
		auto mesh = Lite::MakeDemoMesh();
		auto renderer = api == L"vulkan" ? Lite::CreateVulkanRenderer() : Lite::CreateDX12Renderer();
		renderer->Initialize(window.m_handle, width, height, mesh);
		window.m_resized = false;
		Lite::Log("Controls: WASD, Q/E, Shift, right mouse; Esc exits");
		glm::vec3 eye(6, 4, 8);
		auto forward = glm::normalize(glm::vec3(0, 1, 0) - eye);
		float yaw = std::atan2(forward.x, -forward.z), pitch = std::asin(forward.y), time = 0;
		auto previous = std::chrono::steady_clock::now();
		int drawn = 0;
		while (window.PumpEvents())
		{
			if (window.m_minimized || !window.m_width || !window.m_height)
			{
				WaitMessage();
				continue;
			}
			if (window.m_resized)
			{
				renderer->Resize(window.m_width, window.m_height);
				window.m_resized = false;
			}
			auto now = std::chrono::steady_clock::now();
			float dt = std::min(.05f, std::chrono::duration<float>(now - previous).count());
			previous = now;
			// 第二步：CPU 更新相机，两个 API 看见完全相同的矩阵和物体列表。
			auto mouse = window.TakeMouseDelta();
			yaw += mouse.x * .003f;
			pitch = glm::clamp(pitch - mouse.y * .003f, -1.5f, 1.5f);
			forward = glm::vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
			if (!hidden && GetForegroundWindow() == window.m_handle)
			{
				auto down = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
				float step = dt * (down(VK_SHIFT) ? 15.f : 4.f);
				auto right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
				if (down('W'))
					eye += forward * step;
				if (down('S'))
					eye -= forward * step;
				if (down('A'))
					eye -= right * step;
				if (down('D'))
					eye += right * step;
				if (down('Q'))
					eye.y -= step;
				if (down('E'))
					eye.y += step;
			}
			if (animate)
				time += dt;
			auto frame = Lite::MakeDemoFrame(eye, forward, float(window.m_width) / window.m_height, time);
			// 第三步：进入后端，顺着 Render 看命令录制 -> 提交 -> Present。
			renderer->Render(frame, vsync);
			++drawn;
			if (frames && drawn >= frames)
				break;
		}
		renderer->WaitIdle();
		if (!capture.empty())
			renderer->Capture(capture);
		Lite::Log("Finished: frames=" + std::to_string(drawn));
		// renderer 比 window 先析构：先释放 GPU 与 surface，再销毁原生窗口。
	}
	catch (const std::exception &error)
	{
		Lite::Log(std::string("ERROR: ") + error.what());
		return 1;
	}
	return 0;
}
