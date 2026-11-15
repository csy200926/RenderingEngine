#pragma once
#include <filesystem>
#include <string>
namespace Platform
{
	void InitLog(const std::filesystem::path &i_path);
	void Log(const std::string &i_message);
} // namespace Platform
