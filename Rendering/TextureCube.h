#pragma once
#include "Texture.h"
#include <array>
#include <glm.hpp>
namespace Rendering
{
	class TextureCube : public IResource
	{
	  public:
		TextureCube();
		bool Init(const char *i_front, const char *i_back, const char *i_top, const char *i_bottom, const char *i_left,
				  const char *i_right);
		std::shared_ptr<TextureCube> ConvolveIrradiance(int i_size = 16) const;
		glm::vec3 Sample(const glm::vec3 &i_direction) const;
		static glm::vec3 GetDirection(int i_face, float i_u, float i_v);
		std::array<TexturePtr, 6> m_faces; // +X,-X,+Y,-Y,+Z,-Z, matching original cube faces.
	};
	using TextureCubePtr = std::shared_ptr<TextureCube>;
} // namespace Rendering
