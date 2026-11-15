#include "Texture.h"
#include "Platform/Paths.h"
#include <stb_image.h>
#include <stdexcept>
#include <limits>
namespace Rendering
{
	Texture::Texture(const std::string &i_path) : IResource(RT_Texture)
	{
		auto bytes = Platform::ReadBytes(Platform::GetAssetPath(i_path));
		if (bytes.size() > INT_MAX)
			throw std::runtime_error("Texture file too large");
		int channels = 0;
		auto *pPixels =
			stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &m_width, &m_height, &channels, 4);
		if (!pPixels)
			throw std::runtime_error("Texture decode failed: " + i_path);
		m_pixels.assign(pPixels, pPixels + static_cast<size_t>(m_width) * m_height * 4);
		stbi_image_free(pPixels);
		for (size_t i = 3; i < m_pixels.size(); i += 4)
			if (m_pixels[i] < 250)
			{
				m_hasAlpha = true;
				break;
			}
		m_fileName = i_path;
	}
	Texture::Texture(int i_width, int i_height, const std::vector<unsigned char> &i_pixels)
		: IResource(RT_Texture), m_width(i_width), m_height(i_height), m_pixels(i_pixels)
	{
		if (i_width <= 0 || i_height <= 0 || i_pixels.size() != static_cast<size_t>(i_width) * i_height * 4)
			throw std::invalid_argument("Invalid texture dimensions");
	}
} // namespace Rendering
