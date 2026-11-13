#include "Files.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace Lite
{
	static std::ofstream s_log;
	std::filesystem::path ShaderPath(const char *i_name)
	{
		std::vector<wchar_t> name(32768);
		auto length = GetModuleFileNameW(nullptr, name.data(), static_cast<DWORD>(name.size()));
		if (!length || length >= name.size())
			throw std::runtime_error("Cannot locate executable");
		return std::filesystem::path(std::wstring(name.data(), length)).parent_path() / L"Shaders" / i_name;
	}
	std::vector<unsigned char> ReadBytes(const std::filesystem::path &i_path)
	{
		std::ifstream file(i_path, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error("Cannot open file: " + i_path.u8string());
		auto length = file.tellg();
		if (length < 0)
			throw std::runtime_error("Cannot get file size");
		std::vector<unsigned char> result(static_cast<size_t>(length));
		file.seekg(0);
		if (length && !file.read(reinterpret_cast<char *>(result.data()), length))
			throw std::runtime_error("Incomplete file read");
		return result;
	}
	std::string ReadText(const std::filesystem::path &i_path)
	{
		auto bytes = ReadBytes(i_path);
		return std::string(bytes.begin(), bytes.end());
	}
	void SetLogFile(const std::filesystem::path &i_path)
	{
		if (i_path.has_parent_path())
			std::filesystem::create_directories(i_path.parent_path());
		s_log.open(i_path);
		if (!s_log)
			throw std::runtime_error("Cannot create log");
	}
	void Log(const std::string &i_message)
	{
		std::cout << i_message << std::endl;
		if (s_log)
			s_log << i_message << std::endl;
	}
	void WriteBMP(const std::filesystem::path &i_path, unsigned i_width, unsigned i_height, const unsigned char *i_rgba,
				  bool i_bottomUp)
	{
		if (i_path.has_parent_path())
			std::filesystem::create_directories(i_path.parent_path());
		BITMAPFILEHEADER header = {};
		BITMAPINFOHEADER info = {};
		header.bfType = 0x4d42;
		header.bfOffBits = sizeof(header) + sizeof(info);
		header.bfSize = header.bfOffBits + i_width * i_height * 4;
		info.biSize = sizeof(info);
		info.biWidth = static_cast<LONG>(i_width);
		info.biHeight = -static_cast<LONG>(i_height);
		info.biPlanes = 1;
		info.biBitCount = 32;
		std::ofstream file(i_path, std::ios::binary);
		file.write(reinterpret_cast<const char *>(&header), sizeof(header));
		file.write(reinterpret_cast<const char *>(&info), sizeof(info));
		std::vector<unsigned char> row(i_width * 4);
		for (unsigned y = 0; y < i_height; ++y)
		{
			auto source = i_rgba + (i_bottomUp ? i_height - 1 - y : y) * i_width * 4;
			for (unsigned x = 0; x < i_width; ++x)
			{
				row[4 * x] = source[4 * x + 2];
				row[4 * x + 1] = source[4 * x + 1];
				row[4 * x + 2] = source[4 * x];
				row[4 * x + 3] = 255;
			}
			file.write(reinterpret_cast<const char *>(row.data()), row.size());
		}
		if (!file)
			throw std::runtime_error("Cannot write BMP");
		Log("Captured: " + i_path.u8string());
	}
} // namespace Lite
