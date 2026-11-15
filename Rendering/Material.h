#pragma once
#include "IResources.h"
#include "Texture.h"
#include <glm.hpp>
namespace Rendering
{
	class Material : public IResource
	{
	  public:
		Material() : IResource(RT_Material) {}
		explicit Material(const std::string &i_path);
		void SetTexture(TexturePtr i_texture) { m_texture = std::move(i_texture); }
		void SetColor(const glm::vec3 &i_color) { m_color = glm::vec4(i_color, 1); }
		TexturePtr m_texture;
		TexturePtr m_opacityTexture, m_normalTexture;
		float m_alphaCutoff = 0;
		bool m_transparent = false;
		glm::vec4 m_color = glm::vec4(1);
		float m_metallic = .1f, m_roughness = .35f, m_ao = 1;
		std::string m_vertexShader = "Shaders/OpenGL/DefaultVS.shader";
		std::string m_fragmentShader = "Shaders/OpenGL/DefaultFS.shader";
		std::string m_dxShader = "Shaders/DX11/Default.hlsl";
	};
	using MaterialPtr = std::shared_ptr<Material>;
} // namespace Rendering
