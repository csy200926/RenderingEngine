#pragma once
#include "INodeComponent.h"
#include <glm.hpp>


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

		LightBase();


		virtual ~LightBase(){};
		
		glm::vec3 GetPosition()
		{ 
			return m_pNode->m_position;
		};



		LightType GetType(){ return m_type; }
		bool IsActivated(){ return m_isActivated; }
	protected:
		LightType m_type;

		glm::vec3 m_ambient;
		glm::vec3 m_diffuse;
		glm::vec3 m_specular;

		float m_intensity;
		float m_constant;
		float m_linear;
		float m_quadratic;

	private:
		bool m_isActivated;

		friend class RenderingEngine;
	};
	class DirectionalLight : public LightBase
	{
	public:
		DirectionalLight()
		{
			m_type = DIRECTIONAL;
		}

		// TODO: Hack
		glm::vec3 GetDirection(){ return glm::vec3(-1.0f, -1.0f, -1.0f); }
	};
	class PointLight :public LightBase
	{
	public:
		PointLight()
		{
			m_type = POINT;
		}
	};
	class SpotLight :public LightBase
	{
	public:
		SpotLight()
		{
			m_type = SPOT;
		}
		float m_cutoff;
	};

}
