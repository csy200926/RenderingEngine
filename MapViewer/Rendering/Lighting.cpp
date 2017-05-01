#include "Lighting.h"
#include "RenderingEngine.h"

namespace Rendering
{

	LightBase::LightBase() :m_type(NONE),
		m_intensity(1.0f),
		m_isActivated(true),
		m_diffuse(glm::vec3(1.0f, 1.0f, 1.0f)),
		m_specular(glm::vec3(1.0f, 1.0f, 1.0f))
	{
		
	}

	void LightBase::AddToEngine()
	{
		RenderingEngine::GetInstance()->AddLight(this);
	}


}



