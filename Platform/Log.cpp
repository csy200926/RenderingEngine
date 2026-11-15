#include "Log.h"
#include <fstream>
#include <iostream>
#include <mutex>
#include <windows.h>
namespace Platform
{
	static std::ofstream s_file;
	static std::mutex s_mutex;
	void InitLog(const std::filesystem::path &i_path)
	{
		if (i_path.has_parent_path())
			std::filesystem::create_directories(i_path.parent_path());
		s_file.open(i_path);
		if (!s_file)
			throw std::runtime_error("Cannot create log file");
	}
	void Log(const std::string &i_message)
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		std::cout << i_message << std::endl;
		if (s_file)
		{
			s_file << i_message << std::endl;
		}
		OutputDebugStringA((i_message + "\n").c_str());
	}
} // namespace Platform
