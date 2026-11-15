#include "Engine/Director.h"
#include "Engine/SceneFile.h"
#include "Engine/InputManager.h"
#include "GameWorld.h"
#include "Platform/Log.h"
#include "Platform/Paths.h"
#include <iostream>
#include <stdexcept>
#include <memory>
int wmain(int i_argc, wchar_t **i_argv)
{
	int result = 0;
	try
	{
		for (int i = 1; i < i_argc; ++i)
			if (std::wstring(i_argv[i]) == L"--help")
			{
				std::cout
					<< "MyEngine: --renderer opengl|dx11|dx12|vulkan --scene rendering|game|sponza|sibenik --model "
					   "Models/file.obj --scene-file Assets/Scenes/EditorDemo.lua --instances N --frames N --capture "
					   "file.bmp "
					   "--log file.log --no-vsync --no-shadows --profile --autoplay --warp --width N --height N "
					   "--samples 1|2|4|8\n";
				return 0;
			}
		auto settings = MyEngine::LoadSettings();
		auto log = Platform::GetWritableDirectory() / L"engine-latest.log";
		for (int i = 1; i < i_argc; ++i)
		{
			std::wstring arg = i_argv[i];
			auto value = [&]()
			{
				if (i + 1 >= i_argc)
					throw std::invalid_argument("Missing command-line value");
				return std::wstring(i_argv[++i]);
			};
			if (arg == L"--renderer")
				settings.m_renderer = Platform::ToUTF8(value());
			else if (arg == L"--scene")
				settings.m_scene = Platform::ToUTF8(value());
			else if (arg == L"--scene-file")
			{
				// 场景文件本身可由命令行指定；文件内资源仍严格相对 Assets。
				settings.m_sceneFile = std::filesystem::absolute(value());
			}
			else if (arg == L"--model")
			{
				settings.m_model = Platform::ToUTF8(value());
				settings.m_scene = "model";
			}
			else if (arg == L"--instances")
				settings.m_instances = std::stoi(value());
			else if (arg == L"--frames")
			{
				settings.m_frameLimit = std::stoi(value());
				if (settings.m_frameLimit < 1)
					throw std::invalid_argument("--frames must be positive");
			}
			else if (arg == L"--capture")
				settings.m_capture = std::filesystem::absolute(value());
			else if (arg == L"--log")
				log = std::filesystem::absolute(value());
			else if (arg == L"--width")
				settings.m_width = std::stoi(value());
			else if (arg == L"--height")
				settings.m_height = std::stoi(value());
			else if (arg == L"--samples")
				settings.m_samples = std::stoi(value());
			else if (arg == L"--no-vsync")
				settings.m_vsync = false;
			else if (arg == L"--no-shadows")
				settings.m_shadows = false;
			else if (arg == L"--profile")
				settings.m_profile = true;
			else if (arg == L"--autoplay")
				settings.m_autoplay = true;
			else if (arg == L"--warp")
				settings.m_warp = true;
			else
				throw std::invalid_argument("Unknown command-line option");
		}
		if (settings.m_renderer != "opengl" && settings.m_renderer != "dx11" && settings.m_renderer != "dx12" &&
			settings.m_renderer != "vulkan")
			throw std::invalid_argument("Renderer must be opengl, dx11, dx12 or vulkan");
		if (settings.m_scene != "rendering" && settings.m_scene != "game" && settings.m_scene != "sponza" &&
			settings.m_scene != "sibenik" && settings.m_scene != "model")
			throw std::invalid_argument("Scene must be rendering, game, sponza, sibenik or model");
		if (settings.m_sceneFile.empty() && settings.m_scene == "model" && settings.m_model.empty())
			throw std::invalid_argument("Model scene requires --model Models/file.obj");
		if (settings.m_instances < 1 || settings.m_instances > 256)
			throw std::invalid_argument("Instances must be between 1 and 256");
		if (settings.m_width < 320 || settings.m_width > 7680 || settings.m_height < 240 || settings.m_height > 4320)
			throw std::invalid_argument("Window dimensions out of supported range");
		if (settings.m_samples != 1 && settings.m_samples != 2 && settings.m_samples != 4 && settings.m_samples != 8)
			throw std::invalid_argument("Samples must be 1, 2, 4 or 8");
		if (settings.m_warp && settings.m_renderer != "dx11" && settings.m_renderer != "dx12")
			throw std::invalid_argument("--warp requires --renderer dx11 or dx12");
		Platform::InitLog(log);
		Platform::Log("Assets: " + Platform::GetAssetPath("").u8string());
		Platform::Log("Log: " + log.u8string());
		{
			MyEngine::Director director(settings);
			director.Initilize();
			if (!settings.m_sceneFile.empty())
			{
				// 读取完整的新场景和资源后才替换；独立 Demo 不包含 Editor。
				auto loaded = MyEngine::LoadSceneFile(settings.m_sceneFile);
				director.ApplySceneFileSettings(loaded.settings);
				director.pushScene(std::move(loaded.scene));
				Platform::Log("Loaded scene file: " + settings.m_sceneFile.u8string());
			}
			else
				director.pushScene(std::make_unique<GameWorld>(settings));
			director.run();
		}
		MyEngine::Actor::releaseMemoryPool();
		Platform::Log("Shutdown: scene, GPU resources and actor pools released");
	}
	catch (const std::exception &error)
	{
		Platform::Log(std::string("ERROR: ") + error.what());
		result = 1;
	}
	MyEngine::InputManager::DestroyInstance();
	return result;
}
