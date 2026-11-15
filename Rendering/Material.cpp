#include "Material.h"
#include "TextureManager.h"
#include "Platform/Paths.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <locale>
namespace Rendering
{
	static std::string Trim(std::string i_value)
	{
		auto first = i_value.find_first_not_of(" \t\r\n");
		if (first == std::string::npos)
			return {};
		return i_value.substr(first, i_value.find_last_not_of(" \t\r\n") - first + 1);
	}
	Material::Material(const std::string &i_path) : Material()
	{
		auto fail = [&](const std::string &message)
		{ throw std::runtime_error("Material " + i_path + ": " + message); };
		auto asset = [&](const std::string &path)
		{
			// 所有路径相对 Assets，规范化后拦住目录链接和 .. 逃逸。
			Platform::ToWide(path);
			auto relative = std::filesystem::u8path(path);
			if (path.empty() || relative.is_absolute() || relative.has_root_name() ||
				path.find(':') != std::string::npos || path.find('\0') != std::string::npos)
				fail("resource must be relative to Assets: " + path);
			auto root = std::filesystem::canonical(Platform::GetAssetPath(""));
			auto full = std::filesystem::weakly_canonical(root / relative);
			auto local = full.lexically_relative(root);
			if (local.empty() || local.is_absolute() || *local.begin() == ".." ||
				!std::filesystem::is_regular_file(full))
				fail("missing resource or path outside Assets: " + path);
			return local.generic_u8string();
		};
		auto texture = [&](const std::string &path) -> TexturePtr
		{
			// 显式空值表示无贴图，不把空路径交给解码器。
			if (path.empty())
				return nullptr;
			auto local = asset(path);
			return TextureManager::GetInstance()->Load(local, local);
		};
		m_fileName = i_path;
		auto text = Platform::ReadText(Platform::GetAssetPath(asset(i_path)));
		if (text.compare(0, 3, "\xEF\xBB\xBF") == 0)
			text.erase(0, 3);
		std::istringstream input(text);
		std::string line;
		int shaderLine = 0;
		while (std::getline(input, line))
		{
			line = Trim(line.substr(0, line.find('#')));
			if (line.empty())
				continue;
			if (shaderLine < 2)
			{
				// 保留原有两行格式，兼容迁移前的默认 Shader 路径。
				if (shaderLine == 0)
					m_vertexShader =
						asset(line == "Shaders/DefaultVS.shader" ? "Shaders/OpenGL/DefaultVS.shader" : line);
				else
					m_fragmentShader =
						asset(line == "Shaders/DefaultFS.shader" ? "Shaders/OpenGL/DefaultFS.shader" : line);
				++shaderLine;
				continue;
			}
			auto separator = line.find('=');
			if (separator == std::string::npos)
				fail("expected key=value: " + line);
			auto key = Trim(line.substr(0, separator)), value = Trim(line.substr(separator + 1));
			std::replace(value.begin(), value.end(), ',', ' ');
			std::istringstream fields(value);
			fields.imbue(std::locale::classic());
			auto number = [&](float &result)
			{
				if (!(fields >> result) || !std::isfinite(result))
					fail("invalid finite number for " + key + ": " + value);
			};
			bool numericField = true;
			if (key == "DX11")
			{
				m_dxShader = asset(Trim(line.substr(separator + 1)));
				numericField = false;
			}
			else if (key == "Texture" || key == "NormalTexture" || key == "OpacityTexture")
			{
				auto path = Trim(line.substr(separator + 1));
				auto loaded = texture(path);
				if (key == "Texture")
					m_texture = loaded;
				else if (key == "NormalTexture")
					m_normalTexture = loaded;
				else
					m_opacityTexture = loaded;
				numericField = false;
			}
			else if (key == "Color")
			{
				for (int i = 0; i < 4; ++i)
				{
					number(m_color[i]);
					if (m_color[i] < 0 || m_color[i] > 1)
						fail("Color must contain four values in [0,1]");
				}
			}
			else if (key == "Metallic")
			{
				number(m_metallic);
			}
			else if (key == "Roughness")
			{
				number(m_roughness);
			}
			else if (key == "AO")
			{
				number(m_ao);
			}
			else if (key == "AlphaCutoff")
			{
				number(m_alphaCutoff);
				if (m_alphaCutoff < 0 || m_alphaCutoff > 1)
					fail("AlphaCutoff must be in [0,1]");
			}
			else if (key == "Transparent")
			{
				auto flag = Trim(line.substr(separator + 1));
				if (flag != "true" && flag != "false")
					fail("Transparent must be true or false");
				m_transparent = flag == "true";
				numericField = false;
			}
			else
				fail("unknown field: " + key);
			if (numericField)
			{
				fields >> std::ws;
				if (!fields.eof())
					fail("unexpected extra value for " + key + ": " + value);
			}
		}
		if (shaderLine != 2)
			throw std::runtime_error("Material needs vertex and fragment shader paths: " + i_path);
		m_dxShader = asset(m_dxShader);
		// 旧字段沿用已有钳位范围，新字段在读取时严格校验。
		m_metallic = glm::clamp(m_metallic, 0.f, 1.f);
		m_roughness = glm::clamp(m_roughness, .045f, 1.f);
		m_ao = glm::clamp(m_ao, 0.f, 1.f);
	}
} // namespace Rendering
