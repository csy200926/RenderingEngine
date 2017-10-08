#pragma once
#include "Material.h"
#include <glm.hpp>
#include "SkyBox.h"
#include "TextureCube.h"
#include "RenderingEngine.h"
#include "GUI.h"
namespace Rendering
{

	class RenderingEngineDiffuse : public RenderingEngine
	{

	public:
		virtual void Render();

		virtual void Initilize();
		virtual void ShutDown();
	private: 
		GUI m_UIManager;
		
	};
}