#pragma once
#include <assimp/IOSystem.hpp>
#include <assimp/IOStream.hpp>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <algorithm>
namespace Rendering
{
	// Keep Assimp's companion-file handling while opening UTF-8 paths through the Windows wide API.
	class AssetIOStream final : public Assimp::IOStream
	{
	  public:
		explicit AssetIOStream(FILE *i_file) : m_file(i_file)
		{
			_fseeki64(m_file, 0, SEEK_END);
			m_size = static_cast<size_t>(_ftelli64(m_file));
			_fseeki64(m_file, 0, SEEK_SET);
		}
		~AssetIOStream() override { fclose(m_file); }
		size_t Read(void *i_buffer, size_t i_size, size_t i_count) override
		{
			return fread(i_buffer, i_size, i_count, m_file);
		}
		size_t Write(const void *, size_t, size_t) override { return 0; }
		aiReturn Seek(size_t i_offset, aiOrigin i_origin) override
		{
			int origin = i_origin == aiOrigin_SET ? SEEK_SET : (i_origin == aiOrigin_CUR ? SEEK_CUR : SEEK_END);
			return _fseeki64(m_file, static_cast<__int64>(i_offset), origin) == 0 ? aiReturn_SUCCESS : aiReturn_FAILURE;
		}
		size_t Tell() const override { return static_cast<size_t>(_ftelli64(m_file)); }
		size_t FileSize() const override { return m_size; }
		void Flush() override {}

	  private:
		FILE *m_file;
		size_t m_size = 0;
	};
	class AssetIOSystem final : public Assimp::IOSystem
	{
	  public:
		explicit AssetIOSystem(std::filesystem::path i_directory,
							   std::vector<std::filesystem::path> *o_dependencies = nullptr)
			: m_directory(std::move(i_directory)), m_dependencies(o_dependencies) {}
		bool Exists(const char *i_path) const override
		{
			std::error_code error;
			return std::filesystem::is_regular_file(Resolve(i_path), error);
		}
		char getOsSeparator() const override { return '/'; }
		Assimp::IOStream *Open(const char *i_path, const char *i_mode = "rb") override
		{
			if (!i_mode || i_mode[0] != 'r')
				return nullptr;
			FILE *file = nullptr;
			auto path = Resolve(i_path);
			if (_wfopen_s(&file, path.c_str(), L"rb") || !file)
				return nullptr;
			// 只记录真正打开的伴随文件，Assimp 的 Exists/Open 探测失败不算资源缺失。
			if (m_dependencies && std::find(m_dependencies->begin(), m_dependencies->end(), path) == m_dependencies->end())
				m_dependencies->push_back(path);
			return new AssetIOStream(file);
		}
		void Close(Assimp::IOStream *i_stream) override { delete i_stream; }

	  private:
		std::filesystem::path Resolve(const char *i_path) const
		{
			auto path = std::filesystem::u8path(i_path);
			return (path.is_absolute() ? path : m_directory / path).lexically_normal();
		}
		std::filesystem::path m_directory;
		std::vector<std::filesystem::path> *m_dependencies = nullptr;
	};
} // namespace Rendering
