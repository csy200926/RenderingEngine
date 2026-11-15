#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace Platform
{
	std::filesystem::path GetExecutableDirectory();
	std::filesystem::path GetAssetPath(const std::string &i_path);
	std::filesystem::path GetWritableDirectory();
	std::vector<unsigned char> ReadBytes(const std::filesystem::path &i_path);
	std::string ReadText(const std::filesystem::path &i_path);
	std::wstring ToWide(const std::string &i_utf8);
	std::string ToUTF8(const std::wstring &i_wide);
	void WriteBMP(const std::filesystem::path &i_path, int i_width, int i_height, const unsigned char *i_rgba,
				  bool i_bottomUp);
} // namespace Platform
