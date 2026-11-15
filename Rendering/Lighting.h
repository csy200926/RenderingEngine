#pragma once
#include "INodeComponent.h"
namespace Rendering
{
	enum LightType
	{
		DIRECTIONAL,
		POINT,
		SPOT,
		NONE
	};
	class LightBase : public INodeComponent
	{
	  public:
		void Render(RenderingEngine &i_engine) override;
		void SetColor(const glm::vec3 &i_color) { m_diffuse = i_color; }
		void SetIntensity(float i_intensity) { m_intensity = i_intensity; }
		LightType GetType() const { return m_type; }
		glm::vec3 GetPosition() const;
		glm::vec3 m_diffuse = glm::vec3(1), m_direction = glm::vec3(-1, -1, -1);
		float m_intensity = 1, m_constant = .2f, m_linear = .03f, m_quadratic = .01f;

	  protected:
		LightType m_type = NONE;
	};
	class DirectionalLight : public LightBase
	{
	  public:
		DirectionalLight() { m_type = DIRECTIONAL; }
	};
	class PointLight : public LightBase
	{
	  public:
		PointLight() { m_type = POINT; }
	};
} // namespace Rendering
