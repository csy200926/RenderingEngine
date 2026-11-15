#pragma once
#include "IResources.h"
#include <vector>
namespace Rendering
{
	class Texture : public IResource
	{
	  public:
		explicit Texture(const std::string &i_path);
		Texture(int i_width, int i_height, const std::vector<unsigned char> &i_pixels);
		int m_width = 0, m_height = 0;
		bool m_hasAlpha = false;
		std::vector<unsigned char> m_pixels; // RGBA8, first row is the top of the image, v=0.
	};
	using TexturePtr = std::shared_ptr<Texture>;
} // namespace Rendering
