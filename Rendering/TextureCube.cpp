#include "TextureCube.h"
#include "Platform/Log.h"
#include <cmath>
#include <algorithm>
namespace Rendering
{
	TextureCube::TextureCube() : IResource(RT_Texture)
	{
	}
	bool TextureCube::Init(const char *i_front, const char *i_back, const char *i_top, const char *i_bottom,
						   const char *i_left, const char *i_right)
	{
		const char *paths[] = {i_right, i_left, i_top, i_bottom, i_back, i_front};
		for (int i = 0; i < 6; ++i)
			m_faces[i] = std::make_shared<Texture>(paths[i]);
		int size = m_faces[0]->m_width;
		for (auto &face : m_faces)
			if (face->m_width != size || face->m_height != size)
				throw std::runtime_error("Cubemap faces must be equally sized squares");
		return true;
	}
	glm::vec3 TextureCube::GetDirection(int i_face, float i_u, float i_v)
	{
		float x = 2 * i_u - 1, y = 2 * i_v - 1;
		glm::vec3 directions[] = {{1, -y, -x}, {-1, -y, x}, {x, 1, y}, {x, -1, -y}, {x, -y, 1}, {-x, -y, -1}};
		return glm::normalize(directions[i_face]);
	}
	glm::vec3 TextureCube::Sample(const glm::vec3 &i_direction) const
	{
		auto d = glm::normalize(i_direction);
		auto a = glm::abs(d);
		int face;
		float u, v;
		if (a.x >= a.y && a.x >= a.z)
		{
			face = d.x > 0 ? 0 : 1;
			u = (d.x > 0 ? -d.z : d.z) / a.x;
			v = -d.y / a.x;
		}
		else if (a.y >= a.z)
		{
			face = d.y > 0 ? 2 : 3;
			u = d.x / a.y;
			v = (d.y > 0 ? d.z : -d.z) / a.y;
		}
		else
		{
			face = d.z > 0 ? 4 : 5;
			u = (d.z > 0 ? d.x : -d.x) / a.z;
			v = -d.y / a.z;
		}
		auto &image = *m_faces[face];
		int x = std::clamp(static_cast<int>((u * .5f + .5f) * image.m_width), 0, image.m_width - 1);
		int y = std::clamp(static_cast<int>((v * .5f + .5f) * image.m_height), 0, image.m_height - 1);
		const auto *pPixel = image.m_pixels.data() + (y * image.m_width + x) * 4;
		return glm::pow(glm::vec3(pPixel[0], pPixel[1], pPixel[2]) / 255.f, glm::vec3(2.2f));
	}
	std::shared_ptr<TextureCube> TextureCube::ConvolveIrradiance(int i_size) const
	{
		if (i_size < 1)
			throw std::invalid_argument("Invalid irradiance size");
		auto result = std::make_shared<TextureCube>();
		// Same tangent-space hemisphere integral as the user's cube_convolution_fs.shader.
		// CPU preparation makes both backends consume identical convolution results.
		const float pi = 3.14159265359f;
		for (int face = 0; face < 6; ++face)
		{
			std::vector<unsigned char> pixels(i_size * i_size * 4);
			for (int y = 0; y < i_size; ++y)
				for (int x = 0; x < i_size; ++x)
				{
					auto n = GetDirection(face, (x + .5f) / i_size, (y + .5f) / i_size);
					auto up = std::abs(n.y) > .99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
					auto right = glm::normalize(glm::cross(up, n));
					up = glm::cross(n, right);
					glm::vec3 irradiance(0);
					float weight = 0;
					for (int p = 0; p < 32; ++p)
						for (int t = 0; t < 16; ++t)
						{
							float phi = (p + .5f) * 2 * pi / 32, theta = (t + .5f) * .5f * pi / 16;
							float w = std::cos(theta) * std::sin(theta);
							glm::vec3 tangent(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi),
											  std::cos(theta));
							irradiance += Sample(tangent.x * right + tangent.y * up + tangent.z * n) * w;
							weight += w;
						}
					irradiance =
						glm::pow(glm::clamp(irradiance / weight, glm::vec3(0), glm::vec3(1)), glm::vec3(1 / 2.2f));
					for (int c = 0; c < 3; ++c)
						pixels[(y * i_size + x) * 4 + c] = static_cast<unsigned char>(irradiance[c] * 255 + .5f);
					pixels[(y * i_size + x) * 4 + 3] = 255;
				}
			result->m_faces[face] = std::make_shared<Texture>(i_size, i_size, pixels);
		}
		Platform::Log("Environment: six-face irradiance convolution complete, " + std::to_string(i_size) + "x" +
					  std::to_string(i_size));
		return result;
	}
} // namespace Rendering
