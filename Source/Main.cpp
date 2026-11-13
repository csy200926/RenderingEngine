#include "Window.h"
#include "IRenderer.h"
#include "Files.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cmath>

static std::wstring GetArgument(int i_argc, wchar_t** i_argv, int& io_index)
{
	if(io_index + 1 >= i_argc)
		throw std::invalid_argument("Missing command-line value");
	io_index++;
	return i_argv[io_index];
}

int wmain(int i_argc, wchar_t **i_argv)
{
	try
	{
		std::wstring api = L"vulkan";
		std::filesystem::path capture, log;
		unsigned width = 1280, height = 800;
		int frames = 0;
		bool hidden = false, vsync = true, animate = false;

		for (int i = 1; i < i_argc; i++)
		{
			std::wstring arg = i_argv[i];
			if (arg == L"--help")
			{
				std::cout << "RendererLite --renderer vulkan|dx12 --frames N --capture file.bmp --log file.log\n"
							 "--width N --height N --hidden --no-vsync --animate\n"
							 "WASD: move, Q/E: down/up, Shift: faster, right mouse: look, Esc: exit\n";
				return 0;
			}
			else if (arg == L"--renderer")
				api = GetArgument(i_argc, i_argv, i);
			else if (arg == L"--frames")
			{
				frames = std::stoi(GetArgument(i_argc, i_argv, i));
				if (frames < 1)
					throw std::invalid_argument("Frames must be positive");
			}
			else if (arg == L"--width")
			{
				int n = std::stoi(GetArgument(i_argc, i_argv, i));
				if (n < 320 || n > 7680)
					throw std::invalid_argument("Invalid width");
				width = n;
			}
			else if (arg == L"--height")
			{
				int n = std::stoi(GetArgument(i_argc, i_argv, i));
				if (n < 240 || n > 4320)
					throw std::invalid_argument("Invalid height");
				height = n;
			}
			else if (arg == L"--capture")
				capture = std::filesystem::absolute(GetArgument(i_argc, i_argv, i));
			else if (arg == L"--log")
				log = std::filesystem::absolute(GetArgument(i_argc, i_argv, i));
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
			Rendering::SetLogFile(log);

		Rendering::Window window;
		window.Initialize(width, height, hidden,
						  api == L"vulkan" ? L"RendererLite / Vulkan 1.3" : L"RendererLite / DX12");
		Rendering::MeshData mesh = Rendering::MakeDemoMesh();
		std::unique_ptr<Rendering::IRenderer> pRenderer;
		if(api == L"vulkan")
			pRenderer = Rendering::CreateVulkanRenderer();
		else
			pRenderer = Rendering::CreateDX12Renderer();
		pRenderer->Initialize(window.m_handle, width, height, mesh);
		window.m_resized = false;
		Rendering::Log("Controls: WASD, Q/E, Shift, right mouse; Esc exits");


		Rendering::Camera camera;
		float time = 0;
		std::chrono::steady_clock::time_point previous = std::chrono::steady_clock::now();
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
				pRenderer->Resize(window.m_width, window.m_height);
				window.m_resized = false;
			}

			std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
			float dt = std::min(.05f, std::chrono::duration<float>(now - previous).count());
			previous = now;

			bool keyboard = !hidden && GetForegroundWindow() == window.m_handle;
			camera.Update(dt, window.TakeMouseDelta(), keyboard);
			camera.SetPerspective(55.0f, float(window.m_width) / window.m_height, 0.1f, 100.0f);
			if (animate)
				time += dt;

			Rendering::FrameData frame = Rendering::MakeDemoFrame(camera, time);
			pRenderer->Render(frame, vsync);
			++drawn;
			if (frames && drawn >= frames)
				break;
		}

		pRenderer->WaitIdle();
		if (!capture.empty())
			pRenderer->Capture(capture);
		Rendering::Log("Finished: frames=" + std::to_string(drawn));
		// 先结束 GPU 工作，再销毁窗口。
	}
	catch (const std::exception &error)
	{
		Rendering::Log(std::string("ERROR: ") + error.what());
		return 1;
	}
	return 0;
}
