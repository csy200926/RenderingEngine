#include "EditorApplication.h"
#include "Platform/Log.h"
#include "Platform/Paths.h"
#include <iostream>

int wmain(int i_argc, wchar_t **i_argv)
{
	try
	{
		// 编辑器是单独的程序；这里只处理入口参数，游戏 Demo 不会链接 Editor。
		std::string renderer = "opengl";
		int frames = 0;
		std::filesystem::path capture, scene;
		for (int i = 1; i < i_argc; ++i)
		{
			std::wstring arg = i_argv[i];
			if (arg == L"--help")
			{
				std::cout
					<< "Editor --renderer opengl|dx11|dx12|vulkan --scene-file path --frames N --capture path.bmp\n";
				return 0;
			}
			if (i + 1 >= i_argc)
				throw std::invalid_argument("Missing argument value");
			std::wstring value = i_argv[++i];
			if (arg == L"--renderer")
				renderer = Platform::ToUTF8(value);
			else if (arg == L"--frames")
			{
				frames = std::stoi(value);
				if (frames < 1)
					throw std::invalid_argument("Frames must be positive");
			}
			else if (arg == L"--capture")
				capture = std::filesystem::absolute(value);
			else if (arg == L"--scene-file")
				scene = std::filesystem::absolute(value);
			else
				throw std::invalid_argument("Unknown editor argument");
		}
		Platform::InitLog(Platform::GetWritableDirectory() / L"editor-latest.log");
		Editor::EditorApplication app;
		return app.Run(renderer, frames, capture, scene);
	}
	catch (const std::exception &error)
	{
		Platform::Log(std::string("EDITOR ERROR: ") + error.what());
		std::cerr << error.what() << '\n';
		return 1;
	}
}
