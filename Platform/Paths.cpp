#include "Paths.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <stdexcept>
namespace Platform
{
	std::filesystem::path GetExecutableDirectory()
	{
		std::vector<wchar_t> buffer(32768);
		DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (!size || size == buffer.size())
			throw std::runtime_error("Cannot locate executable");
		return std::filesystem::path(std::wstring(buffer.data(), size)).parent_path();
	}
	std::filesystem::path GetAssetPath(const std::string &i_path)
	{
		return GetExecutableDirectory() / L"Assets" / std::filesystem::u8path(i_path);
	}
	std::filesystem::path GetWritableDirectory()
	{
		PWSTR pPath = nullptr;
		if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &pPath)))
			throw std::runtime_error("Cannot locate writable application folder");
		std::filesystem::path result = std::filesystem::path(pPath) / L"MyEngine";
		CoTaskMemFree(pPath);
		std::filesystem::create_directories(result);
		return result;
	}
	std::vector<unsigned char> ReadBytes(const std::filesystem::path &i_path)
	{
		std::ifstream file(i_path, std::ios::binary | std::ios::ate);
		if (!file)
			throw std::runtime_error("Cannot open file: " + i_path.u8string());
		auto length = file.tellg();
		if (length < 0)
			throw std::runtime_error("Cannot read file: " + i_path.u8string());
		std::vector<unsigned char> result(static_cast<size_t>(length));
		file.seekg(0);
		if (length && !file.read(reinterpret_cast<char *>(result.data()), length))
			throw std::runtime_error("Incomplete file read: " + i_path.u8string());
		return result;
	}
	std::string ReadText(const std::filesystem::path &i_path)
	{
		auto bytes = ReadBytes(i_path);
		return std::string(bytes.begin(), bytes.end());
	}
	std::wstring ToWide(const std::string &i_utf8)
	{
		if (i_utf8.empty())
			return {};
		int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, i_utf8.data(), static_cast<int>(i_utf8.size()),
										nullptr, 0);
		if (!count)
			throw std::invalid_argument("Invalid UTF-8 string");
		std::wstring result(count, L'\0');
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, i_utf8.data(), static_cast<int>(i_utf8.size()),
							result.data(), count);
		return result;
	}
	std::string ToUTF8(const std::wstring &i_wide)
	{
		if (i_wide.empty())
			return {};
		int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, i_wide.data(), static_cast<int>(i_wide.size()),
										nullptr, 0, nullptr, nullptr);
		if (!count)
			throw std::invalid_argument("Invalid UTF-16 string");
		std::string result(count, '\0');
		WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, i_wide.data(), static_cast<int>(i_wide.size()),
							result.data(), count, nullptr, nullptr);
		return result;
	}
	void WriteBMP(const std::filesystem::path &i_path, int i_width, int i_height, const unsigned char *i_rgba,
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
		info.biWidth = i_width;
		info.biHeight = -i_height;
		info.biPlanes = 1;
		info.biBitCount = 32;
		info.biCompression = BI_RGB;
		std::ofstream file(i_path, std::ios::binary);
		file.write(reinterpret_cast<char *>(&header), sizeof(header));
		file.write(reinterpret_cast<char *>(&info), sizeof(info));
		std::vector<unsigned char> row(i_width * 4);
		for (int y = 0; y < i_height; ++y)
		{
			const auto *pSource = i_rgba + (i_bottomUp ? i_height - 1 - y : y) * i_width * 4;
			for (int x = 0; x < i_width; ++x)
			{
				row[x * 4] = pSource[x * 4 + 2];
				row[x * 4 + 1] = pSource[x * 4 + 1];
				row[x * 4 + 2] = pSource[x * 4];
				row[x * 4 + 3] = 255;
			}
			file.write(reinterpret_cast<char *>(row.data()), row.size());
		}
		if (!file)
			throw std::runtime_error("Cannot write frame capture");
	}
} // namespace Platform
