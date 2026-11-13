#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace Lite
{
	// 所有运行时文件相对于 exe，换工作目录不会找错 Shader。
	std::filesystem::path ShaderPath(const char *i_name);
	std::vector<unsigned char> ReadBytes(const std::filesystem::path &i_path);
	std::string ReadText(const std::filesystem::path &i_path);
	void WriteBMP(const std::filesystem::path &i_path, unsigned i_width, unsigned i_height, const unsigned char *i_rgba,
				  bool i_bottomUp = false);
	void SetLogFile(const std::filesystem::path &i_path);
	void Log(const std::string &i_message);
} // namespace Lite
