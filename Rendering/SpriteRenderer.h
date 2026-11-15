#pragma once
#include "INodeComponent.h"
#include "Texture.h"
namespace Rendering
{
	class SpriteRenderer : public INodeComponent
	{
	  public:
		SpriteRenderer(TexturePtr i_texture, const glm::vec2 &i_size) : m_texture(std::move(i_texture)), m_size(i_size)
		{
		}
		void Render(RenderingEngine &i_engine) override;
		TexturePtr m_texture;
		glm::vec2 m_size;
		glm::vec4 m_color = glm::vec4(1);
	};
} // namespace Rendering
